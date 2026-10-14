#pragma once
#include <QObject>
#include <QTimer>
#include <QTcpSocket>
#include <QFile>
#include <QTemporaryFile>
#include <QCryptographicHash>
#include <QJsonObject>
#include <memory>

struct Peer {
    QString id, offer, device, name, address;
    qint64 size = 0;
    quint16 port = 0;
    qint64 seen = 0;
};
Q_DECLARE_METATYPE(Peer)

class Transfer : public QObject {
    Q_OBJECT
public:
    explicit Transfer(QTcpSocket *socket, QObject *parent = nullptr);
    bool isDone() const { return done; }
    QTcpSocket *connection() const { return socket; }
    void cancel();
    void setTimeout(int milliseconds);
signals:
    void progress(qint64 bytes, qint64 total);
    void finished(const QString &path);
    void failed(const QString &reason);
protected:
    QTcpSocket *socket;
    QByteArray buffer;
    QTimer timeout;
    QCryptographicHash hash{QCryptographicHash::Sha256};
    bool done = false;
    qint64 total = 0, count = 0;
    void writeFrame(const QJsonObject &value);
    void touch();
    void fail(const QString &reason);
    void succeed(const QString &path);
    virtual void consume() = 0;
    virtual void cleanup() {}
};

class SendTransfer final : public Transfer {
    Q_OBJECT
public:
    SendTransfer(QTcpSocket *socket, const QString &path, const QString &offer, QObject *parent = nullptr);
protected:
    void consume() override;
    void cleanup() override;
private:
    enum State { Request, Sending, Ack } state = Request;
    QFile source;
    QString offer;
    void pump();
};

class ReceiveTransfer final : public Transfer {
    Q_OBJECT
public:
    ReceiveTransfer(const Peer &peer, const QString &directory, QObject *parent = nullptr);
protected:
    void consume() override;
    void cleanup() override;
private:
    enum State { Header, Body, Digest, Complete } state = Header;
    Peer peer;
    QString directory, name;
    std::unique_ptr<QTemporaryFile> temp;
};
