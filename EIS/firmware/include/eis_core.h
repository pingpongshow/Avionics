#pragma once

#include <array>
#include <string>
#include <vector>

namespace eis {

enum class Zone {
    White,
    Green,
    Yellow,
    Red
};

struct RangeBand {
    double whiteMin = 0.0;
    double whiteMax = 0.0;
    double greenMin = 0.0;
    double greenMax = 0.0;
    double yellowMin = 0.0;
    double yellowMax = 0.0;
    double redMin = 0.0;
    double redMax = 0.0;
};

struct TankState {
    std::string label;
    double quantityGal = 0.0;
    double capacityGal = 0.0;
};

struct EngineSample {
    double rpm = 0.0;
    double manifoldPressureInHg = 0.0;
    double oilPressurePsi = 0.0;
    double oilTemperatureF = 0.0;
    double fuelFlowGph = 0.0;
    double busVoltage = 0.0;
    double busCurrentA = 0.0;
    std::array<double, 4> chtF {};
    std::array<double, 4> egtF {};
    std::vector<TankState> tanks;
};

struct EngineConfig {
    RangeBand rpmBand { 0.0, 0.0, 700.0, 2450.0, 2450.0, 2550.0, 0.0, 2700.0 };
    RangeBand manifoldBand { 0.0, 0.0, 10.0, 29.0, 29.0, 31.0, 0.0, 35.0 };
    RangeBand oilPressureBand { 0.0, 0.0, 55.0, 95.0, 25.0, 110.0, 10.0, 120.0 };
    RangeBand oilTemperatureBand { 0.0, 0.0, 120.0, 230.0, 230.0, 245.0, 0.0, 260.0 };
    RangeBand fuelFlowBand { 0.0, 0.0, 0.0, 18.5, 18.5, 22.0, 0.0, 24.0 };
    RangeBand busVoltageBand { 0.0, 0.0, 24.0, 29.5, 23.0, 30.0, 22.0, 32.0 };
    double chtCautionF = 420.0;
    double chtWarningF = 460.0;
    double egtCautionF = 1525.0;
    double egtWarningF = 1650.0;
};

struct ChannelState {
    double value = 0.0;
    Zone zone = Zone::White;
};

struct EngineSnapshot {
    ChannelState rpm;
    ChannelState manifoldPressure;
    ChannelState oilPressure;
    ChannelState oilTemperature;
    ChannelState fuelFlow;
    ChannelState busVoltage;
    double fuelRemainingGal = 0.0;
    double fuelUsedGal = 0.0;
    double fuelEnduranceHr = 0.0;
    double maxChtF = 0.0;
    double maxEgtF = 0.0;
    std::vector<std::string> cautionMessages;
    std::vector<std::string> warningMessages;
};

class EngineMonitor {
public:
    explicit EngineMonitor(EngineConfig config = {});

    EngineSnapshot update(const EngineSample &sample, double deltaTimeHours);

private:
    static ChannelState evaluate(double value, const RangeBand &band);

    EngineConfig m_config;
    double m_fuelUsedGal = 0.0;
};

} // namespace eis

