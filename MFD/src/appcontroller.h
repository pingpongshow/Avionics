#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariantMap>
#include <QVariantList>

#include "modulelistmodel.h"

class NavDataManager;

class AppController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString currentPage READ currentPage WRITE setCurrentPage NOTIFY currentPageChanged)
    Q_PROPERTY(QStringList availablePages READ availablePages NOTIFY availablePagesChanged)
    Q_PROPERTY(QStringList displayClasses READ displayClasses CONSTANT)
    Q_PROPERTY(QString displayClass READ displayClass WRITE setDisplayClass NOTIFY displayClassChanged)
    Q_PROPERTY(QStringList mapBaseSources READ mapBaseSources CONSTANT)
    Q_PROPERTY(QString mapBaseSource READ mapBaseSource WRITE setMapBaseSource NOTIFY mapConfigChanged)
    Q_PROPERTY(QString mapBaseSourceLabel READ mapBaseSourceLabel NOTIFY mapConfigChanged)
    Q_PROPERTY(bool openAipOverlayEnabled READ openAipOverlayEnabled WRITE setOpenAipOverlayEnabled NOTIFY mapConfigChanged)
    Q_PROPERTY(bool weatherOverlayEnabled READ weatherOverlayEnabled WRITE setWeatherOverlayEnabled NOTIFY mapConfigChanged)
    Q_PROPERTY(double weatherOverlayOpacity READ weatherOverlayOpacity WRITE setWeatherOverlayOpacity NOTIFY mapConfigChanged)
    Q_PROPERTY(double mapZoomFactor READ mapZoomFactor WRITE setMapZoomFactor NOTIFY mapConfigChanged)
    Q_PROPERTY(QString mapStatusSummary READ mapStatusSummary NOTIFY mapConfigChanged)
    Q_PROPERTY(QString tailNumber READ tailNumber WRITE setTailNumber NOTIFY configurationChanged)
    Q_PROPERTY(bool setupAccessAllowed READ setupAccessAllowed NOTIFY configurationChanged)
    Q_PROPERTY(QStringList scenarios READ scenarios CONSTANT)
    Q_PROPERTY(QString scenario READ scenario WRITE setScenario NOTIFY scenarioChanged)
    Q_PROPERTY(bool demoMode READ demoMode WRITE setDemoMode NOTIFY demoModeChanged)
    Q_PROPERTY(QAbstractListModel *moduleModel READ moduleModel CONSTANT)
    Q_PROPERTY(double pitchDeg READ pitchDeg NOTIFY flightDataChanged)
    Q_PROPERTY(double rollDeg READ rollDeg NOTIFY flightDataChanged)
    Q_PROPERTY(double headingDeg READ headingDeg NOTIFY flightDataChanged)
    Q_PROPERTY(double turnRateDegPerSec READ turnRateDegPerSec NOTIFY flightDataChanged)
    Q_PROPERTY(double slipSkidNormalized READ slipSkidNormalized NOTIFY flightDataChanged)
    Q_PROPERTY(double iasKt READ iasKt NOTIFY flightDataChanged)
    Q_PROPERTY(double airspeedTrendKt READ airspeedTrendKt NOTIFY flightDataChanged)
    Q_PROPERTY(double tasKt READ tasKt NOTIFY flightDataChanged)
    Q_PROPERTY(double aoaNormalized READ aoaNormalized NOTIFY flightDataChanged)
    Q_PROPERTY(double altitudeFt READ altitudeFt NOTIFY flightDataChanged)
    Q_PROPERTY(double verticalSpeedFpm READ verticalSpeedFpm NOTIFY flightDataChanged)
    Q_PROPERTY(double baroSettingInHg READ baroSettingInHg WRITE setBaroSettingInHg NOTIFY flightDataChanged)
    Q_PROPERTY(double selectedAltitudeFt READ selectedAltitudeFt WRITE setSelectedAltitudeFt NOTIFY flightDataChanged)
    Q_PROPERTY(double latitudeDeg READ latitudeDeg NOTIFY navDataChanged)
    Q_PROPERTY(double longitudeDeg READ longitudeDeg NOTIFY navDataChanged)
    Q_PROPERTY(double groundSpeedKt READ groundSpeedKt NOTIFY navDataChanged)
    Q_PROPERTY(double groundTrackDeg READ groundTrackDeg NOTIFY navDataChanged)
    Q_PROPERTY(QString utcTime READ utcTime NOTIFY navDataChanged)
    Q_PROPERTY(QString activeWaypointLabel READ activeWaypointLabel NOTIFY navDataChanged)
    Q_PROPERTY(double waypointBearingDeg READ waypointBearingDeg NOTIFY navDataChanged)
    Q_PROPERTY(double waypointDistanceNm READ waypointDistanceNm NOTIFY navDataChanged)
    Q_PROPERTY(QString waypointEteLabel READ waypointEteLabel NOTIFY navDataChanged)
    Q_PROPERTY(QVariantList flightPlanLegs READ flightPlanLegs NOTIFY navDataChanged)
    Q_PROPERTY(double rpm READ rpm NOTIFY engineDataChanged)
    Q_PROPERTY(double manifoldPressureInHg READ manifoldPressureInHg NOTIFY engineDataChanged)
    Q_PROPERTY(double oilPressurePsi READ oilPressurePsi NOTIFY engineDataChanged)
    Q_PROPERTY(double oilTemperatureF READ oilTemperatureF NOTIFY engineDataChanged)
    Q_PROPERTY(double fuelFlowGph READ fuelFlowGph NOTIFY engineDataChanged)
    Q_PROPERTY(double fuelQuantityPercent READ fuelQuantityPercent NOTIFY engineDataChanged)
    Q_PROPERTY(int fuelTankCount READ fuelTankCount WRITE setFuelTankCount NOTIFY configurationChanged)
    Q_PROPERTY(QStringList fuelTankLabels READ fuelTankLabels NOTIFY configurationChanged)
    Q_PROPERTY(QVariantList fuelTankStates READ fuelTankStates NOTIFY engineDataChanged)
    Q_PROPERTY(double fuelRemainingGal READ fuelRemainingGal NOTIFY engineDataChanged)
    Q_PROPERTY(double fuelUsedGal READ fuelUsedGal NOTIFY engineDataChanged)
    Q_PROPERTY(double fuelEnduranceHours READ fuelEnduranceHours NOTIFY engineDataChanged)
    Q_PROPERTY(double fuelRangeNm READ fuelRangeNm NOTIFY engineDataChanged)
    Q_PROPERTY(double busVoltage READ busVoltage NOTIFY engineDataChanged)
    Q_PROPERTY(double chtMaxF READ chtMaxF NOTIFY engineDataChanged)
    Q_PROPERTY(double egtMaxF READ egtMaxF NOTIFY engineDataChanged)
    Q_PROPERTY(QVariantList chtValues READ chtValues NOTIFY engineDataChanged)
    Q_PROPERTY(QVariantList egtValues READ egtValues NOTIFY engineDataChanged)
    Q_PROPERTY(bool autopilotMaster READ autopilotMaster WRITE setAutopilotMaster NOTIFY autopilotChanged)
    Q_PROPERTY(bool flightDirectorEnabled READ flightDirectorEnabled WRITE setFlightDirectorEnabled NOTIFY autopilotChanged)
    Q_PROPERTY(bool levelModeEnabled READ levelModeEnabled WRITE setLevelModeEnabled NOTIFY autopilotChanged)
    Q_PROPERTY(QString autopilotLateralMode READ autopilotLateralMode WRITE setAutopilotLateralMode NOTIFY autopilotChanged)
    Q_PROPERTY(QString autopilotVerticalMode READ autopilotVerticalMode WRITE setAutopilotVerticalMode NOTIFY autopilotChanged)
    Q_PROPERTY(bool gpsModuleAvailable READ gpsModuleAvailable NOTIFY moduleStateChanged)
    Q_PROPERTY(bool flightModuleAvailable READ flightModuleAvailable NOTIFY moduleStateChanged)
    Q_PROPERTY(bool engineModuleAvailable READ engineModuleAvailable NOTIFY moduleStateChanged)
    Q_PROPERTY(bool trafficModuleAvailable READ trafficModuleAvailable NOTIFY moduleStateChanged)
    Q_PROPERTY(bool flightDataHealthy READ flightDataHealthy NOTIFY moduleStateChanged)
    Q_PROPERTY(bool engineDataHealthy READ engineDataHealthy NOTIFY moduleStateChanged)
    Q_PROPERTY(bool trafficDataHealthy READ trafficDataHealthy NOTIFY moduleStateChanged)
    Q_PROPERTY(QString primaryAlert READ primaryAlert NOTIFY alertsChanged)
    Q_PROPERTY(QString secondaryAlert READ secondaryAlert NOTIFY alertsChanged)
    Q_PROPERTY(bool cautionActive READ cautionActive NOTIFY alertsChanged)
    Q_PROPERTY(bool warningActive READ warningActive NOTIFY alertsChanged)
    Q_PROPERTY(QStringList cautionMessages READ cautionMessages NOTIFY alertsChanged)
    Q_PROPERTY(QStringList warningMessages READ warningMessages NOTIFY alertsChanged)
    Q_PROPERTY(int cautionEventSerial READ cautionEventSerial NOTIFY alertsChanged)
    Q_PROPERTY(int warningEventSerial READ warningEventSerial NOTIFY alertsChanged)
    Q_PROPERTY(int trafficCount READ trafficCount NOTIFY trafficDataChanged)
    Q_PROPERTY(double nearestTrafficRangeNm READ nearestTrafficRangeNm NOTIFY trafficDataChanged)
    Q_PROPERTY(double nearestTrafficRelativeAltitudeFt READ nearestTrafficRelativeAltitudeFt NOTIFY trafficDataChanged)
    Q_PROPERTY(QVariantList trafficTargets READ trafficTargets NOTIFY trafficDataChanged)

public:
    explicit AppController(QObject *parent = nullptr);
    void setNavDataManager(NavDataManager *navDataManager);

    QString currentPage() const;
    void setCurrentPage(const QString &page);

    QStringList availablePages() const;
    QStringList displayClasses() const;

    QString displayClass() const;
    void setDisplayClass(const QString &displayClass);

    QStringList mapBaseSources() const;
    QString mapBaseSource() const;
    void setMapBaseSource(const QString &mapBaseSource);
    QString mapBaseSourceLabel() const;
    bool openAipOverlayEnabled() const;
    void setOpenAipOverlayEnabled(bool enabled);
    bool weatherOverlayEnabled() const;
    void setWeatherOverlayEnabled(bool enabled);
    double weatherOverlayOpacity() const;
    void setWeatherOverlayOpacity(double opacity);
    double mapZoomFactor() const;
    void setMapZoomFactor(double zoomFactor);
    QString mapStatusSummary() const;
    QString tailNumber() const;
    void setTailNumber(const QString &tailNumber);
    bool setupAccessAllowed() const;

    QStringList scenarios() const;
    QString scenario() const;
    void setScenario(const QString &scenario);

    bool demoMode() const;
    void setDemoMode(bool enabled);

    QAbstractListModel *moduleModel();

    double pitchDeg() const;
    double rollDeg() const;
    double headingDeg() const;
    double turnRateDegPerSec() const;
    double slipSkidNormalized() const;
    double iasKt() const;
    double airspeedTrendKt() const;
    double tasKt() const;
    double aoaNormalized() const;
    double altitudeFt() const;
    double verticalSpeedFpm() const;
    double baroSettingInHg() const;
    void setBaroSettingInHg(double baroSettingInHg);
    double selectedAltitudeFt() const;
    void setSelectedAltitudeFt(double selectedAltitudeFt);

    double latitudeDeg() const;
    double longitudeDeg() const;
    double groundSpeedKt() const;
    double groundTrackDeg() const;
    QString utcTime() const;
    QString activeWaypointLabel() const;
    double waypointBearingDeg() const;
    double waypointDistanceNm() const;
    QString waypointEteLabel() const;
    QVariantList flightPlanLegs() const;

    double rpm() const;
    double manifoldPressureInHg() const;
    double oilPressurePsi() const;
    double oilTemperatureF() const;
    double fuelFlowGph() const;
    double fuelQuantityPercent() const;
    int fuelTankCount() const;
    void setFuelTankCount(int count);
    QStringList fuelTankLabels() const;
    QVariantList fuelTankStates() const;
    double fuelRemainingGal() const;
    double fuelUsedGal() const;
    double fuelEnduranceHours() const;
    double fuelRangeNm() const;
    double busVoltage() const;
    double chtMaxF() const;
    double egtMaxF() const;
    QVariantList chtValues() const;
    QVariantList egtValues() const;
    bool autopilotMaster() const;
    void setAutopilotMaster(bool enabled);
    bool flightDirectorEnabled() const;
    void setFlightDirectorEnabled(bool enabled);
    bool levelModeEnabled() const;
    void setLevelModeEnabled(bool enabled);
    QString autopilotLateralMode() const;
    void setAutopilotLateralMode(const QString &mode);
    QString autopilotVerticalMode() const;
    void setAutopilotVerticalMode(const QString &mode);

    bool gpsModuleAvailable() const;
    bool flightModuleAvailable() const;
    bool engineModuleAvailable() const;
    bool trafficModuleAvailable() const;

    bool flightDataHealthy() const;
    bool engineDataHealthy() const;
    bool trafficDataHealthy() const;

    QString primaryAlert() const;
    QString secondaryAlert() const;
    bool cautionActive() const;
    bool warningActive() const;
    QStringList cautionMessages() const;
    QStringList warningMessages() const;
    int cautionEventSerial() const;
    int warningEventSerial() const;

    int trafficCount() const;
    double nearestTrafficRangeNm() const;
    double nearestTrafficRelativeAltitudeFt() const;
    QVariantList trafficTargets() const;

    Q_INVOKABLE void nextPage();
    Q_INVOKABLE void previousPage();
    Q_INVOKABLE void setModuleEnabled(const QString &moduleKey, bool enabled);
    Q_INVOKABLE void adjustMapZoom(int direction);
    Q_INVOKABLE void resetMapZoom();
    Q_INVOKABLE void setDirectWaypoint(const QString &waypointIdent);
    Q_INVOKABLE void setFlightPlanWaypoints(const QVariantList &waypointIdentifiers);
    Q_INVOKABLE void clearFlightPlan();
    Q_INVOKABLE void saveCurrentFlightPlan(const QString &name);
    Q_INVOKABLE void loadStoredFlightPlan(const QString &id);
    Q_INVOKABLE QVariantList searchWaypoints(const QString &query, int limit = 12) const;
    Q_INVOKABLE QVariantList nearestAirports(int limit = 12) const;
    Q_INVOKABLE void setFuelTankLabel(int index, const QString &label);

signals:
    void currentPageChanged();
    void availablePagesChanged();
    void displayClassChanged();
    void mapConfigChanged();
    void configurationChanged();
    void scenarioChanged();
    void demoModeChanged();
    void flightDataChanged();
    void navDataChanged();
    void engineDataChanged();
    void trafficDataChanged();
    void alertsChanged();
    void moduleStateChanged();
    void autopilotChanged();

private:
    void seedModules();
    void applyScenarioPreset(const QString &scenario);
    void syncAvailablePages();
    void updateDemoFrame();
    void refreshAlerts();
    void configureFuelTanks(int count);
    void rebuildFuelTankStates();
    void updateAlertEvents(const QStringList &cautionMessages, const QStringList &warningMessages);
    bool modulePresent(const QString &moduleKey) const;
    bool moduleHealthy(const QString &moduleKey) const;
    QVariantMap resolveWaypoint(const QString &ident) const;

    ModuleListModel m_moduleModel;
    NavDataManager *m_navDataManager = nullptr;
    QTimer m_demoTimer;
    QString m_currentPage;
    QString m_displayClass = QStringLiteral("primary");
    QString m_mapBaseSource = QStringLiteral("faa_vfr");
    bool m_openAipOverlayEnabled = true;
    bool m_weatherOverlayEnabled = true;
    double m_weatherOverlayOpacity = 0.38;
    double m_mapZoomFactor = 1.0;
    QString m_tailNumber = QStringLiteral("N430G");
    QString m_scenario = QStringLiteral("cruise");
    QStringList m_availablePages;
    bool m_demoMode = true;
    double m_demoTimeSeconds = 0.0;

    double m_pitchDeg = 0.0;
    double m_rollDeg = 0.0;
    double m_headingDeg = 0.0;
    double m_turnRateDegPerSec = 0.0;
    double m_slipSkidNormalized = 0.0;
    double m_iasKt = 0.0;
    double m_airspeedTrendKt = 0.0;
    double m_tasKt = 0.0;
    double m_aoaNormalized = 0.0;
    double m_altitudeFt = 0.0;
    double m_verticalSpeedFpm = 0.0;
    double m_baroSettingInHg = 29.92;
    double m_selectedAltitudeFt = 7500.0;

    double m_latitudeDeg = 47.5400;
    double m_longitudeDeg = -122.3000;
    double m_groundSpeedKt = 0.0;
    double m_groundTrackDeg = 0.0;
    QString m_utcTime = QStringLiteral("12:00:00 Z");
    QString m_activeWaypointLabel = QStringLiteral("KBFI");
    double m_waypointBearingDeg = 43.0;
    double m_waypointDistanceNm = 18.9;
    QString m_waypointEteLabel = QStringLiteral("01:13");
    QVariantList m_flightPlanLegs;

    double m_rpm = 0.0;
    double m_manifoldPressureInHg = 0.0;
    double m_oilPressurePsi = 0.0;
    double m_oilTemperatureF = 0.0;
    double m_fuelFlowGph = 0.0;
    double m_fuelQuantityPercent = 0.0;
    int m_fuelTankCount = 2;
    QStringList m_fuelTankLabels;
    QVariantList m_fuelTankStates;
    double m_fuelRemainingGal = 0.0;
    double m_fuelUsedGal = 0.0;
    double m_fuelEnduranceHours = 0.0;
    double m_fuelRangeNm = 0.0;
    double m_busVoltage = 0.0;
    double m_chtMaxF = 0.0;
    double m_egtMaxF = 0.0;
    QVariantList m_chtValues;
    QVariantList m_egtValues;
    bool m_autopilotMaster = true;
    bool m_flightDirectorEnabled = true;
    bool m_levelModeEnabled = false;
    QString m_autopilotLateralMode = QStringLiteral("NAV");
    QString m_autopilotVerticalMode = QStringLiteral("ALT");

    QString m_primaryAlert;
    QString m_secondaryAlert;
    QStringList m_cautionMessages;
    QStringList m_warningMessages;
    int m_cautionEventSerial = 0;
    int m_warningEventSerial = 0;

    int m_trafficCount = 0;
    double m_nearestTrafficRangeNm = 0.0;
    double m_nearestTrafficRelativeAltitudeFt = 0.0;
    QVariantList m_trafficTargets;
    double m_previousIasKt = 0.0;
    QStringList m_userFlightPlanIdentifiers;
    QVariantList m_fuelTankCapacitiesGal;
    bool m_demoRouteOwnshipValid = false;
    double m_demoRouteLatitudeDeg = 0.0;
    double m_demoRouteLongitudeDeg = 0.0;
    double m_demoRouteHeadingDeg = 0.0;
};
