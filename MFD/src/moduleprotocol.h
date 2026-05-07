#pragma once

#include <QByteArray>
#include <QVariantMap>

namespace ModuleProtocol {

enum class MessageType {
    Unknown,
    Heartbeat,
    AdcFlight,
    GnssNav,
    EisSummary,
    EisCylinders,
    EisFuelTank,
    MagHeading,
    AdsbTraffic,
    AdsbWeatherHeader,
    AdsbWeatherPayload,
};

struct DecodedFrame {
    MessageType type = MessageType::Unknown;
    QVariantMap fields;
};

bool decodeFrame(quint32 frameId, const QByteArray &payload, DecodedFrame *decodedFrame);
QString moduleKeyForType(int moduleType);
QString moduleNameForType(int moduleType);
QString modeLabel(int mode);
bool healthIsHealthy(int health);

} // namespace ModuleProtocol
