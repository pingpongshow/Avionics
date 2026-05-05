#pragma once

#include <QObject>
#include <QPointF>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QVariantList>
#include <QVector>

class ChartPackageManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList visibleTiles READ visibleTiles NOTIFY visibleTilesChanged)
    Q_PROPERTY(bool chartLoaded READ chartLoaded NOTIFY chartChanged)
    Q_PROPERTY(QString activeChartId READ activeChartId NOTIFY chartChanged)
    Q_PROPERTY(QString activeChartTitle READ activeChartTitle NOTIFY chartChanged)
    Q_PROPERTY(QString activeChartSource READ activeChartSource NOTIFY chartChanged)
    Q_PROPERTY(QString activeChartCycle READ activeChartCycle NOTIFY chartChanged)
    Q_PROPERTY(QString chartStatus READ chartStatus NOTIFY chartChanged)
    Q_PROPERTY(QString chartsDirectory READ chartsDirectory WRITE setChartsDirectory NOTIFY chartsDirectoryChanged)
    Q_PROPERTY(QString preferredSourceKey READ preferredSourceKey WRITE setPreferredSourceKey NOTIFY preferredSourceKeyChanged)
    Q_PROPERTY(QStringList availablePackageIds READ availablePackageIds NOTIFY packagesChanged)
    Q_PROPERTY(bool usingPlaceholder READ usingPlaceholder NOTIFY chartChanged)

public:
    explicit ChartPackageManager(QObject *parent = nullptr);

    QVariantList visibleTiles() const;
    bool chartLoaded() const;
    QString activeChartId() const;
    QString activeChartTitle() const;
    QString activeChartSource() const;
    QString activeChartCycle() const;
    QString chartStatus() const;

    QString chartsDirectory() const;
    void setChartsDirectory(const QString &directory);

    QString preferredSourceKey() const;
    void setPreferredSourceKey(const QString &sourceKey);

    QStringList availablePackageIds() const;
    bool usingPlaceholder() const;

    Q_INVOKABLE void refreshPackages();
    Q_INVOKABLE void updateViewport(double viewportWidth,
                                    double viewportHeight,
                                    double centerLatitudeDeg,
                                    double centerLongitudeDeg,
                                    double zoomFactor = 1.0);
    Q_INVOKABLE QVariantMap projectCoordinateToViewport(double latitudeDeg, double longitudeDeg) const;
    Q_INVOKABLE QVariantMap viewportBounds() const;

signals:
    void visibleTilesChanged();
    void chartChanged();
    void chartsDirectoryChanged();
    void preferredSourceKeyChanged();
    void packagesChanged();

private:
    struct WorldFile {
        double a = 1.0;
        double d = 0.0;
        double b = 0.0;
        double e = -1.0;
        double c = 0.0;
        double f = 0.0;
        bool valid = false;
    };

    struct LambertConformalConic {
        double latitudeOfOriginDeg = 0.0;
        double centralMeridianDeg = 0.0;
        double standardParallel1Deg = 0.0;
        double standardParallel2Deg = 0.0;
        double falseEastingM = 0.0;
        double falseNorthingM = 0.0;
        bool valid = false;
    };

    struct Level {
        QString id;
        QString path;
        QString format = QStringLiteral("jpg");
        double scale = 1.0;
        int width = 0;
        int height = 0;
        int tileSize = 256;
        int rows = 0;
        int cols = 0;
        int minColumn = 0;
        int maxColumn = 0;
        int minRow = 0;
        int maxRow = 0;
        int zoom = 0;
    };

    struct ChartPackage {
        QString packageId;
        QString title;
        QString sourceKey;
        QString sourceName;
        QString packageType;
        QString cycle;
        QString rootPath;
        double westLon = 0.0;
        double eastLon = 0.0;
        double southLat = 0.0;
        double northLat = 0.0;
        int defaultZoom = 0;
        WorldFile worldFile;
        LambertConformalConic lcc;
        QVector<Level> levels;
        bool valid = false;
    };

    static QString defaultChartsDirectoryPath();
    static ChartPackage loadPackageFromManifest(const QString &manifestPath);
    static QPointF projectLambertConformalConic(double latitudeDeg,
                                                double longitudeDeg,
                                                const LambertConformalConic &projection);
    static QPointF inverseAffine(const QPointF &worldPoint, const WorldFile &worldFile);
    static QPointF mercatorPixel(double latitudeDeg, double longitudeDeg, int zoom, int tileSize);
    static bool contains(const ChartPackage &chartPackage, double latitudeDeg, double longitudeDeg);
    static QString tileUrlForLevel(const ChartPackage &chartPackage, const Level &level, int column, int row);
    static QString normalizedDirectoryPath(const QString &directory);
    QPointF sourcePixelForCoordinate(const ChartPackage &chartPackage,
                                     const Level &level,
                                     double latitudeDeg,
                                     double longitudeDeg) const;

    int selectActivePackageIndex(double latitudeDeg, double longitudeDeg) const;
    const Level *selectLevel(const ChartPackage &chartPackage) const;
    void rebuildVisibleTiles();
    void clearVisibleTiles();
    void updateChartStatus();

    QVector<ChartPackage> m_packages;
    QVariantList m_visibleTiles;
    QString m_chartsDirectory;
    QString m_preferredSourceKey = QStringLiteral("faa_vfr");
    QString m_chartStatus;
    int m_activePackageIndex = -1;
    double m_viewportWidth = 0.0;
    double m_viewportHeight = 0.0;
    double m_centerLatitudeDeg = 0.0;
    double m_centerLongitudeDeg = 0.0;
    double m_zoomFactor = 1.0;
};
