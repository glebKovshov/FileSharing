#include "sharingservice.h"
#include "protocol.h"
#include <QDateTime>
#include <QFileInfo>
#include <QHostInfo>
#include <QJsonDocument>
#include <QNetworkDatagram>
#include <QNetworkInterface>
#include <QRegularExpression>
#include <QUuid>
#include <QSet>

static QString identity() { return QUuid::createUuid().toString(QUuid::Id128); }
SharingService::SharingService(QObject *parent, quint16 discoveryPort)
    : QObject(parent), id(identity()), port(discoveryPort) {
    server.setMaxPendingConnections(1);
    connect(&server, &QTcpServer::newConnection, this, &SharingService::accept);
    connect(&udp, &QUdpSocket::readyRead, this, &SharingService::readDatagrams);
    timer.setInterval(2000);
    connect(&timer, &QTimer::timeout, this, &SharingService::tick);
    timer.start();
    ready = udp.bind(QHostAddress::AnyIPv4, port, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
    if (!ready) QTimer::singleShot(0, this, [this] { emit error(tr("Не удалось запустить поиск устройств: %1").arg(udp.errorString())); });
}
void SharingService::sendDatagram(const QJsonObject &value, const QHostAddress &address, quint16 destinationPort) {
    const auto bytes = QJsonDocument(value).toJson(QJsonDocument::Compact);
    if (!address.isNull()) { udp.writeDatagram(bytes, address, destinationPort ? destinationPort : port); return; }
    if (port == 0) return;
    QSet<QString> targets{QStringLiteral("255.255.255.255")};
    for (const auto &interface : QNetworkInterface::allInterfaces()) {
        if (!(interface.flags() & QNetworkInterface::IsUp) || (interface.flags() & QNetworkInterface::IsLoopBack)) continue;
        for (const auto &entry : interface.addressEntries())
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol && !entry.broadcast().isNull()) targets.insert(entry.broadcast().toString());
    }
    for (const auto &target : targets) udp.writeDatagram(bytes, QHostAddress(target), port);
}
void SharingService::setOffer(const QString &filePath) {
    if (busy()) throw Protocol::Error(tr("Сначала завершите или отмените текущую передачу"));
    QFileInfo info(filePath);
    const auto name = Protocol::fileName(info.fileName());
    if (!info.isFile()) throw Protocol::Error(tr("Выберите обычный файл"));
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) throw Protocol::Error(tr("Не удалось открыть файл: %1").arg(file.errorString()));
    const auto length = Protocol::fileSize(QJsonValue(file.size()));
    clearOffer();
    if (!server.listen(QHostAddress::AnyIPv4, 0)) throw Protocol::Error(tr("Не удалось открыть порт передачи: %1").arg(server.errorString()));
    path = info.absoluteFilePath();
    auto device = QHostInfo::localHostName().left(100);
    if (device.isEmpty()) device = tr("Компьютер");
    offer = Protocol::message("offer", {{"id", id}, {"offer", identity()}, {"device", device},
        {"name", name}, {"size", length}, {"port", server.serverPort()}});
    emit offerChanged(true);
    tick();
}
void SharingService::clearOffer() {
    if (!offer.isEmpty()) sendDatagram(Protocol::message("withdraw", {{"id", id}}));
    offer = {};
    path.clear();
    server.close();
    emit offerChanged(false);
}
void SharingService::search(bool enabled) {
    receiving = enabled;
    peers.clear();
    emit peersChanged({});
    if (enabled) sendDatagram(Protocol::message("query", {{"id", id}}));
}
void SharingService::tick() {
    if (offering() && !busy()) sendDatagram(offer);
    if (receiving && !busy()) sendDatagram(Protocol::message("query", {{"id", id}}));
    const auto now = QDateTime::currentMSecsSinceEpoch();
    bool changed = false;
    for (auto it = peers.begin(); it != peers.end();) {
        if (now - it->seen > 7000) { it = peers.erase(it); changed = true; } else ++it;
    }
    if (changed) emit peersChanged(peers.values());
}
void SharingService::readDatagrams() {
    static const QRegularExpression validId(QStringLiteral("^[a-f0-9]{32}$"));
    while (udp.hasPendingDatagrams()) {
        const auto datagram = udp.receiveDatagram(4097);
        const auto raw = datagram.data();
        if (raw.size() > 4096) continue;
        try {
            const auto value = Protocol::decode(raw);
            const auto peerId = value.value("id").toString();
            if (!validId.match(peerId).hasMatch() || peerId == id) continue;
            const auto type = value.value("type").toString();
            if (type == "query" && offering() && !busy()) {
                sendDatagram(offer, datagram.senderAddress(), datagram.senderPort());
            } else if (type == "withdraw") {
                if (peers.remove(peerId)) emit peersChanged(peers.values());
            } else if (type == "offer" && receiving) {
                const auto portValue = value.value("port");
                const double portNumber = portValue.toDouble(-1);
                const auto offerId = value.value("offer").toString();
                const auto device = value.value("device").toString();
                if (!portValue.isDouble() || portNumber < 1 || portNumber > 65535 || static_cast<int>(portNumber) != portNumber
                    || !validId.match(offerId).hasMatch() || device.isEmpty() || device.size() > 100) continue;
                bool control = false;
                for (const auto c : device) control = control || c.unicode() < 32;
                if (control) continue;
                Peer peer;
                peer.id = peerId; peer.offer = offerId; peer.device = device;
                peer.name = Protocol::fileName(value.value("name").toString());
                peer.size = Protocol::fileSize(value.value("size"));
                peer.address = datagram.senderAddress().toString();
                peer.port = static_cast<quint16>(portNumber);
                peer.seen = QDateTime::currentMSecsSinceEpoch();
                if (peers.size() < 200 || peers.contains(peerId)) {
                    const bool changed = !peers.contains(peerId) || peers[peerId].offer != peer.offer || peers[peerId].address != peer.address
                        || peers[peerId].name != peer.name || peers[peerId].size != peer.size || peers[peerId].port != peer.port || peers[peerId].device != peer.device;
                    peers.insert(peerId, peer);
                    if (changed) emit peersChanged(peers.values());
                }
            }
        } catch (const std::exception &) { /* Ignore malformed and legacy datagrams. */ }
    }
}
void SharingService::watch(Transfer *transfer, bool sending) {
    current = transfer;
    connect(transfer, &Transfer::finished, this, [this, transfer] { release(transfer); });
    connect(transfer, &Transfer::failed, this, [this, transfer] { release(transfer); });
    emit sessionStarted(transfer, sending);
}
void SharingService::release(Transfer *transfer) {
    if (current == transfer) current = nullptr;
    // Drain the receiver acknowledgement before destroying its socket.
    if (transfer->connection()->state() == QAbstractSocket::UnconnectedState) transfer->deleteLater();
    else connect(transfer->connection(), &QTcpSocket::disconnected, transfer, &QObject::deleteLater);
    tick();
}
void SharingService::accept() {
    while (server.hasPendingConnections()) {
        auto *socket = server.nextPendingConnection();
        if (!offering() || busy()) { socket->abort(); socket->deleteLater(); continue; }
        watch(new SendTransfer(socket, path, offer.value("offer").toString(), this), true);
    }
}
ReceiveTransfer *SharingService::receive(const Peer &peer, const QString &directory) {
    if (busy()) throw Protocol::Error(tr("Уже выполняется передача"));
    auto *transfer = new ReceiveTransfer(peer, directory, this);
    watch(transfer, false);
    return transfer;
}
void SharingService::stop() {
    timer.stop();
    clearOffer();
    if (busy()) current->cancel();
    receiving = false;
    udp.close();
}
