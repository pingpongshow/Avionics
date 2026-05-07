#pragma once

#include <cstdint>

namespace PfdCan {

enum class ModuleType : std::uint8_t {
    Unknown = 0,
    Adc = 1,
    Gnss = 2,
    Eis = 3,
    Mag = 4,
    Adsb = 5,
};

enum class ModuleMode : std::uint8_t {
    Offline = 0,
    Boot = 1,
    Demo = 2,
    Live = 3,
};

enum class HealthState : std::uint8_t {
    Unknown = 0,
    Healthy = 1,
    Degraded = 2,
    Failed = 3,
};

enum CapabilityFlag : std::uint32_t {
    CapabilityAhrs = 1u << 0,
    CapabilityAirData = 1u << 1,
    CapabilityGps = 1u << 2,
    CapabilityEngine = 1u << 3,
    CapabilityTraffic = 1u << 4,
    CapabilityWeather = 1u << 5,
    CapabilityMagnetometer = 1u << 6,
    CapabilityFuelTanks = 1u << 7,
    CapabilityAoa = 1u << 8,
};

constexpr std::uint32_t kHeartbeatBaseId = 0x100;
constexpr std::uint32_t kAdcFlightId = 0x110;
constexpr std::uint32_t kGnssNavId = 0x150;
constexpr std::uint32_t kEisSummaryId = 0x210;
constexpr std::uint32_t kEisCylinderId = 0x211;
constexpr std::uint32_t kEisFuelTankId = 0x212;
constexpr std::uint32_t kMagHeadingId = 0x310;
constexpr std::uint32_t kAdsbTrafficId = 0x510;
constexpr std::uint32_t kAdsbWeatherHeaderId = 0x520;
constexpr std::uint32_t kAdsbWeatherPayloadId = 0x521;

constexpr std::uint32_t heartbeatId(const ModuleType moduleType)
{
    return kHeartbeatBaseId + static_cast<std::uint32_t>(moduleType);
}

#pragma pack(push, 1)

struct HeartbeatPayload {
    std::uint8_t moduleType;
    std::uint8_t instanceId;
    std::uint8_t health;
    std::uint8_t mode;
    std::uint32_t capabilityFlags;
    std::uint32_t counterA;
    std::uint32_t counterB;
};

struct AdcFlightPayload {
    float pitchDeg;
    float rollDeg;
    float headingDeg;
    float turnRateDegPerSec;
    float slipSkidNormalized;
    float iasKt;
    float tasKt;
    float aoaNormalized;
    float altitudeFt;
    float verticalSpeedFpm;
    float baroSettingInHg;
};

struct GnssNavPayload {
    std::int32_t latitudeE6;
    std::int32_t longitudeE6;
    std::uint16_t groundSpeedTenthKt;
    std::uint16_t groundTrackTenthDeg;
    std::uint32_t utcSecondsOfDay;
    std::uint8_t fixType;
    std::uint8_t satellitesUsed;
    std::uint16_t horizontalAccuracyTenthMeters;
};

struct EisSummaryPayload {
    std::uint16_t rpm;
    std::uint16_t manifoldPressureTenthInHg;
    std::uint16_t oilPressureTenthPsi;
    std::uint16_t oilTemperatureF;
    std::uint16_t fuelFlowTenthGph;
    std::uint16_t busVoltageTenthV;
    std::int16_t busCurrentTenthA;
    std::uint16_t fuelRemainingTenthGal;
    std::uint16_t fuelUsedTenthGal;
    std::uint16_t chtMaxF;
    std::uint16_t egtMaxF;
};

struct EisCylinderPayload {
    std::uint16_t chtF[6];
    std::uint16_t egtF[6];
    std::uint8_t cylinderCount;
    std::uint8_t reserved[3];
};

struct EisFuelTankPayload {
    std::uint8_t tankIndex;
    std::uint8_t tankCount;
    std::uint16_t quantityTenthGal;
    std::uint16_t capacityTenthGal;
    std::uint8_t lowCaution;
    std::uint8_t lowWarning;
    char label[8];
};

struct MagHeadingPayload {
    float magneticHeadingDeg;
    float trueHeadingDeg;
    float declinationDeg;
    std::uint8_t calibrated;
    std::uint8_t reserved[3];
};

struct AdsbTrafficPayload {
    char identifier[8];
    std::int32_t latitudeE6;
    std::int32_t longitudeE6;
    std::int32_t altitudeFt;
    std::uint16_t trackTenthDeg;
    std::uint16_t groundSpeedKt;
    std::int16_t verticalSpeedFpm;
    std::uint8_t alertLevel;
    std::uint8_t ageSeconds;
};

struct AdsbWeatherHeaderPayload {
    std::uint8_t productId;
    std::uint8_t scaleFactor;
    std::uint32_t blockNumber;
    std::int16_t latNorthHundredthsDeg;
    std::int16_t lonWestHundredthsDeg;
    std::int16_t latHeightHundredthsDeg;
    std::int16_t lonWidthHundredthsDeg;
    std::uint16_t intensityCount;
};

struct AdsbWeatherPayload {
    std::uint32_t blockNumber;
    std::uint16_t offset;
    std::uint8_t intensity[58];
};

#pragma pack(pop)

static_assert(sizeof(HeartbeatPayload) == 16, "Unexpected HeartbeatPayload size");
static_assert(sizeof(AdcFlightPayload) == 44, "Unexpected AdcFlightPayload size");
static_assert(sizeof(GnssNavPayload) == 20, "Unexpected GnssNavPayload size");
static_assert(sizeof(EisSummaryPayload) == 22, "Unexpected EisSummaryPayload size");
static_assert(sizeof(EisCylinderPayload) == 28, "Unexpected EisCylinderPayload size");
static_assert(sizeof(EisFuelTankPayload) == 16, "Unexpected EisFuelTankPayload size");
static_assert(sizeof(MagHeadingPayload) == 16, "Unexpected MagHeadingPayload size");
static_assert(sizeof(AdsbTrafficPayload) == 28, "Unexpected AdsbTrafficPayload size");
static_assert(sizeof(AdsbWeatherHeaderPayload) == 16, "Unexpected AdsbWeatherHeaderPayload size");
static_assert(sizeof(AdsbWeatherPayload) == 64, "Unexpected AdsbWeatherPayload size");

} // namespace PfdCan
