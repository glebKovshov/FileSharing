#include "core/protocol.h"
#include "core/sharingservice.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

class CoreTests : public QObject {
    Q_OBJECT
private slots:
    void fragmentedUnicodeFrame() {
        const auto packet = Protocol::encode(Protocol::message("file", {{"name", QString::fromUtf8("Привет 🎉.txt")}, {"size", 7}}));
        QByteArray buffer;
        for (int i = 0; i < packet.size() - 1; ++i) {
            buffer.append(packet[i]);
            QVERIFY(!Protocol::takeFrame(buffer));
        }
        buffer.append(packet.back());
        const auto frame = Protocol::takeFrame(buffer);
        QVERIFY(frame);
        QCOMPARE(frame->value("name").toString(), QString::fromUtf8("Привет 🎉.txt"));
        QVERIFY(buffer.isEmpty());
    }
    void unsafeNames_data() {
        QTest::addColumn<QString>("name");
        const QStringList names{"../a", "C:\\file", "", "NUL.txt", "x.", "a/b", "COM1", QString("x") + QChar(0) + ".txt"};
        int i = 0;
        for (const auto &name : names) QTest::newRow(qPrintable(QString::number(i++))) << name;
    }
    void unsafeNames() {
        QFETCH(QString, name);
        QVERIFY_EXCEPTION_THROWN(Protocol::fileName(name), Protocol::Error);
    }
    void invalidFrames() {
        for (const auto length : {0u, 16385u, 0xffffffffu}) {
            QByteArray buffer(4, '\0');
            qToBigEndian<quint32>(length, buffer.data());
            QVERIFY_EXCEPTION_THROWN(Protocol::takeFrame(buffer), Protocol::Error);
        }
        for (const auto &value : QList<QJsonValue>{QJsonValue(true), QJsonValue(-1), QJsonValue(1.5), QJsonValue("12")})
            QVERIFY_EXCEPTION_THROWN(Protocol::fileSize(value), Protocol::Error);
        QVERIFY_EXCEPTION_THROWN(Protocol::decode(QByteArray("\xff\xfe")), Protocol::Error);
        const auto tooLarge = QJsonDocument::fromJson("{\"size\":9007199254740993}").object().value("size");
        QVERIFY_EXCEPTION_THROWN(Protocol::fileSize(tooLarge), Protocol::Error);
    }
    void exactBytes_data() {
        QTest::addColumn<int>("length");
        for (int length : {0, 1, 1023, 65535, 65536, 65537, 2 * 1024 * 1024 + 37})
            QTest::newRow(qPrintable(QString::number(length))) << length;
    }
    void exactBytes() {
        QFETCH(int, length);
        QTemporaryDir folder;
        QVERIFY(folder.isValid());
        const auto name = QString::fromUtf8("Пример 🎉.bin");
        QByteArray data(length, '\0');
        for (int i = 0; i < length; ++i) data[i] = static_cast<char>(i * 31);
        const auto path = folder.filePath(name);
        QFile source(path);
        QVERIFY(source.open(QIODevice::WriteOnly));
        QCOMPARE(source.write(data), data.size());
        source.close();
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost, 0));
        QPointer<SendTransfer> sender;
        connect(&server, &QTcpServer::newConnection, this, [&] {
            sender = new SendTransfer(server.nextPendingConnection(), path, "test", &server);
        });
        Peer peer;
        peer.address = "127.0.0.1"; peer.port = server.serverPort(); peer.offer = "test"; peer.name = name; peer.size = length;
        ReceiveTransfer receiver(peer, folder.filePath("received"));
        QSignalSpy finished(&receiver, &Transfer::finished);
        QSignalSpy errors(&receiver, &Transfer::failed);
        QTRY_VERIFY_WITH_TIMEOUT(receiver.isDone(), 10000);
        QTRY_VERIFY_WITH_TIMEOUT(sender && sender->isDone(), 10000);
        QCOMPARE(errors.size(), 0);
        QCOMPARE(finished.size(), 1);
        QFile result(finished[0][0].toString());
        QVERIFY(result.open(QIODevice::ReadOnly));
        QCOMPARE(result.readAll(), data);
        QCOMPARE(QDir(folder.filePath("received")).entryList(QDir::Files | QDir::Hidden).size(), 1);
    }
    void rejectedReceive_data() {
        QTest::addColumn<QString>("failure");
        for (const auto &type : {"digest", "disconnect", "cancel", "timeout", "metadata", "extra"}) QTest::newRow(type) << QString(type);
    }
    void rejectedReceive() {
        QFETCH(QString, failure);
        QTemporaryDir folder;
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost, 0));
        connect(&server, &QTcpServer::newConnection, this, [&] {
            auto *socket = server.nextPendingConnection();
            connect(socket, &QTcpSocket::readyRead, socket, [socket, failure] {
                socket->readAll();
                socket->write(Protocol::encode(Protocol::message("file", {{"offer", "test"}, {"name", "data.bin"}, {"size", failure == "metadata" ? 10 : 100}})));
                socket->write(QByteArray(failure == "digest" || failure == "extra" ? 100 : 5, 'x'));
                if (failure == "digest" || failure == "extra") {
                    const auto digest = failure == "digest" ? QByteArray(64, '0') : QCryptographicHash::hash(QByteArray(100, 'x'), QCryptographicHash::Sha256).toHex();
                    socket->write(Protocol::encode(Protocol::message("digest", {{"sha256", QString::fromLatin1(digest)}})) + (failure == "extra" ? QByteArray("junk") : QByteArray()));
                }
                if (failure == "disconnect") socket->disconnectFromHost();
            });
        });
        Peer peer;
        peer.address = "127.0.0.1"; peer.port = server.serverPort(); peer.offer = "test"; peer.name = "data.bin"; peer.size = 100;
        ReceiveTransfer receiver(peer, folder.filePath("received"));
        QSignalSpy errors(&receiver, &Transfer::failed);
        if (failure == "cancel") QTimer::singleShot(100, &receiver, &Transfer::cancel);
        if (failure == "timeout") receiver.setTimeout(100);
        QTRY_VERIFY_WITH_TIMEOUT(receiver.isDone(), 5000);
        QCOMPARE(errors.size(), 1);
        QVERIFY(QDir(folder.filePath("received")).entryList(QDir::Files | QDir::Hidden).isEmpty());
    }
    void garbageDiscoveryIgnored() {
        SharingService receiver(nullptr, 0);
        receiver.search();
        QUdpSocket udp;
        for (const auto &raw : QList<QByteArray>{"PC-NAME", "{}", QByteArray("\xff\xfe"), QByteArray(5000, 'x')})
            udp.writeDatagram(raw, QHostAddress::LocalHost, receiver.discoveryPort());
        const auto offer = Protocol::message("offer", {{"id", QString(32, 'a')}, {"offer", QString(32, 'b')},
            {"device", QString::fromUtf8("Ноутбук")}, {"name", QString::fromUtf8("Файл.txt")}, {"size", 5}, {"port", 43210}});
        udp.writeDatagram(QJsonDocument(offer).toJson(QJsonDocument::Compact), QHostAddress::LocalHost, receiver.discoveryPort());
        QTRY_COMPARE(receiver.availablePeers().size(), 1);
        QCOMPARE(receiver.availablePeers().first().device, QString::fromUtf8("Ноутбук"));
        receiver.stop();
    }
    void repeatTransferPreservesExisting() {
        QTemporaryDir folder;
        const auto name = QString::fromUtf8("Первый запуск.txt");
        const auto path = folder.filePath(name);
        QFile source(path);
        QVERIFY(source.open(QIODevice::WriteOnly));
        const QByteArray data("no garbage on first launch\0binary\xff", 34);
        source.write(data); source.close();
        QVERIFY(QDir().mkpath(folder.filePath("downloads")));
        QFile existing(QDir(folder.filePath("downloads")).filePath(name));
        QVERIFY(existing.open(QIODevice::WriteOnly)); existing.write("keep me"); existing.close();
        SharingService sender(nullptr, 0), receiver(nullptr, 0);
        sender.setOffer(path);
        receiver.search();
        for (int attempt = 0; attempt < 2; ++attempt) {
            sender.sendDatagram(sender.currentOffer(), QHostAddress::LocalHost, receiver.discoveryPort());
            QTRY_COMPARE(receiver.availablePeers().size(), 1);
            auto *transfer = receiver.receive(receiver.availablePeers().first(), folder.filePath("downloads"));
            QSignalSpy finished(transfer, &Transfer::finished), errors(transfer, &Transfer::failed);
            QTRY_VERIFY_WITH_TIMEOUT(finished.size() || errors.size(), 5000);
            QCOMPARE(errors.size(), 0);
            QCOMPARE(finished.size(), 1);
            QTRY_VERIFY_WITH_TIMEOUT(!sender.busy(), 5000);
            QFile result(finished[0][0].toString());
            QVERIFY(result.open(QIODevice::ReadOnly));
            QCOMPARE(result.readAll(), data);
        }
        QCOMPARE(QDir(folder.filePath("downloads")).entryList(QDir::Files).size(), 3);
        QVERIFY(existing.open(QIODevice::ReadOnly)); QCOMPARE(existing.readAll(), QByteArray("keep me"));
        sender.stop(); receiver.stop();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }
    void sourceFailureAllowsRetry() {
        QTemporaryDir folder;
        const auto path = folder.filePath("retry.bin");
        QFile source(path);
        QVERIFY(source.open(QIODevice::WriteOnly)); source.write("hello"); source.close();
        SharingService sender(nullptr, 0), receiver(nullptr, 0);
        sender.setOffer(path); receiver.search();
        const auto removeConnection = connect(&sender, &SharingService::sessionStarted, this, [&source] { QVERIFY(source.remove()); });
        sender.sendDatagram(sender.currentOffer(), QHostAddress::LocalHost, receiver.discoveryPort());
        QTRY_COMPARE(receiver.availablePeers().size(), 1);
        auto *first = receiver.receive(receiver.availablePeers().first(), folder.filePath("downloads"));
        QSignalSpy errors(first, &Transfer::failed);
        QTRY_COMPARE_WITH_TIMEOUT(errors.size(), 1, 5000);
        QTRY_VERIFY(!sender.busy()); QVERIFY(!receiver.busy());
        QVERIFY(QDir(folder.filePath("downloads")).entryList(QDir::Files | QDir::Hidden).isEmpty());
        disconnect(removeConnection);
        QVERIFY(source.open(QIODevice::WriteOnly)); source.write("hello"); source.close();
        sender.setOffer(path);
        sender.sendDatagram(sender.currentOffer(), QHostAddress::LocalHost, receiver.discoveryPort());
        QTRY_COMPARE(receiver.availablePeers().first().offer, sender.currentOffer().value("offer").toString());
        auto *second = receiver.receive(receiver.availablePeers().first(), folder.filePath("downloads"));
        QSignalSpy success(second, &Transfer::finished);
        QTRY_COMPARE_WITH_TIMEOUT(success.size(), 1, 5000);
        QTRY_VERIFY(!sender.busy());
        sender.stop(); receiver.stop();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }
};
QTEST_GUILESS_MAIN(CoreTests)
#include "core_tests.moc"
