#include "eis_core.h"

#include <algorithm>

namespace eis {

EngineMonitor::EngineMonitor(EngineConfig config)
    : m_config(config)
{
}

ChannelState EngineMonitor::evaluate(const double value, const RangeBand &band)
{
    ChannelState state {};
    state.value = value;

    if ((band.redMin > 0.0 && value <= band.redMin) || (band.redMax > 0.0 && value >= band.redMax)) {
        state.zone = Zone::Red;
    } else if ((band.yellowMin > 0.0 && value <= band.yellowMin) || (band.yellowMax > 0.0 && value >= band.yellowMax)) {
        state.zone = Zone::Yellow;
    } else if (value >= band.greenMin && value <= band.greenMax) {
        state.zone = Zone::Green;
    } else {
        state.zone = Zone::White;
    }

    return state;
}

EngineSnapshot EngineMonitor::update(const EngineSample &sample, const double deltaTimeHours)
{
    EngineSnapshot snapshot {};
    snapshot.rpm = evaluate(sample.rpm, m_config.rpmBand);
    snapshot.manifoldPressure = evaluate(sample.manifoldPressureInHg, m_config.manifoldBand);
    snapshot.oilPressure = evaluate(sample.oilPressurePsi, m_config.oilPressureBand);
    snapshot.oilTemperature = evaluate(sample.oilTemperatureF, m_config.oilTemperatureBand);
    snapshot.fuelFlow = evaluate(sample.fuelFlowGph, m_config.fuelFlowBand);
    snapshot.busVoltage = evaluate(sample.busVoltage, m_config.busVoltageBand);

    m_fuelUsedGal += std::max(0.0, sample.fuelFlowGph) * std::max(0.0, deltaTimeHours);

    double totalFuelRemaining = 0.0;
    for (const TankState &tank : sample.tanks) {
        totalFuelRemaining += std::max(0.0, tank.quantityGal);
    }

    snapshot.fuelRemainingGal = totalFuelRemaining;
    snapshot.fuelUsedGal = m_fuelUsedGal;
    snapshot.fuelEnduranceHr = sample.fuelFlowGph > 0.01 ? totalFuelRemaining / sample.fuelFlowGph : 0.0;
    snapshot.maxChtF = *std::max_element(sample.chtF.begin(), sample.chtF.end());
    snapshot.maxEgtF = *std::max_element(sample.egtF.begin(), sample.egtF.end());

    if (snapshot.oilPressure.zone == Zone::Yellow) {
        snapshot.cautionMessages.emplace_back("Oil pressure outside green range");
    } else if (snapshot.oilPressure.zone == Zone::Red) {
        snapshot.warningMessages.emplace_back("Oil pressure in red range");
    }

    if (snapshot.oilTemperature.zone == Zone::Yellow) {
        snapshot.cautionMessages.emplace_back("Oil temperature outside green range");
    } else if (snapshot.oilTemperature.zone == Zone::Red) {
        snapshot.warningMessages.emplace_back("Oil temperature in red range");
    }

    if (snapshot.maxChtF >= m_config.chtWarningF) {
        snapshot.warningMessages.emplace_back("CHT exceeds warning limit");
    } else if (snapshot.maxChtF >= m_config.chtCautionF) {
        snapshot.cautionMessages.emplace_back("CHT exceeds caution limit");
    }

    if (snapshot.maxEgtF >= m_config.egtWarningF) {
        snapshot.warningMessages.emplace_back("EGT exceeds warning limit");
    } else if (snapshot.maxEgtF >= m_config.egtCautionF) {
        snapshot.cautionMessages.emplace_back("EGT exceeds caution limit");
    }

    return snapshot;
}

} // namespace eis

