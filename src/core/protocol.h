#pragma once
#include <QByteArray>
#include <QJsonObject>
#include <QString>
#include <optional>
#include <stdexcept>

namespace Protocol {
constexpr int Version = 2;
constexpr quint16 DiscoveryPort = 12345;
constexpr qsizetype MaxHeader = 16 * 1024;
constexpr qint64 ChunkSize = 64 * 1024;
constexpr qint64 MaxFileSize = 8LL * 1024 * 1024 * 1024 * 1024 * 1024;
class Error : public std::runtime_error {
public:
    explicit Error(const QString &reason) : std::runtime_error(reason.toUtf8().constData()) {}
};
QString fileName(const QString &value);
qint64 fileSize(const QJsonValue &value);
QJsonObject message(const QString &type, QJsonObject values = {});
QByteArray encode(const QJsonObject &message);
QJsonObject decode(const QByteArray &data);
std::optional<QJsonObject> takeFrame(QByteArray &buffer);
}
