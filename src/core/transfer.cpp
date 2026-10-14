#include "transfer.h"
#include "protocol.h"
#include <QDir>
#include <QFileInfo>
#include <QStorageInfo>

Transfer::Transfer(QTcpSocket *connection, QObject *parent) : QObject(parent), socket(connection) {
    socket->setParent(this);
    socket->setReadBufferSize(Protocol::ChunkSize * 4);
    timeout.setSingleShot(true);
    timeout.setInterval(45000);
    connect(&timeout, &QTimer::timeout, this, [this] { fail(tr("Устройство не отвечает. Проверьте сеть и повторите передачу.")); });
    connect(socket, &QTcpSocket::readyRead, this, [this] {
        if (done) return;
        touch();
        buffer += socket->readAll();
        try { consume(); }
        catch (const std::exception &error) { fail(QString::fromUtf8(error.what())); }
    });
    connect(socket, &QTcpSocket::bytesWritten, this, [this] { touch(); });
    connect(socket, &QTcpSocket::disconnected, this, [this] {
        if (!done) fail(tr("Соединение прервалось. Неполный файл удалён; повторите попытку."));
    });
    connect(socket, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        fail(tr("Ошибка соединения: %1").arg(socket->errorString()));
    });
    touch();
}
void Transfer::setTimeout(int milliseconds) { timeout.setInterval(milliseconds); touch(); }
void Transfer::touch() { if (!done) timeout.start(); }
void Transfer::writeFrame(const QJsonObject &value) {
    const auto frame = Protocol::encode(value);
    if (socket->write(frame) != frame.size()) throw Protocol::Error(tr("Не удалось отправить сообщение"));
    touch();
}
void Transfer::cancel() { fail(tr("Передача отменена")); }
void Transfer::fail(const QString &reason) {
    if (done) return;
    done = true;
    timeout.stop();
    cleanup();
    socket->abort();
    emit failed(reason);
}
void Transfer::succeed(const QString &path) {
    done = true;
    timeout.stop();
    cleanup();
    emit finished(path);
    socket->disconnectFromHost();
}

SendTransfer::SendTransfer(QTcpSocket *socket, const QString &path, const QString &offer, QObject *parent)
    : Transfer(socket, parent), source(path), offer(offer) {
    connect(socket, &QTcpSocket::bytesWritten, this, [this] { pump(); });
}
void SendTransfer::consume() {
    const auto frame = Protocol::takeFrame(buffer);
    if (!frame) return;
    if (!buffer.isEmpty()) throw Protocol::Error(tr("Лишние данные в запросе"));
    if (state == Request) {
        if (frame->value("type") != "pull" || frame->value("offer") != offer)
            throw Protocol::Error(tr("Файл уже недоступен. Обновите список устройств."));
        if (!source.open(QIODevice::ReadOnly)) throw Protocol::Error(tr("Не удалось открыть файл: %1").arg(source.errorString()));
        total = Protocol::fileSize(QJsonValue(source.size()));
        writeFrame(Protocol::message("file", {{"name", Protocol::fileName(QFileInfo(source).fileName())}, {"size", total}, {"offer", offer}}));
        state = Sending;
        pump();
    } else if (state == Ack) {
        if (frame->value("type") != "complete" || frame->value("sha256").toString().toLatin1() != hash.result().toHex())
            throw Protocol::Error(tr("Получатель не подтвердил целостность файла"));
        succeed(source.fileName());
    } else {
        throw Protocol::Error(tr("Неожиданный ответ устройства"));
    }
}
void SendTransfer::pump() {
    if (done || state != Sending) return;
    try {
        while (socket->bytesToWrite() < Protocol::ChunkSize * 4 && count < total) {
            const auto data = source.read(qMin(Protocol::ChunkSize, total - count));
            if (data.isEmpty()) throw Protocol::Error(tr("Файл изменился во время передачи. Выберите его снова."));
            if (socket->write(data) != data.size()) throw Protocol::Error(tr("Не удалось отправить данные"));
            hash.addData(data);
            count += data.size();
            emit progress(count, total);
        }
        if (count == total) {
            if (source.size() != total || !source.read(1).isEmpty()) throw Protocol::Error(tr("Файл изменился во время передачи"));
            state = Ack;
            writeFrame(Protocol::message("digest", {{"sha256", QString::fromLatin1(hash.result().toHex())}}));
        }
    } catch (const std::exception &error) { fail(QString::fromUtf8(error.what())); }
}
void SendTransfer::cleanup() { source.close(); }

ReceiveTransfer::ReceiveTransfer(const Peer &peer, const QString &directory, QObject *parent)
    : Transfer(new QTcpSocket, parent), peer(peer), directory(directory) {
    connect(socket, &QTcpSocket::connected, this, [this] {
        try { writeFrame(Protocol::message("pull", {{"offer", this->peer.offer}})); }
        catch (const std::exception &error) { fail(QString::fromUtf8(error.what())); }
    });
    socket->connectToHost(peer.address, peer.port);
}
void ReceiveTransfer::consume() {
    if (state == Header) {
        const auto frame = Protocol::takeFrame(buffer);
        if (!frame) return;
        if (frame->value("type") != "file" || frame->value("offer") != peer.offer)
            throw Protocol::Error(tr("Устройство предложило другой файл. Повторите поиск."));
        name = Protocol::fileName(frame->value("name").toString());
        total = Protocol::fileSize(frame->value("size"));
        if (name != peer.name || total != peer.size)
            throw Protocol::Error(tr("Файл изменился. Обновите список и подтвердите приём заново."));
        if (!QDir().mkpath(directory)) throw Protocol::Error(tr("Не удалось создать папку сохранения"));
        QStorageInfo storage(directory);
        if (storage.isValid() && storage.bytesAvailable() >= 0 && storage.bytesAvailable() < total + 1024 * 1024)
            throw Protocol::Error(tr("Недостаточно места в выбранной папке"));
        temp = std::make_unique<QTemporaryFile>(QDir(directory).filePath(".filesharing-XXXXXX.part"));
        if (!temp->open()) throw Protocol::Error(tr("Не удалось создать файл: %1").arg(temp->errorString()));
        state = Body;
    }
    if (state == Body) {
        const auto length = qMin<qint64>(buffer.size(), total - count);
        if (length > 0) {
            const auto data = buffer.left(length);
            buffer.remove(0, length);
            if (temp->write(data) != length) throw Protocol::Error(tr("Ошибка записи: проверьте свободное место и права доступа"));
            hash.addData(data);
            count += length;
            emit progress(count, total);
        }
        if (count < total) return;
        state = Digest;
    }
    if (state == Digest) {
        const auto frame = Protocol::takeFrame(buffer);
        if (!frame) return;
        if (frame->value("type") != "digest" || frame->value("sha256").toString().toLatin1() != hash.result().toHex())
            throw Protocol::Error(tr("Проверка целостности не пройдена. Файл не сохранён."));
        if (!buffer.isEmpty()) throw Protocol::Error(tr("Получены лишние данные. Файл не сохранён."));
        if (!temp->flush()) throw Protocol::Error(tr("Не удалось сохранить файл на диск"));
        temp->close();
        QString target;
        const QFileInfo info(name);
        for (int index = 0; index < 10000; ++index) {
            QString candidate = name;
            if (index) {
                const auto suffix = info.suffix();
                candidate = info.completeBaseName() + QString(" (%1)").arg(index) + (suffix.isEmpty() ? QString() : "." + suffix);
            }
            target = QDir(directory).filePath(candidate);
            if (QFileInfo::exists(target)) continue;
            // QFile::rename refuses to overwrite existing files, including races.
            if (temp->rename(target)) { temp->setAutoRemove(false); break; }
            if (!QFileInfo::exists(target)) throw Protocol::Error(tr("Не удалось сохранить файл: %1").arg(temp->errorString()));
        }
        if (temp->autoRemove()) throw Protocol::Error(tr("Слишком много файлов с таким именем"));
        state = Complete;
        writeFrame(Protocol::message("complete", {{"sha256", QString::fromLatin1(hash.result().toHex())}}));
        succeed(target);
    }
}
void ReceiveTransfer::cleanup() {
    if (temp && temp->autoRemove()) temp->remove();
}
