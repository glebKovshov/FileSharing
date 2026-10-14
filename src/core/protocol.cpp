#include "protocol.h"
#include <QJsonDocument>
#include <QJsonParseError>
#include <QRegularExpression>
#include <QStringDecoder>
#include <QtEndian>
#include <cmath>

namespace Protocol {
QString fileName(const QString &input) {
    const auto name = input.normalized(QString::NormalizationForm_C);
    static const QRegularExpression reserved(QStringLiteral("^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(?:\\.|$)"), QRegularExpression::CaseInsensitiveOption);
    bool invalid = name.isEmpty() || name.toUtf8().size() > 240 || name == "." || name == ".."
        || name.endsWith('.') || name.endsWith(' ') || reserved.match(name).hasMatch();
    for (const auto c : name)
        invalid = invalid || c.unicode() < 32 || QStringLiteral("/\\:<>\"|?*").contains(c);
    if (invalid)
        throw Error(QStringLiteral("Имя файла несовместимо с одной из платформ. Переименуйте файл."));
    return name;
}
qint64 fileSize(const QJsonValue &value) {
    const auto number = value.toDouble(-1);
    const auto integer = value.toInteger(-1);
    if (!value.isDouble() || !std::isfinite(number) || integer < 0 || integer > MaxFileSize || std::floor(number) != number)
        throw Error(QStringLiteral("Некорректный размер файла"));
    return integer;
}
QJsonObject message(const QString &type, QJsonObject values) {
    values.insert("version", Version);
    values.insert("type", type);
    return values;
}
QByteArray encode(const QJsonObject &value) {
    const auto data = QJsonDocument(value).toJson(QJsonDocument::Compact);
    if (data.isEmpty() || data.size() > MaxHeader)
        throw Error(QStringLiteral("Слишком длинное сообщение"));
    QByteArray frame(4, '\0');
    qToBigEndian<quint32>(static_cast<quint32>(data.size()), frame.data());
    return frame + data;
}
QJsonObject decode(const QByteArray &data) {
    QStringDecoder decoder(QStringDecoder::Utf8);
    const QString text = decoder(data);
    Q_UNUSED(text)
    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(data, &error);
    if (decoder.hasError() || error.error != QJsonParseError::NoError || !document.isObject())
        throw Error(QStringLiteral("Некорректное сетевое сообщение"));
    const auto value = document.object();
    if (!value.value("version").isDouble() || value.value("version").toDouble() != Version)
        throw Error(QStringLiteral("Несовместимая версия приложения"));
    return value;
}
std::optional<QJsonObject> takeFrame(QByteArray &buffer) {
    if (buffer.size() < 4)
        return std::nullopt;
    const auto length = qFromBigEndian<quint32>(buffer.constData());
    if (length == 0 || length > MaxHeader)
        throw Error(QStringLiteral("Некорректная длина сообщения"));
    if (buffer.size() < 4 + static_cast<qsizetype>(length))
        return std::nullopt;
    const auto data = buffer.mid(4, length);
    buffer.remove(0, 4 + length);
    return decode(data);
}
}
