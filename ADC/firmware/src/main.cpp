#include "adc_core.h"

#include <iostream>

int main()
{
    adc::AdcCore core;

    adc::PressureSample pressure {};
    pressure.pitotMinusStaticPa = 850.0;
    pressure.staticPressurePa = 81750.0;
    pressure.aoaDifferentialPa = 22.0;
    pressure.deltaTimeSeconds = 0.02;

    adc::ImuSample imu {};
    imu.accelX = 0.03;
    imu.accelY = 0.10;
    imu.accelZ = 0.99;
    imu.gyroRollRateDegPerSec = 1.8;
    imu.gyroPitchRateDegPerSec = 0.7;
    imu.deltaTimeSeconds = 0.02;

    const adc::AdcResult result = core.update(pressure, imu);
    std::cout << "IAS: " << result.iasKt << " kt\n";
    std::cout << "Altitude: " << result.pressureAltitudeFt << " ft\n";
    std::cout << "Pitch/Roll: " << result.pitchDeg << " / " << result.rollDeg << "\n";
    return 0;
}

