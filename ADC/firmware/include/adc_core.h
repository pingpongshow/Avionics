#pragma once

#include <cmath>

namespace adc {

struct PressureSample {
    double pitotMinusStaticPa = 0.0;
    double staticPressurePa = 101325.0;
    double aoaDifferentialPa = 0.0;
    double oatC = 15.0;
    double deltaTimeSeconds = 0.02;
};

struct ImuSample {
    double accelX = 0.0;
    double accelY = 0.0;
    double accelZ = 1.0;
    double gyroRollRateDegPerSec = 0.0;
    double gyroPitchRateDegPerSec = 0.0;
    double deltaTimeSeconds = 0.02;
};

struct AdcResult {
    double iasKt = 0.0;
    double pressureAltitudeFt = 0.0;
    double verticalSpeedFpm = 0.0;
    double aoaNormalized = 0.0;
    double pitchDeg = 0.0;
    double rollDeg = 0.0;
};

class AdcCore {
public:
    void setBaroReferenceInHg(double inHg);
    AdcResult update(const PressureSample &pressure, const ImuSample &imu);

private:
    double m_baroReferenceInHg = 29.92;
    double m_lastAltitudeFt = 0.0;
    double m_pitchDeg = 0.0;
    double m_rollDeg = 0.0;
};

} // namespace adc

