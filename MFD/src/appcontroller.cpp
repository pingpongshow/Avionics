#include "appcontroller.h"

#include "navdatamanager.h"

#include <QDate>
#include <QDateTime>
#include <QVariantMap>
#include <QtGlobal>
#include <QTimeZone>

#include <algorithm>
#include <cmath>

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kEarthRadiusNm = 3440.065;

double clampHeading(double heading)
{
    const double normalized = std::fmod(heading, 360.0);
    return normalized < 0.0 ? normalized + 360.0 : normalized;
}

double toRadians(double degrees)
{
    return degrees * kPi / 180.0;
}

double toDegrees(double radians)
{
    return radians * 180.0 / kPi;
}

double signedHeadingErrorDeg(double targetDeg, double currentDeg)
{
    double delta = std::fmod(targetDeg - currentDeg + 540.0, 360.0) - 180.0;
    if (delta < -180.0) {
        delta += 360.0;
    }
    return delta;
}

double haversineDistanceNm(double latitude1Deg, double longitude1Deg, double latitude2Deg, double longitude2Deg)
{
    const double latitude1Rad = toRadians(latitude1Deg);
    const double longitude1Rad = toRadians(longitude1Deg);
    const double latitude2Rad = toRadians(latitude2Deg);
    const double longitude2Rad = toRadians(longitude2Deg);
    const double deltaLatitude = latitude2Rad - latitude1Rad;
    const double deltaLongitude = longitude2Rad - longitude1Rad;
    const double a = std::pow(std::sin(deltaLatitude / 2.0), 2.0)
        + std::cos(latitude1Rad) * std::cos(latitude2Rad) * std::pow(std::sin(deltaLongitude / 2.0), 2.0);
    const double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(std::max(0.0, 1.0 - a)));
    return kEarthRadiusNm * c;
}

double initialBearingDeg(double latitude1Deg, double longitude1Deg, double latitude2Deg, double longitude2Deg)
{
    const double latitude1Rad = toRadians(latitude1Deg);
    const double latitude2Rad = toRadians(latitude2Deg);
    const double deltaLongitude = toRadians(longitude2Deg - longitude1Deg);
    const double y = std::sin(deltaLongitude) * std::cos(latitude2Rad);
    const double x = std::cos(latitude1Rad) * std::sin(latitude2Rad)
        - std::sin(latitude1Rad) * std::cos(latitude2Rad) * std::cos(deltaLongitude);
    return clampHeading(toDegrees(std::atan2(y, x)));
}

QPair<double, double> offsetCoordinate(double latitudeDeg, double longitudeDeg, double bearingDeg, double distanceNm)
{
    const double angularDistance = distanceNm / kEarthRadiusNm;
    const double bearingRad = toRadians(bearingDeg);
    const double latitude1Rad = toRadians(latitudeDeg);
    const double longitude1Rad = toRadians(longitudeDeg);

    const double latitude2Rad = std::asin(std::sin(latitude1Rad) * std::cos(angularDistance)
        + std::cos(latitude1Rad) * std::sin(angularDistance) * std::cos(bearingRad));
    const double longitude2Rad = longitude1Rad + std::atan2(
        std::sin(bearingRad) * std::sin(angularDistance) * std::cos(latitude1Rad),
        std::cos(angularDistance) - std::sin(latitude1Rad) * std::sin(latitude2Rad));
    return { toDegrees(latitude2Rad), toDegrees(longitude2Rad) };
}

QString scenarioLabel(const QString &scenario)
{
    QString label = scenario;
    label.replace('_', ' ');
    if (!label.isEmpty()) {
        label[0] = label[0].toUpper();
    }
    return label;
}

QString mapBaseSourceLabelForKey(const QString &mapBaseSource)
{
    if (mapBaseSource == QStringLiteral("faa_vfr")) {
        return QStringLiteral("FAA VFR");
    }
    if (mapBaseSource == QStringLiteral("openflightmaps")) {
        return QStringLiteral("OpenFlightMaps");
    }
    return QStringLiteral("Demo Preview");
}

QString formatHoursMinutes(double hours)
{
    const int totalMinutes = std::max(0, static_cast<int>(std::round(hours * 60.0)));
    const int hh = totalMinutes / 60;
    const int mm = totalMinutes % 60;
    return QStringLiteral("%1:%2")
        .arg(hh, 2, 10, QLatin1Char('0'))
        .arg(mm, 2, 10, QLatin1Char('0'));
}

QStringList defaultFuelTankLabelsForCount(int count)
{
    switch (count) {
    case 1:
        return { QStringLiteral("CTR") };
    case 2:
        return { QStringLiteral("L"), QStringLiteral("R") };
    case 3:
        return { QStringLiteral("L"), QStringLiteral("CTR"), QStringLiteral("R") };
    default:
        return {
            QStringLiteral("L AUX"),
            QStringLiteral("L"),
            QStringLiteral("R"),
            QStringLiteral("R AUX")
        };
    }
}

QVariantList defaultFuelTankCapacitiesForCount(int count)
{
    QVariantList capacities;
    switch (count) {
    case 1:
        capacities = { 36.0 };
        break;
    case 2:
        capacities = { 18.0, 18.0 };
        break;
    case 3:
        capacities = { 12.0, 12.0, 12.0 };
        break;
    default:
        capacities = { 9.0, 9.0, 9.0, 9.0 };
        break;
    }
    return capacities;
}

} // namespace

AppController::AppController(QObject *parent)
    : QObject(parent)
{
    seedModules();
    configureFuelTanks(2);
    applyScenarioPreset(m_scenario);
    syncAvailablePages();

    connect(&m_demoTimer, &QTimer::timeout, this, &AppController::updateDemoFrame);
    m_demoTimer.setInterval(100);
    m_demoTimer.start();
    updateDemoFrame();
}

void AppController::setNavDataManager(NavDataManager *navDataManager)
{
    if (m_navDataManager == navDataManager) {
        return;
    }

    m_navDataManager = navDataManager;
    if (m_navDataManager != nullptr) {
        connect(m_navDataManager, &NavDataManager::userWaypointsChanged, this, [this]() {
            updateDemoFrame();
        });
    }
    updateDemoFrame();
}

QString AppController::currentPage() const
{
    return m_currentPage;
}

void AppController::setCurrentPage(const QString &page)
{
    if (!m_availablePages.contains(page) || m_currentPage == page) {
        return;
    }

    m_currentPage = page;
    emit currentPageChanged();
}

QStringList AppController::availablePages() const
{
    return m_availablePages;
}

QStringList AppController::displayClasses() const
{
    return {
        QStringLiteral("primary"),
        QStringLiteral("compact"),
        QStringLiteral("standby"),
    };
}

QString AppController::displayClass() const
{
    return m_displayClass;
}

void AppController::setDisplayClass(const QString &displayClass)
{
    if (!displayClasses().contains(displayClass) || m_displayClass == displayClass) {
        return;
    }

    m_displayClass = displayClass;
    emit displayClassChanged();
}

QStringList AppController::mapBaseSources() const
{
    return {
        QStringLiteral("faa_vfr"),
        QStringLiteral("openflightmaps"),
        QStringLiteral("demo"),
    };
}

QString AppController::mapBaseSource() const
{
    return m_mapBaseSource;
}

void AppController::setMapBaseSource(const QString &mapBaseSource)
{
    if (!mapBaseSources().contains(mapBaseSource) || m_mapBaseSource == mapBaseSource) {
        return;
    }

    m_mapBaseSource = mapBaseSource;
    emit mapConfigChanged();
    refreshAlerts();
}

QString AppController::mapBaseSourceLabel() const
{
    return mapBaseSourceLabelForKey(m_mapBaseSource);
}

bool AppController::openAipOverlayEnabled() const
{
    return m_openAipOverlayEnabled;
}

void AppController::setOpenAipOverlayEnabled(bool enabled)
{
    if (m_openAipOverlayEnabled == enabled) {
        return;
    }

    m_openAipOverlayEnabled = enabled;
    emit mapConfigChanged();
}

bool AppController::weatherOverlayEnabled() const
{
    return m_weatherOverlayEnabled;
}

void AppController::setWeatherOverlayEnabled(bool enabled)
{
    if (m_weatherOverlayEnabled == enabled) {
        return;
    }

    m_weatherOverlayEnabled = enabled;
    emit mapConfigChanged();
}

double AppController::weatherOverlayOpacity() const
{
    return m_weatherOverlayOpacity;
}

void AppController::setWeatherOverlayOpacity(double opacity)
{
    const double clampedOpacity = std::clamp(opacity, 0.0, 1.0);
    if (qFuzzyCompare(m_weatherOverlayOpacity + 1.0, clampedOpacity + 1.0)) {
        return;
    }

    m_weatherOverlayOpacity = clampedOpacity;
    emit mapConfigChanged();
}

double AppController::mapZoomFactor() const
{
    return m_mapZoomFactor;
}

void AppController::setMapZoomFactor(double zoomFactor)
{
    const double clampedZoom = std::clamp(zoomFactor, 0.6, 3.0);
    if (qFuzzyCompare(m_mapZoomFactor + 1.0, clampedZoom + 1.0)) {
        return;
    }

    m_mapZoomFactor = clampedZoom;
    emit mapConfigChanged();
}

QString AppController::mapStatusSummary() const
{
    return QStringLiteral("Base chart: %1  |  openAIP: %2  |  Weather: %3  |  Zoom: %4x")
        .arg(mapBaseSourceLabel())
        .arg(m_openAipOverlayEnabled ? QStringLiteral("enabled")
                                     : QStringLiteral("disabled"))
        .arg(m_weatherOverlayEnabled ? QStringLiteral("enabled")
                                     : QStringLiteral("disabled"))
        .arg(QString::number(m_mapZoomFactor, 'f', 1));
}

QString AppController::tailNumber() const
{
    return m_tailNumber;
}

void AppController::setTailNumber(const QString &tailNumber)
{
    const QString normalized = tailNumber.trimmed().toUpper();
    if (normalized.isEmpty() || m_tailNumber == normalized) {
        return;
    }

    m_tailNumber = normalized;
    emit configurationChanged();
}

bool AppController::setupAccessAllowed() const
{
    return m_demoMode || m_rpm < 300.0;
}

QStringList AppController::scenarios() const
{
    return {
        QStringLiteral("gps_only"),
        QStringLiteral("takeoff"),
        QStringLiteral("cruise"),
        QStringLiteral("approach"),
        QStringLiteral("engine_alert"),
        QStringLiteral("traffic"),
        QStringLiteral("module_failure"),
    };
}

QString AppController::scenario() const
{
    return m_scenario;
}

void AppController::setScenario(const QString &scenario)
{
    if (!scenarios().contains(scenario) || m_scenario == scenario) {
        return;
    }

    m_scenario = scenario;
    emit scenarioChanged();
    applyScenarioPreset(scenario);
    syncAvailablePages();
    updateDemoFrame();
}

bool AppController::demoMode() const
{
    return m_demoMode;
}

void AppController::setDemoMode(bool enabled)
{
    if (m_demoMode == enabled) {
        return;
    }

    m_demoMode = enabled;
    emit demoModeChanged();
    emit configurationChanged();
    refreshAlerts();
}

QAbstractListModel *AppController::moduleModel()
{
    return &m_moduleModel;
}

double AppController::pitchDeg() const
{
    return m_pitchDeg;
}

double AppController::rollDeg() const
{
    return m_rollDeg;
}

double AppController::headingDeg() const
{
    return m_headingDeg;
}

double AppController::turnRateDegPerSec() const
{
    return m_turnRateDegPerSec;
}

double AppController::slipSkidNormalized() const
{
    return m_slipSkidNormalized;
}

double AppController::iasKt() const
{
    return m_iasKt;
}

double AppController::airspeedTrendKt() const
{
    return m_airspeedTrendKt;
}

double AppController::tasKt() const
{
    return m_tasKt;
}

double AppController::aoaNormalized() const
{
    return m_aoaNormalized;
}

double AppController::altitudeFt() const
{
    return m_altitudeFt;
}

double AppController::verticalSpeedFpm() const
{
    return m_verticalSpeedFpm;
}

double AppController::baroSettingInHg() const
{
    return m_baroSettingInHg;
}

void AppController::setBaroSettingInHg(double baroSettingInHg)
{
    const double clampedBaro = std::clamp(baroSettingInHg, 27.50, 31.50);
    if (qFuzzyCompare(m_baroSettingInHg + 1.0, clampedBaro + 1.0)) {
        return;
    }

    m_baroSettingInHg = clampedBaro;
    emit flightDataChanged();
}

double AppController::selectedAltitudeFt() const
{
    return m_selectedAltitudeFt;
}

void AppController::setSelectedAltitudeFt(double selectedAltitudeFt)
{
    const double clampedAltitude = std::clamp(std::round(selectedAltitudeFt / 10.0) * 10.0, 0.0, 30000.0);
    if (qFuzzyCompare(m_selectedAltitudeFt + 1.0, clampedAltitude + 1.0)) {
        return;
    }

    m_selectedAltitudeFt = clampedAltitude;
    emit flightDataChanged();
}

double AppController::latitudeDeg() const
{
    return m_latitudeDeg;
}

double AppController::longitudeDeg() const
{
    return m_longitudeDeg;
}

double AppController::groundSpeedKt() const
{
    return m_groundSpeedKt;
}

double AppController::groundTrackDeg() const
{
    return m_groundTrackDeg;
}

QString AppController::utcTime() const
{
    return m_utcTime;
}

QString AppController::activeWaypointLabel() const
{
    return m_activeWaypointLabel;
}

double AppController::waypointBearingDeg() const
{
    return m_waypointBearingDeg;
}

double AppController::waypointDistanceNm() const
{
    return m_waypointDistanceNm;
}

QString AppController::waypointEteLabel() const
{
    return m_waypointEteLabel;
}

QVariantList AppController::flightPlanLegs() const
{
    return m_flightPlanLegs;
}

double AppController::rpm() const
{
    return m_rpm;
}

double AppController::manifoldPressureInHg() const
{
    return m_manifoldPressureInHg;
}

double AppController::oilPressurePsi() const
{
    return m_oilPressurePsi;
}

double AppController::oilTemperatureF() const
{
    return m_oilTemperatureF;
}

double AppController::fuelFlowGph() const
{
    return m_fuelFlowGph;
}

double AppController::fuelQuantityPercent() const
{
    return m_fuelQuantityPercent;
}

int AppController::fuelTankCount() const
{
    return m_fuelTankCount;
}

void AppController::setFuelTankCount(int count)
{
    const int clampedCount = std::clamp(count, 1, 4);
    if (m_fuelTankCount == clampedCount) {
        return;
    }

    configureFuelTanks(clampedCount);
    rebuildFuelTankStates();
    emit configurationChanged();
    emit engineDataChanged();
    refreshAlerts();
}

QStringList AppController::fuelTankLabels() const
{
    return m_fuelTankLabels;
}

QVariantList AppController::fuelTankStates() const
{
    return m_fuelTankStates;
}

double AppController::fuelRemainingGal() const
{
    return m_fuelRemainingGal;
}

double AppController::fuelUsedGal() const
{
    return m_fuelUsedGal;
}

double AppController::fuelEnduranceHours() const
{
    return m_fuelEnduranceHours;
}

double AppController::fuelRangeNm() const
{
    return m_fuelRangeNm;
}

double AppController::busVoltage() const
{
    return m_busVoltage;
}

double AppController::chtMaxF() const
{
    return m_chtMaxF;
}

double AppController::egtMaxF() const
{
    return m_egtMaxF;
}

QVariantList AppController::chtValues() const
{
    return m_chtValues;
}

QVariantList AppController::egtValues() const
{
    return m_egtValues;
}

bool AppController::autopilotMaster() const
{
    return m_autopilotMaster;
}

void AppController::setAutopilotMaster(bool enabled)
{
    if (m_autopilotMaster == enabled) {
        return;
    }
    m_autopilotMaster = enabled;
    emit autopilotChanged();
}

bool AppController::flightDirectorEnabled() const
{
    return m_flightDirectorEnabled;
}

void AppController::setFlightDirectorEnabled(bool enabled)
{
    if (m_flightDirectorEnabled == enabled) {
        return;
    }
    m_flightDirectorEnabled = enabled;
    emit autopilotChanged();
}

bool AppController::levelModeEnabled() const
{
    return m_levelModeEnabled;
}

void AppController::setLevelModeEnabled(bool enabled)
{
    if (m_levelModeEnabled == enabled) {
        return;
    }
    m_levelModeEnabled = enabled;
    emit autopilotChanged();
}

QString AppController::autopilotLateralMode() const
{
    return m_autopilotLateralMode;
}

void AppController::setAutopilotLateralMode(const QString &mode)
{
    const QString normalized = mode.trimmed().toUpper();
    if (normalized.isEmpty() || m_autopilotLateralMode == normalized) {
        return;
    }
    m_autopilotLateralMode = normalized;
    emit autopilotChanged();
}

QString AppController::autopilotVerticalMode() const
{
    return m_autopilotVerticalMode;
}

void AppController::setAutopilotVerticalMode(const QString &mode)
{
    const QString normalized = mode.trimmed().toUpper();
    if (normalized.isEmpty() || m_autopilotVerticalMode == normalized) {
        return;
    }
    m_autopilotVerticalMode = normalized;
    emit autopilotChanged();
}

bool AppController::gpsModuleAvailable() const
{
    return modulePresent(QStringLiteral("gps"));
}

bool AppController::flightModuleAvailable() const
{
    return modulePresent(QStringLiteral("flight"));
}

bool AppController::engineModuleAvailable() const
{
    return modulePresent(QStringLiteral("engine"));
}

bool AppController::trafficModuleAvailable() const
{
    return modulePresent(QStringLiteral("traffic"));
}

bool AppController::flightDataHealthy() const
{
    return moduleHealthy(QStringLiteral("flight"));
}

bool AppController::engineDataHealthy() const
{
    return moduleHealthy(QStringLiteral("engine"));
}

bool AppController::trafficDataHealthy() const
{
    return moduleHealthy(QStringLiteral("traffic"));
}

QString AppController::primaryAlert() const
{
    return m_primaryAlert;
}

QString AppController::secondaryAlert() const
{
    return m_secondaryAlert;
}

bool AppController::cautionActive() const
{
    return !m_cautionMessages.isEmpty();
}

bool AppController::warningActive() const
{
    return !m_warningMessages.isEmpty();
}

QStringList AppController::cautionMessages() const
{
    return m_cautionMessages;
}

QStringList AppController::warningMessages() const
{
    return m_warningMessages;
}

int AppController::cautionEventSerial() const
{
    return m_cautionEventSerial;
}

int AppController::warningEventSerial() const
{
    return m_warningEventSerial;
}

int AppController::trafficCount() const
{
    return m_trafficCount;
}

double AppController::nearestTrafficRangeNm() const
{
    return m_nearestTrafficRangeNm;
}

double AppController::nearestTrafficRelativeAltitudeFt() const
{
    return m_nearestTrafficRelativeAltitudeFt;
}

QVariantList AppController::trafficTargets() const
{
    return m_trafficTargets;
}

void AppController::nextPage()
{
    if (m_availablePages.isEmpty()) {
        return;
    }

    const qsizetype currentIndex = m_availablePages.indexOf(m_currentPage);
    const int index = currentIndex < 0 ? 0 : static_cast<int>(currentIndex);
    setCurrentPage(m_availablePages.at((index + 1) % m_availablePages.size()));
}

void AppController::previousPage()
{
    if (m_availablePages.isEmpty()) {
        return;
    }

    const qsizetype currentIndex = m_availablePages.indexOf(m_currentPage);
    const int index = currentIndex < 0 ? 0 : static_cast<int>(currentIndex);
    const int nextIndex = (index - 1 + m_availablePages.size()) % m_availablePages.size();
    setCurrentPage(m_availablePages.at(nextIndex));
}

void AppController::setModuleEnabled(const QString &moduleKey, bool enabled)
{
    const bool changed = m_moduleModel.setModuleState(
        moduleKey,
        enabled,
        enabled,
        enabled ? QStringLiteral("demo") : QStringLiteral("offline"),
        enabled ? QStringLiteral("Synthetic module active") : QStringLiteral("Module disabled"));

    if (!changed) {
        return;
    }

    syncAvailablePages();
    refreshAlerts();
    emit moduleStateChanged();
}

void AppController::adjustMapZoom(int direction)
{
    if (direction == 0) {
        return;
    }

    const double step = direction > 0 ? 0.2 : -0.2;
    setMapZoomFactor(m_mapZoomFactor + step);
}

void AppController::resetMapZoom()
{
    setMapZoomFactor(1.0);
}

void AppController::setDirectWaypoint(const QString &waypointIdent)
{
    const QString normalized = waypointIdent.trimmed().toUpper();
    if (normalized.isEmpty()) {
        return;
    }

    m_userFlightPlanIdentifiers = { normalized };
    m_activeWaypointLabel = normalized;
    if (m_autopilotMaster) {
        m_autopilotLateralMode = QStringLiteral("NAV");
        emit autopilotChanged();
    }
    emit navDataChanged();
}

void AppController::setFlightPlanWaypoints(const QVariantList &waypointIdentifiers)
{
    QStringList identifiers;
    identifiers.reserve(waypointIdentifiers.size());

    for (const QVariant &value : waypointIdentifiers) {
        const QString normalized = value.toString().trimmed().toUpper();
        if (!normalized.isEmpty()) {
            identifiers << normalized;
        }
    }

    m_userFlightPlanIdentifiers = identifiers;
    if (!m_userFlightPlanIdentifiers.isEmpty()) {
        m_activeWaypointLabel = m_userFlightPlanIdentifiers.first();
        if (m_autopilotMaster) {
            m_autopilotLateralMode = QStringLiteral("NAV");
            emit autopilotChanged();
        }
    }
    emit navDataChanged();
}

void AppController::clearFlightPlan()
{
    if (m_userFlightPlanIdentifiers.isEmpty()) {
        return;
    }

    m_userFlightPlanIdentifiers.clear();
    emit navDataChanged();
}

void AppController::saveCurrentFlightPlan(const QString &name)
{
    if (m_navDataManager == nullptr || m_userFlightPlanIdentifiers.isEmpty()) {
        return;
    }

    QVariantList identifiers;
    for (const QString &ident : m_userFlightPlanIdentifiers) {
        identifiers.push_back(ident);
    }
    m_navDataManager->saveFlightPlan(name, identifiers);
}

void AppController::loadStoredFlightPlan(const QString &id)
{
    if (m_navDataManager == nullptr) {
        return;
    }
    setFlightPlanWaypoints(m_navDataManager->storedFlightPlanWaypoints(id));
}

QVariantList AppController::searchWaypoints(const QString &query, int limit) const
{
    return m_navDataManager != nullptr ? m_navDataManager->searchWaypoints(query, limit) : QVariantList {};
}

QVariantList AppController::nearestAirports(int limit) const
{
    return m_navDataManager != nullptr
        ? m_navDataManager->nearestWaypoints(m_latitudeDeg, m_longitudeDeg, limit, true)
        : QVariantList {};
}

void AppController::setFuelTankLabel(int index, const QString &label)
{
    if (index < 0 || index >= m_fuelTankLabels.size()) {
        return;
    }

    const QString normalized = label.trimmed().toUpper();
    if (normalized.isEmpty() || m_fuelTankLabels.at(index) == normalized) {
        return;
    }

    m_fuelTankLabels[index] = normalized;
    rebuildFuelTankStates();
    emit configurationChanged();
    emit engineDataChanged();
    refreshAlerts();
}

void AppController::seedModules()
{
    m_moduleModel.setModules({
        { QStringLiteral("gps"), QStringLiteral("GPS / GNSS"), true, true, QStringLiteral("demo"), QStringLiteral("Moving-map position source") },
        { QStringLiteral("flight"), QStringLiteral("ADC / AHRS"), true, true, QStringLiteral("demo"), QStringLiteral("Synthetic attitude and air-data") },
        { QStringLiteral("engine"), QStringLiteral("EIS"), true, true, QStringLiteral("demo"), QStringLiteral("Synthetic engine data") },
        { QStringLiteral("traffic"), QStringLiteral("ADS-B In"), true, true, QStringLiteral("demo"), QStringLiteral("Synthetic traffic feed") },
    });
}

void AppController::configureFuelTanks(int count)
{
    m_fuelTankCount = std::clamp(count, 1, 4);
    m_fuelTankLabels = defaultFuelTankLabelsForCount(m_fuelTankCount);
    m_fuelTankCapacitiesGal = defaultFuelTankCapacitiesForCount(m_fuelTankCount);
}

void AppController::rebuildFuelTankStates()
{
    QVariantList tankStates;
    if (m_fuelTankCount <= 0 || m_fuelTankCapacitiesGal.size() != m_fuelTankCount) {
        m_fuelTankStates = tankStates;
        return;
    }

    double totalCapacityGal = 0.0;
    for (const QVariant &value : m_fuelTankCapacitiesGal) {
        totalCapacityGal += value.toDouble();
    }
    totalCapacityGal = std::max(1.0, totalCapacityGal);

    QVector<double> distributionWeights(m_fuelTankCount, 1.0);
    if (m_fuelTankCount == 2) {
        const double imbalance = std::sin(m_demoTimeSeconds * 0.05) * 0.08;
        distributionWeights[0] = std::max(0.1, 1.0 + imbalance);
        distributionWeights[1] = std::max(0.1, 1.0 - imbalance);
    } else if (m_fuelTankCount == 3) {
        distributionWeights[0] = 0.92;
        distributionWeights[1] = 1.12;
        distributionWeights[2] = 0.96;
    } else if (m_fuelTankCount == 4) {
        distributionWeights[0] = 0.84;
        distributionWeights[1] = 1.06;
        distributionWeights[2] = 1.04;
        distributionWeights[3] = 0.82;
    }

    double weightedCapacity = 0.0;
    for (int index = 0; index < m_fuelTankCount; ++index) {
        weightedCapacity += m_fuelTankCapacitiesGal.at(index).toDouble() * distributionWeights.at(index);
    }
    weightedCapacity = std::max(0.1, weightedCapacity);

    for (int index = 0; index < m_fuelTankCount; ++index) {
        const double capacityGal = std::max(0.1, m_fuelTankCapacitiesGal.at(index).toDouble());
        const double weightedShare = (capacityGal * distributionWeights.at(index)) / weightedCapacity;
        const double quantityGal = std::clamp(m_fuelRemainingGal * weightedShare, 0.0, capacityGal);
        const double quantityPercent = std::clamp((quantityGal / capacityGal) * 100.0, 0.0, 100.0);

        QVariantMap tankState;
        tankState.insert(QStringLiteral("index"), index);
        tankState.insert(QStringLiteral("label"), m_fuelTankLabels.value(index, QStringLiteral("T%1").arg(index + 1)));
        tankState.insert(QStringLiteral("capacityGal"), capacityGal);
        tankState.insert(QStringLiteral("quantityGal"), quantityGal);
        tankState.insert(QStringLiteral("percent"), quantityPercent);
        tankState.insert(QStringLiteral("lowCaution"), quantityPercent <= 20.0);
        tankState.insert(QStringLiteral("lowWarning"), quantityPercent <= 10.0);
        tankStates.push_back(tankState);
    }

    m_fuelTankStates = tankStates;
}

void AppController::updateAlertEvents(const QStringList &cautionMessages, const QStringList &warningMessages)
{
    if (m_cautionMessages != cautionMessages) {
        if (!cautionMessages.isEmpty()) {
            ++m_cautionEventSerial;
        }
        m_cautionMessages = cautionMessages;
    }

    if (m_warningMessages != warningMessages) {
        if (!warningMessages.isEmpty()) {
            ++m_warningEventSerial;
        }
        m_warningMessages = warningMessages;
    }
}

void AppController::applyScenarioPreset(const QString &scenario)
{
    const auto setModule = [this](const QString &key, bool present, bool healthy, const QString &details) {
        m_moduleModel.setModuleState(
            key,
            present,
            healthy,
            present ? QStringLiteral("demo") : QStringLiteral("offline"),
            details);
    };

    setModule(QStringLiteral("gps"), true, true, QStringLiteral("Moving-map position source"));

    if (scenario == QStringLiteral("gps_only")) {
        m_baroSettingInHg = 29.92;
        m_selectedAltitudeFt = 2500.0;
        m_activeWaypointLabel = QStringLiteral("KBFI");
        setModule(QStringLiteral("flight"), false, false, QStringLiteral("ADC / AHRS not connected"));
        setModule(QStringLiteral("engine"), false, false, QStringLiteral("EIS not connected"));
        setModule(QStringLiteral("traffic"), false, false, QStringLiteral("Traffic receiver not connected"));
    } else if (scenario == QStringLiteral("takeoff")) {
        m_baroSettingInHg = 30.11;
        m_selectedAltitudeFt = 5500.0;
        m_activeWaypointLabel = QStringLiteral("KSEA");
        setModule(QStringLiteral("flight"), true, true, QStringLiteral("Flight sensors online"));
        setModule(QStringLiteral("engine"), true, true, QStringLiteral("Engine sensors online"));
        setModule(QStringLiteral("traffic"), false, false, QStringLiteral("Traffic receiver not connected"));
    } else if (scenario == QStringLiteral("cruise")) {
        m_baroSettingInHg = 30.11;
        m_selectedAltitudeFt = 7500.0;
        m_activeWaypointLabel = QStringLiteral("KPAE");
        setModule(QStringLiteral("flight"), true, true, QStringLiteral("Flight sensors online"));
        setModule(QStringLiteral("engine"), true, true, QStringLiteral("Engine sensors online"));
        setModule(QStringLiteral("traffic"), true, true, QStringLiteral("Traffic feed active"));
    } else if (scenario == QStringLiteral("approach")) {
        m_baroSettingInHg = 29.98;
        m_selectedAltitudeFt = 2400.0;
        m_activeWaypointLabel = QStringLiteral("KBFI");
        setModule(QStringLiteral("flight"), true, true, QStringLiteral("Flight sensors online"));
        setModule(QStringLiteral("engine"), true, true, QStringLiteral("Engine sensors online"));
        setModule(QStringLiteral("traffic"), false, false, QStringLiteral("Traffic receiver not connected"));
    } else if (scenario == QStringLiteral("engine_alert")) {
        m_baroSettingInHg = 29.87;
        m_selectedAltitudeFt = 7400.0;
        m_activeWaypointLabel = QStringLiteral("KORS");
        setModule(QStringLiteral("flight"), true, true, QStringLiteral("Flight sensors online"));
        setModule(QStringLiteral("engine"), true, true, QStringLiteral("Engine warning state active"));
        setModule(QStringLiteral("traffic"), false, false, QStringLiteral("Traffic receiver not connected"));
    } else if (scenario == QStringLiteral("traffic")) {
        m_baroSettingInHg = 30.05;
        m_selectedAltitudeFt = 6500.0;
        m_activeWaypointLabel = QStringLiteral("KPAE");
        setModule(QStringLiteral("flight"), true, true, QStringLiteral("Flight sensors online"));
        setModule(QStringLiteral("engine"), false, false, QStringLiteral("EIS not connected"));
        setModule(QStringLiteral("traffic"), true, true, QStringLiteral("Traffic feed active"));
    } else if (scenario == QStringLiteral("module_failure")) {
        m_baroSettingInHg = 30.11;
        m_selectedAltitudeFt = 7500.0;
        m_activeWaypointLabel = QStringLiteral("KPAE");
        setModule(QStringLiteral("flight"), true, false, QStringLiteral("ADC / AHRS data stale"));
        setModule(QStringLiteral("engine"), true, true, QStringLiteral("Engine sensors online"));
        setModule(QStringLiteral("traffic"), false, false, QStringLiteral("Traffic receiver not connected"));
    }
}

void AppController::syncAvailablePages()
{
    QStringList pages;

    if (gpsModuleAvailable() && flightModuleAvailable()) {
        pages << QStringLiteral("PFD + Map");
    }
    if (flightModuleAvailable()) {
        pages << QStringLiteral("PFD Focus");
    }
    if (gpsModuleAvailable()) {
        pages << QStringLiteral("Full Map");
    }
    if (engineModuleAvailable()) {
        pages << QStringLiteral("Engine");
    }
    if (trafficModuleAvailable()) {
        pages << QStringLiteral("Traffic");
    }

    pages << QStringLiteral("System Status")
          << QStringLiteral("Setup");

    if (m_availablePages == pages) {
        if (m_currentPage.isEmpty()) {
            m_currentPage = m_availablePages.value(0, QStringLiteral("System Status"));
            emit currentPageChanged();
        }
        return;
    }

    const bool pageStillValid = pages.contains(m_currentPage);
    m_availablePages = pages;
    emit availablePagesChanged();

    if (!pageStillValid) {
        m_currentPage = m_availablePages.value(0, QStringLiteral("System Status"));
        emit currentPageChanged();
    } else if (m_currentPage.isEmpty()) {
        m_currentPage = m_availablePages.value(0, QStringLiteral("System Status"));
        emit currentPageChanged();
    }
}

void AppController::updateDemoFrame()
{
    if (m_demoMode) {
        m_demoTimeSeconds += 0.1;
    }

    const double t = m_demoTimeSeconds;
    const double phase = std::fmod(t, 240.0);
    QVariantList trafficTargets;
    QStringList routeIdentifiers = m_userFlightPlanIdentifiers;
    if (routeIdentifiers.isEmpty()) {
        routeIdentifiers = { m_activeWaypointLabel };
    }

    if (m_scenario == QStringLiteral("gps_only")) {
        m_groundSpeedKt = 82.0 + std::sin(t * 0.25) * 2.5;
        m_groundTrackDeg = clampHeading(118.0 + std::sin(t * 0.12) * 4.0);
        m_headingDeg = m_groundTrackDeg;
        m_latitudeDeg = 47.508 + std::sin(t * 0.02) * 0.020;
        m_longitudeDeg = -122.288 + std::cos(t * 0.017) * 0.028;
        m_pitchDeg = 0.0;
        m_rollDeg = 0.0;
        m_turnRateDegPerSec = 0.0;
        m_slipSkidNormalized = 0.0;
        m_iasKt = 0.0;
        m_tasKt = 0.0;
        m_aoaNormalized = 0.0;
        m_altitudeFt = 0.0;
        m_verticalSpeedFpm = 0.0;
        m_waypointBearingDeg = clampHeading(m_groundTrackDeg + 18.0);
        m_waypointDistanceNm = 4.6 + std::sin(t * 0.08) * 0.2;
        m_rpm = 0.0;
        m_manifoldPressureInHg = 0.0;
        m_oilPressurePsi = 0.0;
        m_oilTemperatureF = 0.0;
        m_fuelFlowGph = 0.0;
        m_fuelQuantityPercent = 76.0;
        m_busVoltage = 27.1;
        m_chtMaxF = 0.0;
        m_egtMaxF = 0.0;
    } else if (m_scenario == QStringLiteral("takeoff")) {
        m_pitchDeg = 8.5 + std::sin(t * 0.9) * 1.2;
        m_rollDeg = std::sin(t * 0.8) * 4.5;
        m_headingDeg = clampHeading(163.0 + std::sin(t * 0.18) * 5.0);
        m_turnRateDegPerSec = std::sin(t * 0.65) * 1.8;
        m_slipSkidNormalized = std::sin(t * 0.75) * 0.25;
        m_iasKt = 74.0 + std::sin(t * 0.25) * 6.5;
        m_tasKt = m_iasKt + 3.0;
        m_aoaNormalized = 0.62 + std::sin(t * 0.6) * 0.06;
        m_altitudeFt = 1750.0 + phase * 22.0 + std::sin(t * 0.4) * 30.0;
        m_verticalSpeedFpm = 780.0 + std::sin(t * 0.55) * 60.0;
        m_groundSpeedKt = 94.0 + std::sin(t * 0.22) * 4.0;
        m_groundTrackDeg = clampHeading(m_headingDeg + 2.0);
        m_latitudeDeg = 47.444 + phase * 0.00020 + std::sin(t * 0.018) * 0.001;
        m_longitudeDeg = -122.309 + phase * 0.00048;
        m_waypointBearingDeg = clampHeading(m_groundTrackDeg + 8.0 + std::sin(t * 0.2) * 2.0);
        m_waypointDistanceNm = std::max(1.2, 12.0 - phase * 0.035);
        m_rpm = 2480.0 + std::sin(t * 0.55) * 24.0;
        m_manifoldPressureInHg = 27.1 + std::sin(t * 0.15) * 0.3;
        m_oilPressurePsi = 73.0 + std::sin(t * 0.18) * 2.0;
        m_oilTemperatureF = 196.0 + std::sin(t * 0.12) * 4.0;
        m_fuelFlowGph = 13.4 + std::sin(t * 0.25) * 0.4;
        m_fuelQuantityPercent = 84.0 - std::fmod(t * 0.01, 3.0);
        m_busVoltage = 27.6;
        m_chtMaxF = 362.0 + std::sin(t * 0.3) * 8.0;
        m_egtMaxF = 1415.0 + std::sin(t * 0.25) * 18.0;
    } else if (m_scenario == QStringLiteral("approach")) {
        m_pitchDeg = -2.2 + std::sin(t * 0.85) * 0.7;
        m_rollDeg = std::sin(t * 0.9) * 7.5;
        m_headingDeg = clampHeading(345.0 + std::sin(t * 0.22) * 8.0);
        m_turnRateDegPerSec = std::sin(t * 0.7) * 2.5;
        m_slipSkidNormalized = std::sin(t * 0.6) * 0.2;
        m_iasKt = 88.0 + std::sin(t * 0.4) * 4.5;
        m_tasKt = m_iasKt + 5.0;
        m_aoaNormalized = 0.58 + std::sin(t * 0.45) * 0.05;
        m_altitudeFt = 3100.0 - std::fmod(phase * 14.0, 850.0) + std::sin(t * 0.5) * 20.0;
        m_verticalSpeedFpm = -520.0 + std::sin(t * 0.4) * 40.0;
        m_groundSpeedKt = 96.0 + std::sin(t * 0.18) * 3.0;
        m_groundTrackDeg = clampHeading(m_headingDeg + 1.8);
        m_latitudeDeg = 47.628 - phase * 0.00018 + std::sin(t * 0.02) * 0.0014;
        m_longitudeDeg = -122.387 + phase * 0.00022;
        m_waypointBearingDeg = clampHeading(m_groundTrackDeg + 3.0 + std::sin(t * 0.14) * 2.0);
        m_waypointDistanceNm = std::max(0.8, 8.4 - phase * 0.03);
        m_rpm = 2230.0 + std::sin(t * 0.45) * 30.0;
        m_manifoldPressureInHg = 20.8 + std::sin(t * 0.18) * 0.3;
        m_oilPressurePsi = 62.0 + std::sin(t * 0.16) * 2.0;
        m_oilTemperatureF = 189.0 + std::sin(t * 0.1) * 3.0;
        m_fuelFlowGph = 9.2 + std::sin(t * 0.15) * 0.2;
        m_fuelQuantityPercent = 61.0 - std::fmod(t * 0.008, 2.0);
        m_busVoltage = 27.4;
        m_chtMaxF = 346.0 + std::sin(t * 0.25) * 7.0;
        m_egtMaxF = 1368.0 + std::sin(t * 0.22) * 15.0;
    } else if (m_scenario == QStringLiteral("engine_alert")) {
        m_pitchDeg = 1.2 + std::sin(t * 0.5) * 0.6;
        m_rollDeg = std::sin(t * 0.3) * 4.0;
        m_headingDeg = clampHeading(118.0 + std::sin(t * 0.1) * 4.0);
        m_turnRateDegPerSec = std::sin(t * 0.25) * 0.8;
        m_slipSkidNormalized = std::sin(t * 0.4) * 0.12;
        m_iasKt = 129.0 + std::sin(t * 0.16) * 2.5;
        m_tasKt = 136.0 + std::sin(t * 0.16) * 2.8;
        m_aoaNormalized = 0.34 + std::sin(t * 0.14) * 0.03;
        m_altitudeFt = 6550.0 + std::sin(t * 0.22) * 60.0;
        m_verticalSpeedFpm = -20.0 + std::sin(t * 0.2) * 30.0;
        m_groundSpeedKt = 141.0 + std::sin(t * 0.15) * 1.7;
        m_groundTrackDeg = clampHeading(m_headingDeg + 3.5);
        m_latitudeDeg = 47.752 + std::sin(t * 0.012) * 0.018;
        m_longitudeDeg = -122.207 + phase * 0.00011;
        m_waypointBearingDeg = clampHeading(m_groundTrackDeg + 28.0 + std::sin(t * 0.1) * 3.0);
        m_waypointDistanceNm = 21.4 + std::sin(t * 0.08) * 0.8;
        m_rpm = 2410.0 + std::sin(t * 0.4) * 20.0;
        m_manifoldPressureInHg = 24.2 + std::sin(t * 0.14) * 0.2;
        m_oilPressurePsi = 49.0 + std::sin(t * 0.18) * 1.5;
        m_oilTemperatureF = 242.0 + std::sin(t * 0.45) * 3.0;
        m_fuelFlowGph = 12.4 + std::sin(t * 0.1) * 0.2;
        m_fuelQuantityPercent = 55.0 - std::fmod(t * 0.007, 1.5);
        m_busVoltage = 27.2;
        m_chtMaxF = 436.0 + std::sin(t * 0.4) * 7.0;
        m_egtMaxF = 1458.0 + std::sin(t * 0.2) * 12.0;
    } else {
        m_pitchDeg = 1.0 + std::sin(t * 0.55) * 1.3;
        m_rollDeg = std::sin(t * 0.32) * 8.5;
        m_headingDeg = clampHeading(68.0 + std::sin(t * 0.16) * 12.0);
        m_turnRateDegPerSec = std::sin(t * 0.42) * 1.4;
        m_slipSkidNormalized = std::sin(t * 0.38) * 0.14;
        m_iasKt = 132.0 + std::sin(t * 0.2) * 3.5;
        m_tasKt = 141.0 + std::sin(t * 0.2) * 3.8;
        m_aoaNormalized = 0.28 + std::sin(t * 0.18) * 0.03;
        m_altitudeFt = 7480.0 + std::sin(t * 0.18) * 75.0;
        m_verticalSpeedFpm = std::sin(t * 0.12) * 40.0;
        m_groundSpeedKt = 148.0 + std::sin(t * 0.15) * 2.2;
        m_groundTrackDeg = clampHeading(m_headingDeg + 1.5);
        m_latitudeDeg = 47.544 + std::sin(t * 0.01) * 0.018;
        m_longitudeDeg = -122.365 + phase * 0.00070;
        m_waypointBearingDeg = clampHeading(m_groundTrackDeg - 22.0 + std::sin(t * 0.18) * 4.0);
        m_waypointDistanceNm = std::max(2.0, 18.9 - phase * 0.045);
        m_rpm = 2340.0 + std::sin(t * 0.3) * 18.0;
        m_manifoldPressureInHg = 25.9 + std::sin(t * 0.12) * 0.25;
        m_oilPressurePsi = 68.0 + std::sin(t * 0.14) * 1.6;
        m_oilTemperatureF = 201.0 + std::sin(t * 0.1) * 2.0;
        m_fuelFlowGph = 10.6 + std::sin(t * 0.08) * 0.2;
        m_fuelQuantityPercent = 67.0 - std::fmod(t * 0.006, 2.5);
        m_busVoltage = 27.6;
        m_chtMaxF = 348.0 + std::sin(t * 0.22) * 6.0;
        m_egtMaxF = 1392.0 + std::sin(t * 0.16) * 14.0;
    }

    if (!flightModuleAvailable()) {
        m_pitchDeg = 0.0;
        m_rollDeg = 0.0;
        m_headingDeg = m_groundTrackDeg;
        m_turnRateDegPerSec = 0.0;
        m_slipSkidNormalized = 0.0;
        m_iasKt = 0.0;
        m_tasKt = 0.0;
        m_airspeedTrendKt = 0.0;
        m_aoaNormalized = 0.0;
        m_altitudeFt = 0.0;
        m_verticalSpeedFpm = 0.0;
    }

    if (!engineModuleAvailable()) {
        m_rpm = 0.0;
        m_manifoldPressureInHg = 0.0;
        m_oilPressurePsi = 0.0;
        m_oilTemperatureF = 0.0;
        m_fuelFlowGph = 0.0;
        m_fuelQuantityPercent = 0.0;
        m_chtMaxF = 0.0;
        m_egtMaxF = 0.0;
    }

    if (!trafficModuleAvailable()) {
        m_trafficTargets.clear();
        m_trafficCount = 0;
        m_nearestTrafficRangeNm = 0.0;
        m_nearestTrafficRelativeAltitudeFt = 0.0;
    }

    if (m_demoMode
        && flightModuleAvailable()
        && gpsModuleAvailable()
        && m_autopilotMaster
        && m_autopilotLateralMode == QStringLiteral("NAV")
        && !routeIdentifiers.isEmpty()) {
        const QVariantMap activeRouteTarget = resolveWaypoint(routeIdentifiers.first());
        if (!activeRouteTarget.isEmpty()) {
            if (!m_demoRouteOwnshipValid) {
                m_demoRouteLatitudeDeg = m_latitudeDeg;
                m_demoRouteLongitudeDeg = m_longitudeDeg;
                m_demoRouteHeadingDeg = m_headingDeg;
                m_demoRouteOwnshipValid = true;
            }

            const double desiredTrackDeg = initialBearingDeg(
                m_demoRouteLatitudeDeg,
                m_demoRouteLongitudeDeg,
                activeRouteTarget.value(QStringLiteral("latitudeDeg")).toDouble(),
                activeRouteTarget.value(QStringLiteral("longitudeDeg")).toDouble());
            const double headingErrorDeg = signedHeadingErrorDeg(desiredTrackDeg, m_demoRouteHeadingDeg);
            const double turnStepDeg = std::clamp(headingErrorDeg * 0.18, -2.2, 2.2);
            m_demoRouteHeadingDeg = clampHeading(m_demoRouteHeadingDeg + turnStepDeg);

            const double distanceStepNm = std::max(0.05, m_groundSpeedKt * 0.1 / 3600.0);
            const QPair<double, double> nextCoordinate = offsetCoordinate(
                m_demoRouteLatitudeDeg,
                m_demoRouteLongitudeDeg,
                m_demoRouteHeadingDeg,
                distanceStepNm);
            m_demoRouteLatitudeDeg = nextCoordinate.first;
            m_demoRouteLongitudeDeg = nextCoordinate.second;

            m_latitudeDeg = m_demoRouteLatitudeDeg;
            m_longitudeDeg = m_demoRouteLongitudeDeg;
            m_headingDeg = m_demoRouteHeadingDeg;
            m_groundTrackDeg = m_demoRouteHeadingDeg;
            m_turnRateDegPerSec = turnStepDeg * 10.0;
            m_rollDeg = std::clamp(turnStepDeg * 8.0, -20.0, 20.0);
            m_slipSkidNormalized = std::clamp(turnStepDeg / 2.2, -0.45, 0.45);
        }
    } else {
        m_demoRouteOwnshipValid = false;
    }

    const double instantAirspeedTrend = std::clamp((m_iasKt - m_previousIasKt) * 60.0, -25.0, 25.0);
    m_airspeedTrendKt = m_demoMode
        ? (m_airspeedTrendKt * 0.72 + instantAirspeedTrend * 0.28)
        : 0.0;
    m_previousIasKt = m_iasKt;

    QVariantList flightPlanLegs;
    double cumulativeDistance = 0.0;
    const QVariantMap activeLookup = resolveWaypoint(routeIdentifiers.value(0, m_activeWaypointLabel));
    if (!activeLookup.isEmpty()) {
        m_activeWaypointLabel = activeLookup.value(QStringLiteral("ident")).toString();
        m_waypointBearingDeg = initialBearingDeg(
            m_latitudeDeg,
            m_longitudeDeg,
            activeLookup.value(QStringLiteral("latitudeDeg")).toDouble(),
            activeLookup.value(QStringLiteral("longitudeDeg")).toDouble());
        m_waypointDistanceNm = haversineDistanceNm(
            m_latitudeDeg,
            m_longitudeDeg,
            activeLookup.value(QStringLiteral("latitudeDeg")).toDouble(),
            activeLookup.value(QStringLiteral("longitudeDeg")).toDouble());
    }
    if (m_demoMode && routeIdentifiers.size() > 1 && m_waypointDistanceNm < 0.55) {
        m_userFlightPlanIdentifiers.removeFirst();
        routeIdentifiers = m_userFlightPlanIdentifiers;
        m_activeWaypointLabel = routeIdentifiers.first();
        const QVariantMap nextLookup = resolveWaypoint(m_activeWaypointLabel);
        if (!nextLookup.isEmpty()) {
            m_waypointBearingDeg = initialBearingDeg(
                m_latitudeDeg,
                m_longitudeDeg,
                nextLookup.value(QStringLiteral("latitudeDeg")).toDouble(),
                nextLookup.value(QStringLiteral("longitudeDeg")).toDouble());
            m_waypointDistanceNm = haversineDistanceNm(
                m_latitudeDeg,
                m_longitudeDeg,
                nextLookup.value(QStringLiteral("latitudeDeg")).toDouble(),
                nextLookup.value(QStringLiteral("longitudeDeg")).toDouble());
        }
    }
    const double eteHours = m_groundSpeedKt > 1.0 ? (m_waypointDistanceNm / m_groundSpeedKt) : 0.0;
    m_waypointEteLabel = formatHoursMinutes(eteHours);

    QVariantMap previousPoint;
    previousPoint.insert(QStringLiteral("latitudeDeg"), m_latitudeDeg);
    previousPoint.insert(QStringLiteral("longitudeDeg"), m_longitudeDeg);
    for (qsizetype index = 0; index < routeIdentifiers.size(); ++index) {
        const QString ident = routeIdentifiers.at(index);
        const QVariantMap resolved = resolveWaypoint(ident);
        const bool hasPosition = !resolved.isEmpty();
        const double waypointLatitude = resolved.value(QStringLiteral("latitudeDeg")).toDouble();
        const double waypointLongitude = resolved.value(QStringLiteral("longitudeDeg")).toDouble();
        const double fromLatitude = previousPoint.value(QStringLiteral("latitudeDeg")).toDouble();
        const double fromLongitude = previousPoint.value(QStringLiteral("longitudeDeg")).toDouble();
        const double legDistance = hasPosition
            ? haversineDistanceNm(fromLatitude, fromLongitude, waypointLatitude, waypointLongitude)
            : (index == 0 ? m_waypointDistanceNm : 8.0 + index * 6.5);
        const double legBearing = hasPosition
            ? initialBearingDeg(fromLatitude, fromLongitude, waypointLatitude, waypointLongitude)
            : clampHeading(m_waypointBearingDeg + index * 24.0);
        cumulativeDistance += legDistance;

        QVariantMap leg;
        leg.insert(QStringLiteral("ident"), ident);
        leg.insert(QStringLiteral("name"), resolved.value(QStringLiteral("name")).toString().isEmpty()
            ? ident
            : resolved.value(QStringLiteral("name")).toString());
        leg.insert(QStringLiteral("dtkDeg"), legBearing);
        leg.insert(QStringLiteral("distanceNm"), legDistance);
        leg.insert(QStringLiteral("cumulativeNm"), cumulativeDistance);
        leg.insert(QStringLiteral("bearingDeg"), legBearing);
        leg.insert(QStringLiteral("rangeNm"), index == 0 ? m_waypointDistanceNm : legDistance);
        leg.insert(QStringLiteral("latitudeDeg"), waypointLatitude);
        leg.insert(QStringLiteral("longitudeDeg"), waypointLongitude);
        leg.insert(QStringLiteral("validPosition"), hasPosition);
        flightPlanLegs.push_back(leg);

        if (hasPosition) {
            previousPoint = resolved;
        }
    }
    m_flightPlanLegs = flightPlanLegs;

    trafficTargets.clear();
    const auto addIndependentTrafficTarget = [&](double startLatitudeDeg,
                                                 double startLongitudeDeg,
                                                 double trackDeg,
                                                 double speedKt,
                                                 double altitudeFt,
                                                 const QString &style,
                                                 int verticalTrend,
                                                 const QString &callsign,
                                                 double headingWaveDeg,
                                                 double headingWaveRate) {
        const double distanceNm = std::fmod(speedKt * t / 3600.0, 36.0);
        const double dynamicTrackDeg = clampHeading(trackDeg + std::sin(t * headingWaveRate) * headingWaveDeg);
        const QPair<double, double> coordinate = offsetCoordinate(startLatitudeDeg, startLongitudeDeg, dynamicTrackDeg, distanceNm);
        const double rangeNm = haversineDistanceNm(m_latitudeDeg, m_longitudeDeg, coordinate.first, coordinate.second);
        const double bearingDeg = initialBearingDeg(m_latitudeDeg, m_longitudeDeg, coordinate.first, coordinate.second);

        QVariantMap target;
        target.insert(QStringLiteral("bearingDeg"), bearingDeg);
        target.insert(QStringLiteral("rangeNm"), rangeNm);
        target.insert(QStringLiteral("relativeAltitudeFt"), altitudeFt - m_altitudeFt);
        target.insert(QStringLiteral("style"), style);
        target.insert(QStringLiteral("verticalTrend"), verticalTrend);
        target.insert(QStringLiteral("callsign"), callsign);
        target.insert(QStringLiteral("latitudeDeg"), coordinate.first);
        target.insert(QStringLiteral("longitudeDeg"), coordinate.second);
        target.insert(QStringLiteral("trackDeg"), dynamicTrackDeg);
        target.insert(QStringLiteral("speedKt"), speedKt);
        trafficTargets.push_back(target);
    };

    if (trafficModuleAvailable()) {
        if (m_scenario == QStringLiteral("traffic")) {
            addIndependentTrafficTarget(47.482, -122.456, 62.0, 118.0, m_altitudeFt + 400.0, QStringLiteral("filled"), 1, QStringLiteral("N430A"), 14.0, 0.19);
            addIndependentTrafficTarget(47.618, -122.246, 212.0, 132.0, m_altitudeFt - 900.0, QStringLiteral("open"), -1, QStringLiteral("N291K"), 9.0, 0.15);
            addIndependentTrafficTarget(47.704, -122.118, 286.0, 168.0, m_altitudeFt + 1200.0, QStringLiteral("open"), 0, QStringLiteral("DAL52"), 7.0, 0.11);
            addIndependentTrafficTarget(47.575, -122.515, 101.0, 92.0, m_altitudeFt + 100.0, QStringLiteral("open"), 1, QStringLiteral("C172"), 11.0, 0.17);
        } else if (m_scenario == QStringLiteral("cruise")) {
            addIndependentTrafficTarget(47.525, -122.472, 82.0, 146.0, m_altitudeFt - 590.0, QStringLiteral("filled"), -1, QStringLiteral("N1023"), 12.0, 0.16);
            addIndependentTrafficTarget(47.706, -122.094, 241.0, 154.0, m_altitudeFt + 1200.0, QStringLiteral("open"), 0, QStringLiteral("ASA44"), 8.0, 0.12);
        }
    }

    std::sort(trafficTargets.begin(), trafficTargets.end(), [](const QVariant &left, const QVariant &right) {
        return left.toMap().value(QStringLiteral("rangeNm")).toDouble()
            < right.toMap().value(QStringLiteral("rangeNm")).toDouble();
    });

    m_trafficTargets = trafficTargets;
    m_trafficCount = m_trafficTargets.size();
    if (!m_trafficTargets.isEmpty()) {
        const QVariantMap nearestTarget = m_trafficTargets.first().toMap();
        m_nearestTrafficRangeNm = nearestTarget.value(QStringLiteral("rangeNm")).toDouble();
        m_nearestTrafficRelativeAltitudeFt = nearestTarget.value(QStringLiteral("relativeAltitudeFt")).toDouble();
    } else {
        m_nearestTrafficRangeNm = 0.0;
        m_nearestTrafficRelativeAltitudeFt = 0.0;
    }

    m_chtValues = {
        qMax(m_chtMaxF - 18.0, 0.0),
        qMax(m_chtMaxF - 5.0, 0.0),
        qMax(m_chtMaxF + 8.0, 0.0),
        qMax(m_chtMaxF - 12.0, 0.0)
    };
    m_egtValues = {
        qMax(m_egtMaxF - 42.0, 0.0),
        qMax(m_egtMaxF - 8.0, 0.0),
        qMax(m_egtMaxF + 16.0, 0.0),
        qMax(m_egtMaxF - 24.0, 0.0)
    };

    const double usableFuelGal = 36.0;
    m_fuelRemainingGal = std::clamp(usableFuelGal * (m_fuelQuantityPercent / 100.0), 0.0, usableFuelGal);
    m_fuelUsedGal = std::max(0.0, usableFuelGal - m_fuelRemainingGal);
    m_fuelEnduranceHours = m_fuelFlowGph > 0.1 ? (m_fuelRemainingGal / m_fuelFlowGph) : 0.0;
    m_fuelRangeNm = m_fuelEnduranceHours * m_groundSpeedKt;
    rebuildFuelTankStates();

    const QDateTime baseTime(QDate(2026, 5, 5), QTime(17, 29, 0), QTimeZone::UTC);
    m_utcTime = baseTime.addSecs(static_cast<int>(m_demoTimeSeconds * 4.0)).toString(QStringLiteral("hh:mm:ss 'Z'"));

    refreshAlerts();

    emit flightDataChanged();
    emit navDataChanged();
    emit engineDataChanged();
    emit trafficDataChanged();
    emit moduleStateChanged();
    emit configurationChanged();
}

void AppController::refreshAlerts()
{
    QStringList cautionMessages;
    QStringList warningMessages;

    if (!gpsModuleAvailable()) {
        cautionMessages << QStringLiteral("GPS NOT AVAILABLE");
    }
    if (!flightDataHealthy()) {
        cautionMessages << QStringLiteral("AHRS DATA STALE");
    }
    if (engineModuleAvailable() && !engineDataHealthy()) {
        cautionMessages << QStringLiteral("ENGINE DATA STALE");
    }

    if (engineModuleAvailable() && engineDataHealthy()) {
        if (m_oilTemperatureF >= 235.0) {
            warningMessages << QStringLiteral("OIL TEMP HIGH %1 F").arg(QString::number(m_oilTemperatureF, 'f', 0));
        } else if (m_oilTemperatureF >= 220.0) {
            cautionMessages << QStringLiteral("OIL TEMP HIGH %1 F").arg(QString::number(m_oilTemperatureF, 'f', 0));
        }

        if (m_oilPressurePsi <= 40.0) {
            warningMessages << QStringLiteral("OIL PRESS LOW %1 PSI").arg(QString::number(m_oilPressurePsi, 'f', 0));
        } else if (m_oilPressurePsi <= 55.0) {
            cautionMessages << QStringLiteral("OIL PRESS LOW %1 PSI").arg(QString::number(m_oilPressurePsi, 'f', 0));
        }

        if (m_chtMaxF >= 430.0) {
            warningMessages << QStringLiteral("CHT HIGH %1 F").arg(QString::number(m_chtMaxF, 'f', 0));
        } else if (m_chtMaxF >= 400.0) {
            cautionMessages << QStringLiteral("CHT HIGH %1 F").arg(QString::number(m_chtMaxF, 'f', 0));
        }

        if (m_egtMaxF >= 1500.0) {
            warningMessages << QStringLiteral("EGT HIGH %1 F").arg(QString::number(m_egtMaxF, 'f', 0));
        } else if (m_egtMaxF >= 1450.0) {
            cautionMessages << QStringLiteral("EGT HIGH %1 F").arg(QString::number(m_egtMaxF, 'f', 0));
        }

        if (m_busVoltage < 24.0) {
            warningMessages << QStringLiteral("BUS VOLT LOW %1 V").arg(QString::number(m_busVoltage, 'f', 1));
        } else if (m_busVoltage < 25.0) {
            cautionMessages << QStringLiteral("BUS VOLT LOW %1 V").arg(QString::number(m_busVoltage, 'f', 1));
        }

        for (const QVariant &tankValue : m_fuelTankStates) {
            const QVariantMap tank = tankValue.toMap();
            const QString label = tank.value(QStringLiteral("label")).toString();
            const double quantityGal = tank.value(QStringLiteral("quantityGal")).toDouble();
            const double percent = tank.value(QStringLiteral("percent")).toDouble();
            if (percent <= 10.0) {
                warningMessages << QStringLiteral("FUEL %1 LOW %2 GAL").arg(label, QString::number(quantityGal, 'f', 1));
            } else if (percent <= 20.0) {
                cautionMessages << QStringLiteral("FUEL %1 LOW %2 GAL").arg(label, QString::number(quantityGal, 'f', 1));
            }
        }
    }

    updateAlertEvents(cautionMessages, warningMessages);

    QString primary;
    QString secondary;

    if (!m_warningMessages.isEmpty()) {
        primary = QStringLiteral("WARNING: %1").arg(m_warningMessages.first());
        secondary = m_warningMessages.mid(1).join(QStringLiteral("  |  "));
        if (secondary.isEmpty() && !m_cautionMessages.isEmpty()) {
            secondary = m_cautionMessages.join(QStringLiteral("  |  "));
        }
    } else if (!m_cautionMessages.isEmpty()) {
        primary = QStringLiteral("CAUTION: %1").arg(m_cautionMessages.first());
        secondary = m_cautionMessages.mid(1).join(QStringLiteral("  |  "));
    } else if (m_trafficCount > 0 && trafficDataHealthy()) {
        primary = QStringLiteral("TRAFFIC: %1 NM  %2 FT")
            .arg(QString::number(m_nearestTrafficRangeNm, 'f', 1))
            .arg(QString::number(m_nearestTrafficRelativeAltitudeFt, 'f', 0));
        secondary = QStringLiteral("Scenario: %1").arg(scenarioLabel(m_scenario));
    } else {
        primary = m_demoMode
            ? QStringLiteral("DEMO MODE ACTIVE")
            : QStringLiteral("LIVE INPUT MODE");
        secondary = QStringLiteral("Scenario: %1  |  Display: %2  |  Chart: %3")
            .arg(scenarioLabel(m_scenario))
            .arg(m_displayClass)
            .arg(mapBaseSourceLabel());
    }

    if (m_primaryAlert == primary && m_secondaryAlert == secondary) {
        return;
    }

    m_primaryAlert = primary;
    m_secondaryAlert = secondary;
    emit alertsChanged();
}

bool AppController::modulePresent(const QString &moduleKey) const
{
    return m_moduleModel.isPresent(moduleKey);
}

bool AppController::moduleHealthy(const QString &moduleKey) const
{
    return m_moduleModel.isHealthy(moduleKey);
}

QVariantMap AppController::resolveWaypoint(const QString &ident) const
{
    return m_navDataManager != nullptr ? m_navDataManager->lookupWaypoint(ident) : QVariantMap {};
}
