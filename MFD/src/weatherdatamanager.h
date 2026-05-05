#pragma once

#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <QWebSocket>

class WeatherDataManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString radarOverlayUrl READ radarOverlayUrl NOTIFY overlayChanged)
    Q_PROPERTY(QString radarOverlayStatus READ radarOverlayStatus NOTIFY overlayChanged)
    Q_PROPERTY(QVariantList fisbMessages READ fisbMessages NOTIFY fisbMessagesChanged)
    Q_PROPERTY(bool stratuxConnected READ stratuxConnected NOTIFY stratuxStateChanged)
    Q_PROPERTY(QString stratuxEndpoint READ stratuxEndpoint WRITE setStratuxEndpoint NOTIFY stratuxStateChanged)

public:
    explicit WeatherDataManager(QObject *parent = nullptr);

    QString radarOverlayUrl() const;
    QString radarOverlayStatus() const;
    QVariantList fisbMessages() const;
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

signals:
    void overlayChanged();
    void fisbMessagesChanged();
    void stratuxStateChanged();

private:
    void rebuildOverlayUrl();
    void reconnectStratux();
    void scheduleReconnect();
    void appendWeatherMessage(const QVariantMap &message);
    static QString normalizedEndpoint(const QString &endpoint);

    QString m_radarOverlayUrl;
    QString m_radarOverlayStatus = QStringLiteral("NOAA radar idle");
    QVariantList m_fisbMessages;
    QString m_stratuxEndpoint;
    bool m_stratuxConnected = false;
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
};
