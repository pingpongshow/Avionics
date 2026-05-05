#include "navdatamanager.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QtGlobal>

#include <algorithm>
#include <cmath>

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kEarthRadiusNm = 3440.065;

double toRadians(double degrees)
{
    return degrees * kPi / 180.0;
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

QVariantList loadVariantArrayFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isArray()) {
        return {};
    }

    return document.array().toVariantList();
}

bool saveVariantArrayFile(const QString &path, const QVariantList &items)
{
    QDir dir(QFileInfo(path).absolutePath());
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    file.write(QJsonDocument(QJsonArray::fromVariantList(items)).toJson(QJsonDocument::Indented));
    return true;
}

QString sourceDataFilePath(const QString &relativePath)
{
#ifdef MFD_SOURCE_DIR
    return QDir(QStringLiteral(MFD_SOURCE_DIR)).filePath(relativePath);
#else
    Q_UNUSED(relativePath);
    return {};
#endif
}

QString firstExistingPath(const QStringList &candidates)
{
    for (const QString &candidate : candidates) {
        if (!candidate.isEmpty() && QFileInfo::exists(candidate)) {
            return candidate;
        }
    }
    return {};
}

QStringList normalizedAliases(const QVariantMap &item)
{
    QStringList aliases;
    const auto appendAlias = [&aliases](const QString &value) {
        const QString normalized = value.trimmed().toUpper();
        if (!normalized.isEmpty() && !aliases.contains(normalized)) {
            aliases.push_back(normalized);
        }
    };

    appendAlias(item.value(QStringLiteral("ident")).toString());

    const QVariant aliasesValue = item.value(QStringLiteral("aliases"));
    if (aliasesValue.metaType().id() == QMetaType::QStringList) {
        for (const QString &alias : aliasesValue.toStringList()) {
            appendAlias(alias);
        }
    } else {
        for (const QVariant &alias : aliasesValue.toList()) {
            appendAlias(alias.toString());
        }
    }

    return aliases;
}

int itemPriority(const QVariantMap &item)
{
    if (item.value(QStringLiteral("userDefined")).toBool()) {
        return 400;
    }

    const QString symbolType = item.value(QStringLiteral("symbolType")).toString().trimmed().toLower();
    if (symbolType == QStringLiteral("airport")) {
        return 320;
    }
    if (symbolType == QStringLiteral("navaid")) {
        return 260;
    }
    if (symbolType == QStringLiteral("fix")) {
        return 220;
    }
    return 160;
}

bool queryMatches(const QVariantMap &item, const QString &normalizedQuery)
{
    if (normalizedQuery.isEmpty()) {
        return true;
    }

    for (const QString &alias : normalizedAliases(item)) {
        if (alias.contains(normalizedQuery)) {
            return true;
        }
    }

    return item.value(QStringLiteral("name")).toString().trimmed().toUpper().contains(normalizedQuery);
}

int queryScore(const QVariantMap &item, const QString &normalizedQuery)
{
    if (normalizedQuery.isEmpty()) {
        return itemPriority(item);
    }

    int bestScore = 0;
    for (const QString &alias : normalizedAliases(item)) {
        if (alias == normalizedQuery) {
            bestScore = std::max(bestScore, 500);
        } else if (alias.startsWith(normalizedQuery)) {
            bestScore = std::max(bestScore, 420);
        } else if (alias.contains(normalizedQuery)) {
            bestScore = std::max(bestScore, 300);
        }
    }

    const QString upperName = item.value(QStringLiteral("name")).toString().trimmed().toUpper();
    if (upperName.startsWith(normalizedQuery)) {
        bestScore = std::max(bestScore, 240);
    } else if (upperName.contains(normalizedQuery)) {
        bestScore = std::max(bestScore, 180);
    }

    return bestScore + itemPriority(item);
}

} // namespace

NavDataManager::NavDataManager(QObject *parent)
    : QObject(parent)
{
    loadReferenceWaypoints();
    loadPersistentStores();
    rebuildWaypointIndex();
}

QVariantList NavDataManager::userWaypoints() const
{
    return m_userWaypoints;
}

QVariantList NavDataManager::storedFlightPlans() const
{
    return m_storedFlightPlans;
}

QVariantList NavDataManager::flightLogs() const
{
    return m_flightLogs;
}

QVariantList NavDataManager::trackLogs() const
{
    return m_trackLogs;
}

QVariantList NavDataManager::searchWaypoints(const QString &query, int limit) const
{
    const QString normalizedQuery = query.trimmed().toUpper();
    struct Candidate {
        int score = 0;
        QVariantMap item;
    };

    QVector<Candidate> candidates;
    candidates.reserve(m_referenceWaypoints.size() + m_userWaypoints.size());

    const auto appendMatches = [&](const QVariantList &items, bool userDefined) {
        for (const QVariant &value : items) {
            QVariantMap item = value.toMap();
            item.insert(QStringLiteral("userDefined"), userDefined);
            if (!queryMatches(item, normalizedQuery)) {
                continue;
            }
            candidates.push_back({ queryScore(item, normalizedQuery), item });
        }
    };

    appendMatches(m_referenceWaypoints, false);
    appendMatches(m_userWaypoints, true);

    std::sort(candidates.begin(), candidates.end(), [](const Candidate &left, const Candidate &right) {
        if (left.score != right.score) {
            return left.score > right.score;
        }
        return left.item.value(QStringLiteral("ident")).toString() < right.item.value(QStringLiteral("ident")).toString();
    });

    QVariantList matches;
    QStringList emittedKeys;
    const int maxResults = std::max(1, limit);
    for (const Candidate &candidate : std::as_const(candidates)) {
        const QString key = candidate.item.value(QStringLiteral("ident")).toString()
            + QLatin1Char('|')
            + candidate.item.value(QStringLiteral("name")).toString();
        if (emittedKeys.contains(key)) {
            continue;
        }
        emittedKeys.push_back(key);
        matches.push_back(candidate.item);
        if (matches.size() >= maxResults) {
            break;
        }
    }
    return matches;
}

QVariantMap NavDataManager::lookupWaypoint(const QString &ident) const
{
    return m_waypointIndex.value(normalizedIdent(ident)).toMap();
}

QVariantList NavDataManager::nearestWaypoints(double latitudeDeg,
                                              double longitudeDeg,
                                              int limit,
                                              bool airportsOnly) const
{
    struct Candidate {
        double distanceNm = 0.0;
        QVariantMap item;
    };

    QVector<Candidate> candidates;
    candidates.reserve(m_referenceWaypoints.size() + m_userWaypoints.size());

    const auto appendCandidates = [&](const QVariantList &items, bool userDefined) {
        for (const QVariant &value : items) {
            QVariantMap item = value.toMap();
            const QString symbolType = item.value(QStringLiteral("symbolType")).toString().trimmed().toLower();
            if (airportsOnly && symbolType != QStringLiteral("airport")) {
                continue;
            }

            const double waypointLatitudeDeg = item.value(QStringLiteral("latitudeDeg")).toDouble();
            const double waypointLongitudeDeg = item.value(QStringLiteral("longitudeDeg")).toDouble();
            if (!std::isfinite(waypointLatitudeDeg) || !std::isfinite(waypointLongitudeDeg)) {
                continue;
            }

            item.insert(QStringLiteral("userDefined"), userDefined);
            item.insert(QStringLiteral("distanceNm"),
                        haversineDistanceNm(latitudeDeg, longitudeDeg, waypointLatitudeDeg, waypointLongitudeDeg));
            candidates.push_back({ item.value(QStringLiteral("distanceNm")).toDouble(), item });
        }
    };

    appendCandidates(m_referenceWaypoints, false);
    appendCandidates(m_userWaypoints, true);

    std::sort(candidates.begin(), candidates.end(), [](const Candidate &left, const Candidate &right) {
        if (!qFuzzyCompare(left.distanceNm + 1.0, right.distanceNm + 1.0)) {
            return left.distanceNm < right.distanceNm;
        }
        return left.item.value(QStringLiteral("ident")).toString() < right.item.value(QStringLiteral("ident")).toString();
    });

    QVariantList matches;
    const int maxResults = std::max(1, limit);
    for (const Candidate &candidate : std::as_const(candidates)) {
        matches.push_back(candidate.item);
        if (matches.size() >= maxResults) {
            break;
        }
    }
    return matches;
}

QVariantList NavDataManager::referenceWaypointsInView(double southLat,
                                                      double westLon,
                                                      double northLat,
                                                      double eastLon,
                                                      int limit) const
{
    const double minLat = qMin(southLat, northLat);
    const double maxLat = qMax(southLat, northLat);
    const double minLon = qMin(westLon, eastLon);
    const double maxLon = qMax(westLon, eastLon);
    const double centerLat = (minLat + maxLat) / 2.0;
    const double centerLon = (minLon + maxLon) / 2.0;

    struct VisibleItem {
        int priority = 0;
        double centerDistance = 0.0;
        QVariantMap item;
    };

    QVector<VisibleItem> visibleItems;
    visibleItems.reserve(256);

    const auto appendVisible = [&](const QVariantList &source, bool userDefined) {
        for (const QVariant &value : source) {
            QVariantMap item = value.toMap();
            const double lat = item.value(QStringLiteral("latitudeDeg")).toDouble();
            const double lon = item.value(QStringLiteral("longitudeDeg")).toDouble();
            if (lat < minLat || lat > maxLat || lon < minLon || lon > maxLon) {
                continue;
            }

            item.insert(QStringLiteral("userDefined"), userDefined);
            const double latDelta = lat - centerLat;
            const double lonDelta = (lon - centerLon) * std::cos(centerLat * kPi / 180.0);
            visibleItems.push_back({
                itemPriority(item),
                (latDelta * latDelta) + (lonDelta * lonDelta),
                item
            });
        }
    };

    appendVisible(m_referenceWaypoints, false);
    appendVisible(m_userWaypoints, true);

    std::sort(visibleItems.begin(), visibleItems.end(), [](const VisibleItem &left, const VisibleItem &right) {
        if (left.priority != right.priority) {
            return left.priority > right.priority;
        }
        if (!qFuzzyCompare(left.centerDistance + 1.0, right.centerDistance + 1.0)) {
            return left.centerDistance < right.centerDistance;
        }
        return left.item.value(QStringLiteral("ident")).toString() < right.item.value(QStringLiteral("ident")).toString();
    });

    QVariantList items;
    const int maxResults = std::max(1, limit);
    for (const VisibleItem &visibleItem : std::as_const(visibleItems)) {
        items.push_back(visibleItem.item);
        if (items.size() >= maxResults) {
            break;
        }
    }
    return items;
}

void NavDataManager::addOrUpdateUserWaypoint(const QString &ident,
                                             const QString &name,
                                             double latitudeDeg,
                                             double longitudeDeg,
                                             const QString &notes)
{
    const QString normalized = normalizedIdent(ident);
    if (normalized.isEmpty()) {
        return;
    }

    QVariantMap waypoint;
    waypoint.insert(QStringLiteral("ident"), normalized);
    waypoint.insert(QStringLiteral("name"), name.trimmed().isEmpty() ? normalized : name.trimmed());
    waypoint.insert(QStringLiteral("type"), QStringLiteral("user_waypoint"));
    waypoint.insert(QStringLiteral("latitudeDeg"), latitudeDeg);
    waypoint.insert(QStringLiteral("longitudeDeg"), longitudeDeg);
    waypoint.insert(QStringLiteral("notes"), notes.trimmed());

    bool updated = false;
    for (QVariant &value : m_userWaypoints) {
        QVariantMap item = value.toMap();
        if (normalizedIdent(item.value(QStringLiteral("ident")).toString()) == normalized) {
            value = waypoint;
            updated = true;
            break;
        }
    }

    if (!updated) {
        m_userWaypoints.push_back(waypoint);
    }

    rebuildWaypointIndex();
    saveUserWaypoints();
    emit userWaypointsChanged();
}

void NavDataManager::removeUserWaypoint(const QString &ident)
{
    const QString normalized = normalizedIdent(ident);
    for (qsizetype index = 0; index < m_userWaypoints.size(); ++index) {
        if (normalizedIdent(m_userWaypoints.at(index).toMap().value(QStringLiteral("ident")).toString()) == normalized) {
            m_userWaypoints.removeAt(index);
            rebuildWaypointIndex();
            saveUserWaypoints();
            emit userWaypointsChanged();
            return;
        }
    }
}

void NavDataManager::saveFlightPlan(const QString &name, const QVariantList &waypointIdentifiers)
{
    QVariantList identifiers;
    for (const QVariant &value : waypointIdentifiers) {
        const QString ident = normalizedIdent(value.toString());
        if (!ident.isEmpty()) {
            identifiers.push_back(ident);
        }
    }

    if (identifiers.isEmpty()) {
        return;
    }

    QVariantMap plan;
    plan.insert(QStringLiteral("id"), QStringLiteral("fpl-%1").arg(QDateTime::currentMSecsSinceEpoch()));
    plan.insert(QStringLiteral("name"), name.trimmed().isEmpty() ? QStringLiteral("Flight Plan %1").arg(m_storedFlightPlans.size() + 1) : name.trimmed());
    plan.insert(QStringLiteral("createdAt"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    plan.insert(QStringLiteral("waypoints"), identifiers);
    m_storedFlightPlans.push_front(plan);

    saveStoredFlightPlans();
    emit storedFlightPlansChanged();
}

QVariantMap NavDataManager::storedFlightPlan(const QString &id) const
{
    for (const QVariant &value : m_storedFlightPlans) {
        const QVariantMap item = value.toMap();
        if (item.value(QStringLiteral("id")).toString() == id) {
            return item;
        }
    }
    return {};
}

QVariantList NavDataManager::storedFlightPlanWaypoints(const QString &id) const
{
    return storedFlightPlan(id).value(QStringLiteral("waypoints")).toList();
}

void NavDataManager::deleteStoredFlightPlan(const QString &id)
{
    for (qsizetype index = 0; index < m_storedFlightPlans.size(); ++index) {
        if (m_storedFlightPlans.at(index).toMap().value(QStringLiteral("id")).toString() == id) {
            m_storedFlightPlans.removeAt(index);
            saveStoredFlightPlans();
            emit storedFlightPlansChanged();
            return;
        }
    }
}

void NavDataManager::deleteFlightLog(const QString &id)
{
    for (qsizetype index = 0; index < m_flightLogs.size(); ++index) {
        if (m_flightLogs.at(index).toMap().value(QStringLiteral("id")).toString() == id) {
            m_flightLogs.removeAt(index);
            saveFlightLogs();
            emit flightLogsChanged();
            return;
        }
    }
}

void NavDataManager::deleteTrackLog(const QString &id)
{
    for (qsizetype index = 0; index < m_trackLogs.size(); ++index) {
        if (m_trackLogs.at(index).toMap().value(QStringLiteral("id")).toString() == id) {
            m_trackLogs.removeAt(index);
            saveTrackLogs();
            emit trackLogsChanged();
            return;
        }
    }
}

void NavDataManager::loadReferenceWaypoints()
{
    const QString envPath = qEnvironmentVariable("PFD_REFERENCE_WAYPOINTS");
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString sourcePath = sourceDataFilePath(QStringLiteral("data/reference_waypoints.json"));
    const QString appDataPath = QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
        .filePath(QStringLiteral("reference_waypoints.json"));
    const QString runtimePath = firstExistingPath({
        envPath,
        sourcePath,
        QDir(appDir).filePath(QStringLiteral("reference_waypoints.json")),
        QDir(appDir).filePath(QStringLiteral("../Resources/reference_waypoints.json")),
        appDataPath
    });
    m_referenceWaypoints = loadVariantArrayFile(runtimePath);
}

void NavDataManager::loadPersistentStores()
{
    const QString base = dataDirectoryPath();
    m_userWaypoints = loadVariantArrayFile(QDir(base).filePath(QStringLiteral("user_waypoints.json")));
    m_storedFlightPlans = loadVariantArrayFile(QDir(base).filePath(QStringLiteral("stored_flight_plans.json")));
    m_flightLogs = loadVariantArrayFile(QDir(base).filePath(QStringLiteral("flight_logs.json")));
    m_trackLogs = loadVariantArrayFile(QDir(base).filePath(QStringLiteral("track_logs.json")));

    if (m_storedFlightPlans.isEmpty()) {
        QVariantMap plan;
        plan.insert(QStringLiteral("id"), QStringLiteral("fpl-seattle-demo"));
        plan.insert(QStringLiteral("name"), QStringLiteral("Seattle Demo Route"));
        plan.insert(QStringLiteral("createdAt"), QStringLiteral("2026-05-05T17:00:00Z"));
        plan.insert(QStringLiteral("waypoints"), QVariantList { QStringLiteral("KBFI"), QStringLiteral("KPAE"), QStringLiteral("KORS") });
        m_storedFlightPlans = { plan };
        saveStoredFlightPlans();
    }

    if (m_flightLogs.isEmpty()) {
        QVariantMap log;
        log.insert(QStringLiteral("id"), QStringLiteral("flight-log-20260505-1"));
        log.insert(QStringLiteral("name"), QStringLiteral("Bench Demo 2026-05-05"));
        log.insert(QStringLiteral("startedAt"), QStringLiteral("2026-05-05T17:29:00Z"));
        log.insert(QStringLiteral("duration"), QStringLiteral("01:12"));
        log.insert(QStringLiteral("route"), QStringLiteral("KBFI -> KPAE"));
        log.insert(QStringLiteral("size"), QStringLiteral("18.2 MB"));
        m_flightLogs = { log };
        saveFlightLogs();
    }

    if (m_trackLogs.isEmpty()) {
        QVariantMap log;
        log.insert(QStringLiteral("id"), QStringLiteral("track-log-20260505-1"));
        log.insert(QStringLiteral("name"), QStringLiteral("Puget Sound Track"));
        log.insert(QStringLiteral("recordedAt"), QStringLiteral("2026-05-05T17:29:00Z"));
        log.insert(QStringLiteral("distance"), QStringLiteral("94 NM"));
        log.insert(QStringLiteral("duration"), QStringLiteral("01:12"));
        m_trackLogs = { log };
        saveTrackLogs();
    }
}

void NavDataManager::rebuildWaypointIndex()
{
    m_waypointIndex.clear();

    const auto appendItems = [this](const QVariantList &items, bool userDefined) {
        for (const QVariant &value : items) {
            QVariantMap item = value.toMap();
            const QString ident = normalizedIdent(item.value(QStringLiteral("ident")).toString());
            if (ident.isEmpty()) {
                continue;
            }
            item.insert(QStringLiteral("ident"), ident);
            item.insert(QStringLiteral("userDefined"), userDefined);
            const QVariantMap existing = m_waypointIndex.value(ident).toMap();
            if (existing.isEmpty() || itemPriority(item) >= itemPriority(existing)) {
                m_waypointIndex.insert(ident, item);
            }

            for (const QString &alias : normalizedAliases(item)) {
                const QVariantMap existingAlias = m_waypointIndex.value(alias).toMap();
                if (existingAlias.isEmpty() || itemPriority(item) >= itemPriority(existingAlias)) {
                    m_waypointIndex.insert(alias, item);
                }
            }
        }
    };

    appendItems(m_referenceWaypoints, false);
    appendItems(m_userWaypoints, true);
}

void NavDataManager::saveUserWaypoints() const
{
    saveVariantArrayFile(QDir(dataDirectoryPath()).filePath(QStringLiteral("user_waypoints.json")), m_userWaypoints);
}

void NavDataManager::saveStoredFlightPlans() const
{
    saveVariantArrayFile(QDir(dataDirectoryPath()).filePath(QStringLiteral("stored_flight_plans.json")), m_storedFlightPlans);
}

void NavDataManager::saveFlightLogs() const
{
    saveVariantArrayFile(QDir(dataDirectoryPath()).filePath(QStringLiteral("flight_logs.json")), m_flightLogs);
}

void NavDataManager::saveTrackLogs() const
{
    saveVariantArrayFile(QDir(dataDirectoryPath()).filePath(QStringLiteral("track_logs.json")), m_trackLogs);
}

QString NavDataManager::dataDirectoryPath() const
{
    const QString envPath = qEnvironmentVariable("PFD_DATA_DIR");
    if (!envPath.isEmpty()) {
        return QDir(envPath).absolutePath();
    }
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).filePath(QStringLiteral("db"));
}

QString NavDataManager::normalizedIdent(const QString &ident)
{
    return ident.trimmed().toUpper();
}
