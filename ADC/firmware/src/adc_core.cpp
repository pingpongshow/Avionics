#include "adc_core.h"

#include <algorithm>

namespace adc {

namespace {

constexpr double kSeaLevelDensityKgM3 = 1.225;
constexpr double kPaToKt = 1.94384;
constexpr double kComplementaryAlpha = 0.98;
constexpr double kPi = 3.14159265358979323846;

}

void AdcCore::setBaroReferenceInHg(const double inHg)
{
    m_baroReferenceInHg = std::clamp(inHg, 28.00, 31.50);
}

AdcResult AdcCore::update(const PressureSample &pressure, const ImuSample &imu)
{
    AdcResult result {};

    const double qPa = std::max(0.0, pressure.pitotMinusStaticPa);
    const double iasMetersPerSecond = std::sqrt((2.0 * qPa) / kSeaLevelDensityKgM3);
    result.iasKt = iasMetersPerSecond * kPaToKt;

    const double pressureRatio = std::max(0.01, pressure.staticPressurePa / 101325.0);
    result.pressureAltitudeFt = (1.0 - std::pow(pressureRatio, 0.190284)) * 145366.45
        + ((29.92 - m_baroReferenceInHg) * 1000.0);

    const double deltaAltitudeFt = result.pressureAltitudeFt - m_lastAltitudeFt;
    result.verticalSpeedFpm = pressure.deltaTimeSeconds > 0.001
        ? (deltaAltitudeFt / pressure.deltaTimeSeconds) * 60.0
        : 0.0;
    m_lastAltitudeFt = result.pressureAltitudeFt;

    result.aoaNormalized = std::clamp(pressure.aoaDifferentialPa / 120.0, -1.0, 1.0);

    const double accelRollDeg = std::atan2(imu.accelY, imu.accelZ) * 180.0 / kPi;
    const double accelPitchDeg = std::atan2(-imu.accelX, std::sqrt(imu.accelY * imu.accelY + imu.accelZ * imu.accelZ)) * 180.0 / kPi;

    m_rollDeg = (kComplementaryAlpha * (m_rollDeg + imu.gyroRollRateDegPerSec * imu.deltaTimeSeconds))
        + ((1.0 - kComplementaryAlpha) * accelRollDeg);
    m_pitchDeg = (kComplementaryAlpha * (m_pitchDeg + imu.gyroPitchRateDegPerSec * imu.deltaTimeSeconds))
        + ((1.0 - kComplementaryAlpha) * accelPitchDeg);

    result.rollDeg = m_rollDeg;
    result.pitchDeg = m_pitchDeg;
    return result;
}

} // namespace adc
