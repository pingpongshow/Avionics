#include "eis_core.h"

#include <iostream>

int main()
{
    eis::EngineMonitor monitor;

    eis::EngineSample sample {};
    sample.rpm = 2435.0;
    sample.manifoldPressureInHg = 26.7;
    sample.oilPressurePsi = 71.0;
    sample.oilTemperatureF = 206.0;
    sample.fuelFlowGph = 10.8;
    sample.busVoltage = 27.4;
    sample.chtF = { 351.0, 360.0, 355.0, 347.0 };
    sample.egtF = { 1378.0, 1399.0, 1404.0, 1381.0 };
    sample.tanks = {
        { "L", 12.6, 18.0 },
        { "R", 11.9, 18.0 },
    };

    const eis::EngineSnapshot snapshot = monitor.update(sample, 1.0 / 3600.0);
    std::cout << "RPM: " << snapshot.rpm.value << "\n";
    std::cout << "Fuel Remaining: " << snapshot.fuelRemainingGal << " gal\n";
    std::cout << "Warnings: " << snapshot.warningMessages.size() << "\n";
    std::cout << "Cautions: " << snapshot.cautionMessages.size() << "\n";
    return 0;
}

