#include "moduleprotocol.h"

#include "../../Common/pfd_can_protocol.h"

#include <cstring>

namespace {

template <typename Payload>
bool copyPayload(const QByteArray &bytes, Payload *payload)
{
    if (payload == nullptr || bytes.size() < static_cast<int>(sizeof(Payload))) {
        return false;
    }

    std::memcpy(payload, bytes.constData(), sizeof(Payload));
    return true;
}

QString trimAsciiField(const char *data, const int size)
{
    QByteArray bytes(data, size);
    const int nullIndex = bytes.indexOf('\0');
    if (nullIndex >= 0) {
        bytes.truncate(nullIndex);
    }
    return QString::fromLatin1(bytes).trimmed();
}

} // namespace

namespace ModuleProtocol {

bool decodeFrame(const quint32 frameId, const QByteArray &payload, DecodedFrame *decodedFrame)
{
    if (decodedFrame == nullptr) {
        return false;
    }

    *decodedFrame = {};

    if (frameId >= PfdCan::kHeartbeatBaseId && frameId < (PfdCan::kHeartbeatBaseId + 0x10)) {
        PfdCan::HeartbeatPayload heartbeat {};
        if (!copyPayload(payload, &heartbeat)) {
            return false;
        }

        decodedFrame->type = MessageType::Heartbeat;
        decodedFrame->fields = {
            { QStringLiteral("moduleType"), static_cast<int>(heartbeat.moduleType) },
            { QStringLiteral("instanceId"), static_cast<int>(heartbeat.instanceId) },
            { QStringLiteral("health"), static_cast<int>(heartbeat.health) },
            { QStringLiteral("mode"), static_cast<int>(heartbeat.mode) },
            { QStringLiteral("capabilityFlags"), static_cast<quint32>(heartbeat.capabilityFlags) },
            { QStringLiteral("counterA"), static_cast<quint32>(heartbeat.counterA) },
            { QStringLiteral("counterB"), static_cast<quint32>(heartbeat.counterB) },
        };
        return true;
    }

    if (frameId == PfdCan::kAdcFlightId) {
        PfdCan::AdcFlightPayload flight {};
        if (!copyPayload(payload, &flight)) {
            return false;
        }

        decodedFrame->type = MessageType::AdcFlight;
        decodedFrame->fields = {
            { QStringLiteral("pitchDeg"), flight.pitchDeg },
            { QStringLiteral("rollDeg"), flight.rollDeg },
            { QStringLiteral("headingDeg"), flight.headingDeg },
            { QStringLiteral("turnRateDegPerSec"), flight.turnRateDegPerSec },
            { QStringLiteral("slipSkidNormalized"), flight.slipSkidNormalized },
            { QStringLiteral("iasKt"), flight.iasKt },
            { QStringLiteral("tasKt"), flight.tasKt },
            { QStringLiteral("aoaNormalized"), flight.aoaNormalized },
            { QStringLiteral("altitudeFt"), flight.altitudeFt },
            { QStringLiteral("verticalSpeedFpm"), flight.verticalSpeedFpm },
            { QStringLiteral("baroSettingInHg"), flight.baroSettingInHg },
        };
        return true;
    }

    if (frameId == PfdCan::kGnssNavId) {
        PfdCan::GnssNavPayload gnss {};
        if (!copyPayload(payload, &gnss)) {
            return false;
        }

        decodedFrame->type = MessageType::GnssNav;
        decodedFrame->fields = {
            { QStringLiteral("latitudeDeg"), static_cast<double>(gnss.latitudeE6) / 1000000.0 },
            { QStringLiteral("longitudeDeg"), static_cast<double>(gnss.longitudeE6) / 1000000.0 },
            { QStringLiteral("groundSpeedKt"), static_cast<double>(gnss.groundSpeedTenthKt) / 10.0 },
            { QStringLiteral("groundTrackDeg"), static_cast<double>(gnss.groundTrackTenthDeg) / 10.0 },
            { QStringLiteral("utcSecondsOfDay"), static_cast<quint32>(gnss.utcSecondsOfDay) },
            { QStringLiteral("fixType"), static_cast<int>(gnss.fixType) },
            { QStringLiteral("satellitesUsed"), static_cast<int>(gnss.satellitesUsed) },
            { QStringLiteral("horizontalAccuracyMeters"), static_cast<double>(gnss.horizontalAccuracyTenthMeters) / 10.0 },
        };
        return true;
    }

    if (frameId == PfdCan::kEisSummaryId) {
        PfdCan::EisSummaryPayload engine {};
        if (!copyPayload(payload, &engine)) {
            return false;
        }

        decodedFrame->type = MessageType::EisSummary;
        decodedFrame->fields = {
            { QStringLiteral("rpm"), static_cast<double>(engine.rpm) },
            { QStringLiteral("manifoldPressureInHg"), static_cast<double>(engine.manifoldPressureTenthInHg) / 10.0 },
            { QStringLiteral("oilPressurePsi"), static_cast<double>(engine.oilPressureTenthPsi) / 10.0 },
            { QStringLiteral("oilTemperatureF"), static_cast<double>(engine.oilTemperatureF) },
            { QStringLiteral("fuelFlowGph"), static_cast<double>(engine.fuelFlowTenthGph) / 10.0 },
            { QStringLiteral("busVoltage"), static_cast<double>(engine.busVoltageTenthV) / 10.0 },
            { QStringLiteral("busCurrentA"), static_cast<double>(engine.busCurrentTenthA) / 10.0 },
            { QStringLiteral("fuelRemainingGal"), static_cast<double>(engine.fuelRemainingTenthGal) / 10.0 },
            { QStringLiteral("fuelUsedGal"), static_cast<double>(engine.fuelUsedTenthGal) / 10.0 },
            { QStringLiteral("chtMaxF"), static_cast<double>(engine.chtMaxF) },
            { QStringLiteral("egtMaxF"), static_cast<double>(engine.egtMaxF) },
        };
        return true;
    }

    if (frameId == PfdCan::kEisCylinderId) {
        PfdCan::EisCylinderPayload cylinders {};
        if (!copyPayload(payload, &cylinders)) {
            return false;
        }

        QVariantList chtValues;
        QVariantList egtValues;
        const int cylinderCount = std::max(1, std::min(6, static_cast<int>(cylinders.cylinderCount)));
        for (int index = 0; index < cylinderCount; ++index) {
            chtValues.push_back(static_cast<int>(cylinders.chtF[index]));
            egtValues.push_back(static_cast<int>(cylinders.egtF[index]));
        }

        decodedFrame->type = MessageType::EisCylinders;
        decodedFrame->fields = {
            { QStringLiteral("cylinderCount"), cylinderCount },
            { QStringLiteral("chtValues"), chtValues },
            { QStringLiteral("egtValues"), egtValues },
        };
        return true;
    }

    if (frameId == PfdCan::kEisFuelTankId) {
        PfdCan::EisFuelTankPayload tank {};
        if (!copyPayload(payload, &tank)) {
            return false;
        }

        decodedFrame->type = MessageType::EisFuelTank;
        decodedFrame->fields = {
            { QStringLiteral("tankIndex"), static_cast<int>(tank.tankIndex) },
            { QStringLiteral("tankCount"), static_cast<int>(tank.tankCount) },
            { QStringLiteral("label"), trimAsciiField(tank.label, static_cast<int>(sizeof(tank.label))) },
            { QStringLiteral("quantityGal"), static_cast<double>(tank.quantityTenthGal) / 10.0 },
            { QStringLiteral("capacityGal"), static_cast<double>(tank.capacityTenthGal) / 10.0 },
            { QStringLiteral("lowCaution"), tank.lowCaution != 0 },
            { QStringLiteral("lowWarning"), tank.lowWarning != 0 },
        };
        return true;
    }

    if (frameId == PfdCan::kMagHeadingId) {
        PfdCan::MagHeadingPayload mag {};
        if (!copyPayload(payload, &mag)) {
            return false;
        }

        decodedFrame->type = MessageType::MagHeading;
        decodedFrame->fields = {
            { QStringLiteral("magneticHeadingDeg"), mag.magneticHeadingDeg },
            { QStringLiteral("trueHeadingDeg"), mag.trueHeadingDeg },
            { QStringLiteral("declinationDeg"), mag.declinationDeg },
            { QStringLiteral("calibrated"), mag.calibrated != 0 },
        };
        return true;
    }

    if (frameId == PfdCan::kAdsbTrafficId) {
        PfdCan::AdsbTrafficPayload traffic {};
        if (!copyPayload(payload, &traffic)) {
            return false;
        }

        decodedFrame->type = MessageType::AdsbTraffic;
        decodedFrame->fields = {
            { QStringLiteral("identifier"), trimAsciiField(traffic.identifier, static_cast<int>(sizeof(traffic.identifier))) },
            { QStringLiteral("latitudeDeg"), static_cast<double>(traffic.latitudeE6) / 1000000.0 },
            { QStringLiteral("longitudeDeg"), static_cast<double>(traffic.longitudeE6) / 1000000.0 },
            { QStringLiteral("altitudeFt"), static_cast<double>(traffic.altitudeFt) },
            { QStringLiteral("trackDeg"), static_cast<double>(traffic.trackTenthDeg) / 10.0 },
            { QStringLiteral("groundSpeedKt"), static_cast<double>(traffic.groundSpeedKt) },
            { QStringLiteral("verticalSpeedFpm"), static_cast<double>(traffic.verticalSpeedFpm) },
            { QStringLiteral("alertLevel"), static_cast<int>(traffic.alertLevel) },
            { QStringLiteral("ageSeconds"), static_cast<int>(traffic.ageSeconds) },
        };
        return true;
    }

    if (frameId == PfdCan::kAdsbWeatherHeaderId) {
        PfdCan::AdsbWeatherHeaderPayload header {};
        if (!copyPayload(payload, &header)) {
            return false;
        }

        decodedFrame->type = MessageType::AdsbWeatherHeader;
        decodedFrame->fields = {
            { QStringLiteral("productId"), static_cast<int>(header.productId) },
            { QStringLiteral("scaleFactor"), static_cast<int>(header.scaleFactor) },
            { QStringLiteral("blockNumber"), static_cast<quint32>(header.blockNumber) },
            { QStringLiteral("latNorthDeg"), static_cast<double>(header.latNorthHundredthsDeg) / 100.0 },
            { QStringLiteral("lonWestDeg"), static_cast<double>(header.lonWestHundredthsDeg) / 100.0 },
            { QStringLiteral("latHeightDeg"), static_cast<double>(header.latHeightHundredthsDeg) / 100.0 },
            { QStringLiteral("lonWidthDeg"), static_cast<double>(header.lonWidthHundredthsDeg) / 100.0 },
            { QStringLiteral("intensityCount"), static_cast<int>(header.intensityCount) },
        };
        return true;
    }

    if (frameId == PfdCan::kAdsbWeatherPayloadId) {
        PfdCan::AdsbWeatherPayload weatherChunk {};
        if (!copyPayload(payload, &weatherChunk)) {
            return false;
        }

        QVariantList intensity;
        intensity.reserve(static_cast<int>(sizeof(weatherChunk.intensity)));
        for (const std::uint8_t value : weatherChunk.intensity) {
            intensity.push_back(static_cast<int>(value));
        }

        decodedFrame->type = MessageType::AdsbWeatherPayload;
        decodedFrame->fields = {
            { QStringLiteral("blockNumber"), static_cast<quint32>(weatherChunk.blockNumber) },
            { QStringLiteral("offset"), static_cast<int>(weatherChunk.offset) },
            { QStringLiteral("intensity"), intensity },
        };
        return true;
    }

    return false;
}

QString moduleKeyForType(const int moduleType)
{
    switch (static_cast<PfdCan::ModuleType>(moduleType)) {
    case PfdCan::ModuleType::Adc:
    case PfdCan::ModuleType::Mag:
        return QStringLiteral("flight");
    case PfdCan::ModuleType::Gnss:
        return QStringLiteral("gps");
    case PfdCan::ModuleType::Eis:
        return QStringLiteral("engine");
    case PfdCan::ModuleType::Adsb:
        return QStringLiteral("traffic");
    case PfdCan::ModuleType::Unknown:
    default:
        return QStringLiteral("unknown");
    }
}

QString moduleNameForType(const int moduleType)
{
    switch (static_cast<PfdCan::ModuleType>(moduleType)) {
    case PfdCan::ModuleType::Adc:
        return QStringLiteral("ADC / AHRS");
    case PfdCan::ModuleType::Gnss:
        return QStringLiteral("GPS / GNSS");
    case PfdCan::ModuleType::Eis:
        return QStringLiteral("EIS");
    case PfdCan::ModuleType::Mag:
        return QStringLiteral("MAG");
    case PfdCan::ModuleType::Adsb:
        return QStringLiteral("ADS-B In");
    case PfdCan::ModuleType::Unknown:
    default:
        return QStringLiteral("Unknown");
    }
}

QString modeLabel(const int mode)
{
    switch (static_cast<PfdCan::ModuleMode>(mode)) {
    case PfdCan::ModuleMode::Offline:
        return QStringLiteral("offline");
    case PfdCan::ModuleMode::Boot:
        return QStringLiteral("boot");
    case PfdCan::ModuleMode::Demo:
        return QStringLiteral("demo");
    case PfdCan::ModuleMode::Live:
        return QStringLiteral("live");
    default:
        return QStringLiteral("unknown");
    }
}

bool healthIsHealthy(const int health)
{
    return static_cast<PfdCan::HealthState>(health) == PfdCan::HealthState::Healthy;
}

} // namespace ModuleProtocol
