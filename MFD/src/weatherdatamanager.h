#pragma once

#include <QObject>
#include <QDateTime>
#include <QHash>
#include <QTimer>
#include <QVariantList>
#include <QVector>
#include <QWebSocket>

class WeatherDataManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString radarOverlayUrl READ radarOverlayUrl NOTIFY overlayChanged)
    Q_PROPERTY(QString radarOverlayStatus READ radarOverlayStatus NOTIFY overlayChanged)
    Q_PROPERTY(QVariantList fisbMessages READ fisbMessages NOTIFY fisbMessagesChanged)
    Q_PROPERTY(QVariantList nexradBlocks READ nexradBlocks NOTIFY nexradBlocksChanged)
    Q_PROPERTY(bool fisbNexradAvailable READ fisbNexradAvailable NOTIFY nexradBlocksChanged)
    Q_PROPERTY(bool stratuxConnected READ stratuxConnected NOTIFY stratuxStateChanged)
    Q_PROPERTY(QString stratuxEndpoint READ stratuxEndpoint WRITE setStratuxEndpoint NOTIFY stratuxStateChanged)

public:
    struct NexradBlock {
        QString key;
        int productId = 0;
        int scaleFactor = 0;
        int blockNumber = 0;
        double latNorthDeg = 0.0;
        double lonWestDeg = 0.0;
        double latHeightDeg = 0.0;
        double lonWidthDeg = 0.0;
        QVector<quint8> intensity;
        QDateTime receivedAtUtc;
    };

    explicit WeatherDataManager(QObject *parent = nullptr);

    QString radarOverlayUrl() const;
    QString radarOverlayStatus() const;
    QVariantList fisbMessages() const;
    QVariantList nexradBlocks() const;
    bool fisbNexradAvailable() const;
    bool stratuxConnected() const;
    QString stratuxEndpoint() const;
    void setStratuxEndpoint(const QString &endpoint);

    Q_INVOKABLE void updateViewport(double southLat,
                                    double westLon,
                                    double northLat,
                                    double eastLon,
                                    int pixelWidth,
                                    int pixelHeight);
    Q_INVOKABLE void forceRefresh();
    Q_INVOKABLE void ingestExternalNexradBlock(const QVariantMap &block);

signals:
    void overlayChanged();
    void fisbMessagesChanged();
    void nexradBlocksChanged();
    void stratuxStateChanged();

private:
    void rebuildOverlayUrl();
    void reconnectStratux();
    void scheduleReconnect();
    void appendWeatherMessage(const QVariantMap &message);
    void rebuildVisibleNexradBlocks();
    void handleWeatherSocketMessage(const QString &payload);
    void handleJsonSocketMessage(const QString &payload);
    void decodeNexradFrame(int productId, const QByteArray &fisbData, const QDateTime &receivedAtUtc);
    void upsertNexradBlock(const NexradBlock &block);
    void pruneExpiredNexradBlocks();
    static QString normalizedEndpoint(const QString &endpoint);
    static QString websocketPath(const QString &endpoint, const QString &pathSuffix);

    QString m_radarOverlayUrl;
    QString m_radarOverlayStatus = QStringLiteral("NOAA radar idle");
    QVariantList m_fisbMessages;
    QVariantList m_visibleNexradBlocks;
    QString m_stratuxEndpoint;
    bool m_weatherSocketConnected = false;
    bool m_jsonSocketConnected = false;
    double m_southLat = 0.0;
    double m_westLon = 0.0;
    double m_northLat = 0.0;
    double m_eastLon = 0.0;
    int m_pixelWidth = 0;
    int m_pixelHeight = 0;
    qint64 m_overlayNonce = 0;
    QTimer m_radarRefreshTimer;
    QTimer m_reconnectTimer;
    QWebSocket m_weatherSocket;
    QWebSocket m_jsonSocket;
    QHash<QString, NexradBlock> m_nexradBlockCache;
};
