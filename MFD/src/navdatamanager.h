#pragma once

#include <QObject>
#include <QVariantList>

class NavDataManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList userWaypoints READ userWaypoints NOTIFY userWaypointsChanged)
    Q_PROPERTY(QVariantList storedFlightPlans READ storedFlightPlans NOTIFY storedFlightPlansChanged)
    Q_PROPERTY(QVariantList flightLogs READ flightLogs NOTIFY flightLogsChanged)
    Q_PROPERTY(QVariantList trackLogs READ trackLogs NOTIFY trackLogsChanged)

public:
    explicit NavDataManager(QObject *parent = nullptr);

    QVariantList userWaypoints() const;
    QVariantList storedFlightPlans() const;
    QVariantList flightLogs() const;
    QVariantList trackLogs() const;

    Q_INVOKABLE QVariantList searchWaypoints(const QString &query, int limit = 12) const;
    Q_INVOKABLE QVariantMap lookupWaypoint(const QString &ident) const;
    Q_INVOKABLE QVariantList nearestWaypoints(double latitudeDeg,
                                              double longitudeDeg,
                                              int limit = 12,
                                              bool airportsOnly = false) const;
    Q_INVOKABLE QVariantList referenceWaypointsInView(double southLat,
                                                      double westLon,
                                                      double northLat,
                                                      double eastLon,
                                                      int limit = 64) const;

    Q_INVOKABLE void addOrUpdateUserWaypoint(const QString &ident,
                                             const QString &name,
                                             double latitudeDeg,
                                             double longitudeDeg,
                                             const QString &notes = QString());
    Q_INVOKABLE void removeUserWaypoint(const QString &ident);

    Q_INVOKABLE void saveFlightPlan(const QString &name, const QVariantList &waypointIdentifiers);
    Q_INVOKABLE QVariantMap storedFlightPlan(const QString &id) const;
    Q_INVOKABLE QVariantList storedFlightPlanWaypoints(const QString &id) const;
    Q_INVOKABLE void deleteStoredFlightPlan(const QString &id);

    Q_INVOKABLE void deleteFlightLog(const QString &id);
    Q_INVOKABLE void deleteTrackLog(const QString &id);

signals:
    void userWaypointsChanged();
    void storedFlightPlansChanged();
    void flightLogsChanged();
    void trackLogsChanged();

private:
    void loadReferenceWaypoints();
    void loadPersistentStores();
    void rebuildWaypointIndex();
    void saveUserWaypoints() const;
    void saveStoredFlightPlans() const;
    void saveFlightLogs() const;
    void saveTrackLogs() const;
    QString dataDirectoryPath() const;
    static QString normalizedIdent(const QString &ident);

    QVariantList m_referenceWaypoints;
    QVariantList m_userWaypoints;
    QVariantList m_storedFlightPlans;
    QVariantList m_flightLogs;
    QVariantList m_trackLogs;
    QVariantMap m_waypointIndex;
};
