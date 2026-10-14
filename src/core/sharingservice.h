#pragma once
#include "transfer.h"
#include <QHash>
#include <QPointer>
#include <QTcpServer>
#include <QUdpSocket>

class SharingService : public QObject {
    Q_OBJECT
public:
    explicit SharingService(QObject *parent = nullptr, quint16 port = 12345);
    bool busy() const { return current && !current->isDone(); }
    bool offering() const { return !offer.isEmpty(); }
    bool networkReady() const { return ready; }
    quint16 discoveryPort() const { return udp.localPort(); }
    const QJsonObject &currentOffer() const { return offer; }
    QList<Peer> availablePeers() const { return peers.values(); }
    void setOffer(const QString &path);
    void clearOffer();
    void search(bool enabled = true);
    ReceiveTransfer *receive(const Peer &peer, const QString &directory);
    void sendDatagram(const QJsonObject &value, const QHostAddress &address = {}, quint16 port = 0);
    void stop();
signals:
    void peersChanged(const QList<Peer> &peers);
    void sessionStarted(Transfer *session, bool sending);
    void error(const QString &reason);
    void offerChanged(bool enabled);
private:
    QString id, path;
    quint16 port;
    QJsonObject offer;
    QHash<QString, Peer> peers;
    QUdpSocket udp;
    QTcpServer server;
    QTimer timer;
    QPointer<Transfer> current;
    bool receiving = false, ready = false;
    void readDatagrams();
    void tick();
    void accept();
    void watch(Transfer *transfer, bool sending);
    void release(Transfer *transfer);
};
