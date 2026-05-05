#include "chartpackagemanager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QUrl>
#include <QtGlobal>

#include <algorithm>
#include <cmath>

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kEarthSemiMajorAxis = 6378137.0;
constexpr double kEarthEccentricity = 0.0818191910428158;

double toRadians(double degrees)
{
    return degrees * kPi / 180.0;
}

double qJsonDouble(const QJsonObject &object, const char *key, double defaultValue = 0.0)
{
    return object.value(QLatin1String(key)).toDouble(defaultValue);
}

int qJsonInt(const QJsonObject &object, const char *key, int defaultValue = 0)
{
    return object.value(QLatin1String(key)).toInt(defaultValue);
}

QString qJsonString(const QJsonObject &object, const char *key, const QString &defaultValue = {})
{
    const QJsonValue value = object.value(QLatin1String(key));
    return value.isString() ? value.toString() : defaultValue;
}

double lccM(double latitudeRad)
{
    const double sinLat = std::sin(latitudeRad);
    return std::cos(latitudeRad)
        / std::sqrt(1.0 - std::pow(kEarthEccentricity * sinLat, 2.0));
}

double lccT(double latitudeRad)
{
    const double sinLat = std::sin(latitudeRad);
    const double numerator = std::tan(kPi / 4.0 - latitudeRad / 2.0);
    const double ratio = (1.0 - kEarthEccentricity * sinLat)
        / (1.0 + kEarthEccentricity * sinLat);
    return numerator / std::pow(ratio, kEarthEccentricity / 2.0);
}

} // namespace

ChartPackageManager::ChartPackageManager(QObject *parent)
    : QObject(parent)
    , m_chartsDirectory(defaultChartsDirectoryPath())
{
    refreshPackages();
}

QVariantList ChartPackageManager::visibleTiles() const
{
    return m_visibleTiles;
}

bool ChartPackageManager::chartLoaded() const
{
    return m_activePackageIndex >= 0 && m_activePackageIndex < m_packages.size();
}

QString ChartPackageManager::activeChartId() const
{
    return chartLoaded() ? m_packages.at(m_activePackageIndex).packageId : QString {};
}

QString ChartPackageManager::activeChartTitle() const
{
    return chartLoaded() ? m_packages.at(m_activePackageIndex).title : QString {};
}

QString ChartPackageManager::activeChartSource() const
{
    return chartLoaded() ? m_packages.at(m_activePackageIndex).sourceName : QString {};
}

QString ChartPackageManager::activeChartCycle() const
{
    return chartLoaded() ? m_packages.at(m_activePackageIndex).cycle : QString {};
}

QString ChartPackageManager::chartStatus() const
{
    return m_chartStatus;
}

QString ChartPackageManager::chartsDirectory() const
{
    return m_chartsDirectory;
}

void ChartPackageManager::setChartsDirectory(const QString &directory)
{
    const QString normalized = normalizedDirectoryPath(directory);
    if (normalized.isEmpty() || m_chartsDirectory == normalized) {
        return;
    }

    m_chartsDirectory = normalized;
    emit chartsDirectoryChanged();
    refreshPackages();
}

QString ChartPackageManager::preferredSourceKey() const
{
    return m_preferredSourceKey;
}

void ChartPackageManager::setPreferredSourceKey(const QString &sourceKey)
{
    if (sourceKey.isEmpty() || m_preferredSourceKey == sourceKey) {
        return;
    }

    m_preferredSourceKey = sourceKey;
    emit preferredSourceKeyChanged();
    rebuildVisibleTiles();
}

QStringList ChartPackageManager::availablePackageIds() const
{
    QStringList ids;
    ids.reserve(m_packages.size());
    for (const ChartPackage &chartPackage : m_packages) {
        ids << chartPackage.packageId;
    }
    return ids;
}

bool ChartPackageManager::usingPlaceholder() const
{
    return !chartLoaded();
}

void ChartPackageManager::refreshPackages()
{
    QVector<ChartPackage> packages;

    const QDir packagesDir(m_chartsDirectory);
    if (packagesDir.exists()) {
        const QFileInfoList packageDirs = packagesDir.entryInfoList(
            QDir::Dirs | QDir::NoDotAndDotDot,
            QDir::Name | QDir::IgnoreCase);

        for (const QFileInfo &packageDir : packageDirs) {
            const QString manifestPath = QDir(packageDir.absoluteFilePath()).filePath(QStringLiteral("manifest.json"));
            if (!QFileInfo::exists(manifestPath)) {
                continue;
            }

            ChartPackage chartPackage = loadPackageFromManifest(manifestPath);
            if (chartPackage.valid) {
                packages.push_back(chartPackage);
            }
        }
    }

    m_packages = packages;
    emit packagesChanged();
    rebuildVisibleTiles();
}

void ChartPackageManager::updateViewport(double viewportWidth,
                                         double viewportHeight,
                                         double centerLatitudeDeg,
                                         double centerLongitudeDeg,
                                         double zoomFactor)
{
    const bool changed = !qFuzzyCompare(m_viewportWidth + 1.0, viewportWidth + 1.0)
        || !qFuzzyCompare(m_viewportHeight + 1.0, viewportHeight + 1.0)
        || !qFuzzyCompare(m_centerLatitudeDeg + 1.0, centerLatitudeDeg + 1.0)
        || !qFuzzyCompare(m_centerLongitudeDeg + 1.0, centerLongitudeDeg + 1.0)
        || !qFuzzyCompare(m_zoomFactor + 1.0, zoomFactor + 1.0);

    if (!changed) {
        return;
    }

    m_viewportWidth = viewportWidth;
    m_viewportHeight = viewportHeight;
    m_centerLatitudeDeg = centerLatitudeDeg;
    m_centerLongitudeDeg = centerLongitudeDeg;
    m_zoomFactor = std::clamp(zoomFactor, 0.6, 3.0);
    rebuildVisibleTiles();
}

QVariantMap ChartPackageManager::projectCoordinateToViewport(double latitudeDeg, double longitudeDeg) const
{
    QVariantMap point;
    if (m_viewportWidth <= 1.0 || m_viewportHeight <= 1.0) {
        point.insert(QStringLiteral("valid"), false);
        return point;
    }

    const int activeIndex = chartLoaded()
        ? m_activePackageIndex
        : selectActivePackageIndex(m_centerLatitudeDeg, m_centerLongitudeDeg);
    if (activeIndex < 0 || activeIndex >= m_packages.size()) {
        const double lonScale = qMax(0.25, 1.0 / m_zoomFactor);
        const double latScale = qMax(0.18, 0.72 / m_zoomFactor);
        point.insert(QStringLiteral("x"), (m_viewportWidth / 2.0) + (longitudeDeg - m_centerLongitudeDeg) * (m_viewportWidth * lonScale));
        point.insert(QStringLiteral("y"), (m_viewportHeight / 2.0) - (latitudeDeg - m_centerLatitudeDeg) * (m_viewportHeight * latScale));
        point.insert(QStringLiteral("valid"), true);
        point.insert(QStringLiteral("inside"), true);
        return point;
    }

    const ChartPackage &chartPackage = m_packages.at(activeIndex);
    const Level *level = selectLevel(chartPackage);
    if (level == nullptr) {
        point.insert(QStringLiteral("valid"), false);
        return point;
    }

    double renderLatitude = m_centerLatitudeDeg;
    double renderLongitude = m_centerLongitudeDeg;
    if (!contains(chartPackage, renderLatitude, renderLongitude)) {
        renderLatitude = (chartPackage.northLat + chartPackage.southLat) / 2.0;
        renderLongitude = (chartPackage.eastLon + chartPackage.westLon) / 2.0;
    }

    const QPointF centerPixel = sourcePixelForCoordinate(chartPackage, *level, renderLatitude, renderLongitude);
    const QPointF targetPixel = sourcePixelForCoordinate(chartPackage, *level, latitudeDeg, longitudeDeg);
    const double effectiveZoom = std::clamp(m_zoomFactor, 0.6, 3.0);
    const double effectiveViewportWidth = m_viewportWidth / effectiveZoom;
    const double effectiveViewportHeight = m_viewportHeight / effectiveZoom;
    const double left = centerPixel.x() - effectiveViewportWidth / 2.0;
    const double top = centerPixel.y() - effectiveViewportHeight / 2.0;
    const double x = (targetPixel.x() - left) * effectiveZoom;
    const double y = (targetPixel.y() - top) * effectiveZoom;

    point.insert(QStringLiteral("x"), x);
    point.insert(QStringLiteral("y"), y);
    point.insert(QStringLiteral("valid"), true);
    point.insert(QStringLiteral("inside"), x >= -24.0 && x <= m_viewportWidth + 24.0 && y >= -24.0 && y <= m_viewportHeight + 24.0);
    return point;
}

QVariantMap ChartPackageManager::viewportBounds() const
{
    const double latSpan = 1.4 / qMax(0.6, m_zoomFactor);
    const double lonSpan = 2.0 / qMax(0.6, m_zoomFactor);
    QVariantMap bounds;
    bounds.insert(QStringLiteral("southLat"), m_centerLatitudeDeg - latSpan / 2.0);
    bounds.insert(QStringLiteral("northLat"), m_centerLatitudeDeg + latSpan / 2.0);
    bounds.insert(QStringLiteral("westLon"), m_centerLongitudeDeg - lonSpan / 2.0);
    bounds.insert(QStringLiteral("eastLon"), m_centerLongitudeDeg + lonSpan / 2.0);
    return bounds;
}

QString ChartPackageManager::defaultChartsDirectoryPath()
{
    const QString envPath = qEnvironmentVariable("PFD_CHARTS_DIR");
    if (!envPath.isEmpty()) {
        return normalizedDirectoryPath(envPath);
    }

#ifdef MFD_SOURCE_DIR
    const QString sourceTreePath = QDir(QStringLiteral(MFD_SOURCE_DIR)).filePath(QStringLiteral("charts/packages"));
    if (QDir(sourceTreePath).exists()) {
        return normalizedDirectoryPath(sourceTreePath);
    }
#endif

    const QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return normalizedDirectoryPath(QDir(appDataPath).filePath(QStringLiteral("charts/packages")));
}

ChartPackageManager::ChartPackage ChartPackageManager::loadPackageFromManifest(const QString &manifestPath)
{
    QFile file(manifestPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) {
        return {};
    }

    const QJsonObject root = document.object();
    ChartPackage chartPackage;
    chartPackage.packageId = qJsonString(root, "packageId", QFileInfo(manifestPath).dir().dirName());
    chartPackage.title = qJsonString(root, "title", chartPackage.packageId);
    chartPackage.sourceKey = qJsonString(root, "sourceKey", QStringLiteral("unknown"));
    chartPackage.sourceName = qJsonString(root, "sourceName", chartPackage.sourceKey);
    chartPackage.packageType = qJsonString(root, "packageType");
    chartPackage.cycle = qJsonString(root, "cycle");
    chartPackage.rootPath = QFileInfo(manifestPath).dir().absolutePath();
    chartPackage.defaultZoom = qJsonInt(root, "defaultZoom", 0);

    const QJsonObject bounds = root.value(QStringLiteral("bounds")).toObject();
    chartPackage.westLon = qJsonDouble(bounds, "westLon");
    chartPackage.eastLon = qJsonDouble(bounds, "eastLon");
    chartPackage.southLat = qJsonDouble(bounds, "southLat");
    chartPackage.northLat = qJsonDouble(bounds, "northLat");

    const QJsonObject worldFile = root.value(QStringLiteral("worldFile")).toObject();
    chartPackage.worldFile.a = qJsonDouble(worldFile, "a", 1.0);
    chartPackage.worldFile.d = qJsonDouble(worldFile, "d", 0.0);
    chartPackage.worldFile.b = qJsonDouble(worldFile, "b", 0.0);
    chartPackage.worldFile.e = qJsonDouble(worldFile, "e", -1.0);
    chartPackage.worldFile.c = qJsonDouble(worldFile, "c", 0.0);
    chartPackage.worldFile.f = qJsonDouble(worldFile, "f", 0.0);
    chartPackage.worldFile.valid = worldFile.contains(QStringLiteral("a"));

    const QJsonObject projection = root.value(QStringLiteral("projection")).toObject();
    chartPackage.lcc.latitudeOfOriginDeg = qJsonDouble(projection, "lat0");
    chartPackage.lcc.centralMeridianDeg = qJsonDouble(projection, "lon0");
    chartPackage.lcc.standardParallel1Deg = qJsonDouble(projection, "stdParallel1");
    chartPackage.lcc.standardParallel2Deg = qJsonDouble(projection, "stdParallel2");
    chartPackage.lcc.falseEastingM = qJsonDouble(projection, "falseEasting");
    chartPackage.lcc.falseNorthingM = qJsonDouble(projection, "falseNorthing");
    chartPackage.lcc.valid = qJsonString(projection, "type") == QStringLiteral("lambert_conformal_conic");

    const QJsonArray levels = root.value(QStringLiteral("levels")).toArray();
    for (const QJsonValue &levelValue : levels) {
        if (!levelValue.isObject()) {
            continue;
        }

        const QJsonObject levelObject = levelValue.toObject();
        Level level;
        level.id = qJsonString(levelObject, "id");
        level.path = qJsonString(levelObject, "path");
        level.format = qJsonString(levelObject, "format", QStringLiteral("jpg"));
        level.scale = qJsonDouble(levelObject, "scale", 1.0);
        level.width = qJsonInt(levelObject, "width");
        level.height = qJsonInt(levelObject, "height");
        level.tileSize = qJsonInt(levelObject, "tileSize", 256);
        level.rows = qJsonInt(levelObject, "rows");
        level.cols = qJsonInt(levelObject, "cols");
        level.minColumn = qJsonInt(levelObject, "minColumn", 0);
        level.maxColumn = qJsonInt(levelObject, "maxColumn", level.cols > 0 ? level.cols - 1 : 0);
        level.minRow = qJsonInt(levelObject, "minRow", 0);
        level.maxRow = qJsonInt(levelObject, "maxRow", level.rows > 0 ? level.rows - 1 : 0);
        level.zoom = qJsonInt(levelObject, "zoom", 0);

        if (level.path.isEmpty() || level.rows <= 0 || level.cols <= 0 || level.tileSize <= 0) {
            continue;
        }

        chartPackage.levels.push_back(level);
    }

    chartPackage.valid = !chartPackage.packageId.isEmpty()
        && !chartPackage.packageType.isEmpty()
        && !chartPackage.levels.isEmpty();
    return chartPackage;
}

QPointF ChartPackageManager::projectLambertConformalConic(double latitudeDeg,
                                                          double longitudeDeg,
                                                          const LambertConformalConic &projection)
{
    if (!projection.valid) {
        return {};
    }

    const double latitudeRad = toRadians(latitudeDeg);
    const double longitudeRad = toRadians(longitudeDeg);
    const double latitudeOriginRad = toRadians(projection.latitudeOfOriginDeg);
    const double centralMeridianRad = toRadians(projection.centralMeridianDeg);
    const double standardParallel1Rad = toRadians(projection.standardParallel1Deg);
    const double standardParallel2Rad = toRadians(projection.standardParallel2Deg);

    const double m1 = lccM(standardParallel1Rad);
    const double m2 = lccM(standardParallel2Rad);
    const double t1 = lccT(standardParallel1Rad);
    const double t2 = lccT(standardParallel2Rad);
    const double t = lccT(latitudeRad);
    const double t0 = lccT(latitudeOriginRad);

    const double n = (std::log(m1) - std::log(m2)) / (std::log(t1) - std::log(t2));
    const double f = m1 / (n * std::pow(t1, n));
    const double rho = kEarthSemiMajorAxis * f * std::pow(t, n);
    const double rho0 = kEarthSemiMajorAxis * f * std::pow(t0, n);
    const double theta = n * (longitudeRad - centralMeridianRad);

    return {
        projection.falseEastingM + rho * std::sin(theta),
        projection.falseNorthingM + rho0 - rho * std::cos(theta)
    };
}

QPointF ChartPackageManager::inverseAffine(const QPointF &worldPoint, const WorldFile &worldFile)
{
    const double det = worldFile.a * worldFile.e - worldFile.b * worldFile.d;
    if (qFuzzyIsNull(det)) {
        return {};
    }

    const double deltaX = worldPoint.x() - worldFile.c;
    const double deltaY = worldPoint.y() - worldFile.f;
    const double pixelX = (worldFile.e * deltaX - worldFile.b * deltaY) / det;
    const double pixelY = (-worldFile.d * deltaX + worldFile.a * deltaY) / det;
    return { pixelX, pixelY };
}

QPointF ChartPackageManager::mercatorPixel(double latitudeDeg, double longitudeDeg, int zoom, int tileSize)
{
    const double clampedLat = std::clamp(latitudeDeg, -85.05112878, 85.05112878);
    const double sinLat = std::sin(toRadians(clampedLat));
    const double scale = tileSize * std::pow(2.0, zoom);
    const double pixelX = ((longitudeDeg + 180.0) / 360.0) * scale;
    const double pixelY = (0.5 - std::log((1.0 + sinLat) / (1.0 - sinLat)) / (4.0 * kPi)) * scale;
    return { pixelX, pixelY };
}

bool ChartPackageManager::contains(const ChartPackage &chartPackage, double latitudeDeg, double longitudeDeg)
{
    return longitudeDeg >= std::min(chartPackage.westLon, chartPackage.eastLon)
        && longitudeDeg <= std::max(chartPackage.westLon, chartPackage.eastLon)
        && latitudeDeg >= std::min(chartPackage.southLat, chartPackage.northLat)
        && latitudeDeg <= std::max(chartPackage.southLat, chartPackage.northLat);
}

QString ChartPackageManager::tileUrlForLevel(const ChartPackage &chartPackage,
                                             const Level &level,
                                             int column,
                                             int row)
{
    QDir packageDir(chartPackage.rootPath);
    QString relativePath;

    if (chartPackage.packageType == QStringLiteral("mercator_tiles")) {
        relativePath = QStringLiteral("%1/%2/%3.%4")
            .arg(level.path)
            .arg(column)
            .arg(row)
            .arg(level.format);
    } else {
        relativePath = QStringLiteral("%1/%2_%3.%4")
            .arg(level.path)
            .arg(column)
            .arg(row)
            .arg(level.format);
    }

    return QUrl::fromLocalFile(packageDir.filePath(relativePath)).toString();
}

QString ChartPackageManager::normalizedDirectoryPath(const QString &directory)
{
    if (directory.isEmpty()) {
        return {};
    }

    return QDir(directory).absolutePath();
}

QPointF ChartPackageManager::sourcePixelForCoordinate(const ChartPackage &chartPackage,
                                                      const Level &level,
                                                      double latitudeDeg,
                                                      double longitudeDeg) const
{
    if (chartPackage.packageType == QStringLiteral("lcc_affine_tiles")
        && chartPackage.worldFile.valid
        && chartPackage.lcc.valid) {
        const QPointF projected = projectLambertConformalConic(latitudeDeg, longitudeDeg, chartPackage.lcc);
        const QPointF sourcePixel = inverseAffine(projected, chartPackage.worldFile);
        return QPointF(sourcePixel.x() * level.scale, sourcePixel.y() * level.scale);
    }

    if (chartPackage.packageType == QStringLiteral("mercator_tiles")) {
        return mercatorPixel(latitudeDeg, longitudeDeg, level.zoom, level.tileSize);
    }

    return {};
}

int ChartPackageManager::selectActivePackageIndex(double latitudeDeg, double longitudeDeg) const
{
    if (m_preferredSourceKey == QStringLiteral("demo")) {
        return -1;
    }

    int fallbackIndex = -1;
    double fallbackArea = 0.0;

    for (int index = 0; index < m_packages.size(); ++index) {
        const ChartPackage &chartPackage = m_packages.at(index);
        if (chartPackage.sourceKey != m_preferredSourceKey) {
            continue;
        }

        if (!contains(chartPackage, latitudeDeg, longitudeDeg)) {
            if (fallbackIndex < 0) {
                fallbackIndex = index;
            }
            continue;
        }

        const double area = std::abs((chartPackage.eastLon - chartPackage.westLon)
            * (chartPackage.northLat - chartPackage.southLat));
        if (fallbackIndex < 0 || area < fallbackArea || qFuzzyIsNull(fallbackArea)) {
            fallbackIndex = index;
            fallbackArea = area;
        }
    }

    return fallbackIndex;
}

const ChartPackageManager::Level *ChartPackageManager::selectLevel(const ChartPackage &chartPackage) const
{
    if (chartPackage.levels.isEmpty()) {
        return nullptr;
    }

    if (chartPackage.packageType == QStringLiteral("mercator_tiles")) {
        for (const Level &level : chartPackage.levels) {
            if (level.zoom == chartPackage.defaultZoom) {
                return &level;
            }
        }
    }

    return &chartPackage.levels.constLast();
}

void ChartPackageManager::rebuildVisibleTiles()
{
    const int newActiveIndex = selectActivePackageIndex(m_centerLatitudeDeg, m_centerLongitudeDeg);
    const bool activeChanged = newActiveIndex != m_activePackageIndex;
    m_activePackageIndex = newActiveIndex;

    QVariantList newTiles;

    if (chartLoaded() && m_viewportWidth > 1.0 && m_viewportHeight > 1.0) {
        const ChartPackage &chartPackage = m_packages.at(m_activePackageIndex);
        const Level *level = selectLevel(chartPackage);

        if (level != nullptr) {
            double renderLatitude = m_centerLatitudeDeg;
            double renderLongitude = m_centerLongitudeDeg;

            if (!contains(chartPackage, renderLatitude, renderLongitude)) {
                renderLatitude = (chartPackage.northLat + chartPackage.southLat) / 2.0;
                renderLongitude = (chartPackage.eastLon + chartPackage.westLon) / 2.0;
            }

            const QPointF centerPixel = sourcePixelForCoordinate(chartPackage, *level, renderLatitude, renderLongitude);

            const double effectiveZoom = std::clamp(m_zoomFactor, 0.6, 3.0);
            const double effectiveViewportWidth = m_viewportWidth / effectiveZoom;
            const double effectiveViewportHeight = m_viewportHeight / effectiveZoom;
            const double left = centerPixel.x() - effectiveViewportWidth / 2.0;
            const double top = centerPixel.y() - effectiveViewportHeight / 2.0;
            const int firstColumn = std::max(level->minColumn, static_cast<int>(std::floor(left / level->tileSize)));
            const int lastColumn = std::min(level->maxColumn, static_cast<int>(std::floor((left + effectiveViewportWidth) / level->tileSize)));
            const int firstRow = std::max(level->minRow, static_cast<int>(std::floor(top / level->tileSize)));
            const int lastRow = std::min(level->maxRow, static_cast<int>(std::floor((top + effectiveViewportHeight) / level->tileSize)));

            for (int row = firstRow; row <= lastRow; ++row) {
                for (int column = firstColumn; column <= lastColumn; ++column) {
                    QVariantMap tile;
                    tile.insert(QStringLiteral("sourceUrl"), tileUrlForLevel(chartPackage, *level, column, row));
                    tile.insert(QStringLiteral("x"), (column * level->tileSize - left) * effectiveZoom);
                    tile.insert(QStringLiteral("y"), (row * level->tileSize - top) * effectiveZoom);
                    tile.insert(QStringLiteral("width"), level->tileSize * effectiveZoom);
                    tile.insert(QStringLiteral("height"), level->tileSize * effectiveZoom);
                    newTiles.push_back(tile);
                }
            }
        }
    }

    if (m_visibleTiles != newTiles) {
        m_visibleTiles = newTiles;
        emit visibleTilesChanged();
    }

    updateChartStatus();
    if (activeChanged) {
        emit chartChanged();
    } else {
        emit chartChanged();
    }
}

void ChartPackageManager::clearVisibleTiles()
{
    if (m_visibleTiles.isEmpty()) {
        return;
    }

    m_visibleTiles.clear();
    emit visibleTilesChanged();
}

void ChartPackageManager::updateChartStatus()
{
    QString status;

    if (m_preferredSourceKey == QStringLiteral("demo")) {
        status = QStringLiteral("Demo preview map active. No local aviation chart package selected.");
    } else if (chartLoaded()) {
        const ChartPackage &chartPackage = m_packages.at(m_activePackageIndex);
        const QString locationNote = contains(chartPackage, m_centerLatitudeDeg, m_centerLongitudeDeg)
            ? QString {}
            : QStringLiteral("  |  Current position outside chart coverage");
        status = QStringLiteral("Loaded %1 chart package: %2%3%4")
            .arg(chartPackage.sourceName, chartPackage.title,
                 chartPackage.cycle.isEmpty() ? QString {} : QStringLiteral("  |  Cycle %1").arg(chartPackage.cycle),
                 locationNote);
    } else if (m_packages.isEmpty()) {
        status = QStringLiteral("No chart packages found in %1").arg(m_chartsDirectory);
    } else {
        status = QStringLiteral("No %1 chart package matches the current location. Packages directory: %2")
            .arg(m_preferredSourceKey, m_chartsDirectory);
    }

    if (m_chartStatus == status) {
        return;
    }

    m_chartStatus = status;
}
