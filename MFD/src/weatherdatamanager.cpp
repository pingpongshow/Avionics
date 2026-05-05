#include "weatherdatamanager.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QUrlQuery>

#include <algorithm>

namespace {

constexpr int kRadarRefreshIntervalMs = 120000;
constexpr int kReconnectIntervalMs = 5000;
constexpr int kMaxMessages = 48;

QString defaultStratuxEndpoint()
{
    const QString envEndpoint = qEnvironmentVariable("PFD_STRATUX_ENDPOINT");
    if (!envEndpoint.trimmed().isEmpty()) {
        return envEndpoint.trimmed();
    }
    return QStringLiteral("ws://192.168.10.1");
}

QString weatherTypeLabel(const QVariantMap &message)
{
    const QString type = message.value(QStringLiteral("type")).toString().trimmed();
    return type.isEmpty() ? QStringLiteral("FIS-B") : type.toUpper();
}

} // namespace

WeatherDataManager::WeatherDataManager(QObject *parent)
    : QObject(parent)
    , m_stratuxEndpoint(normalizedEndpoint(defaultStratuxEndpoint()))
{
    m_radarRefreshTimer.setInterval(kRadarRefreshIntervalMs);
    m_radarRefreshTimer.setSingleShot(false);
    connect(&m_radarRefreshTimer, &QTimer::timeout, this, &WeatherDataManager::rebuildOverlayUrl);
    m_radarRefreshTimer.start();

    m_reconnectTimer.setInterval(kReconnectIntervalMs);
    m_reconnectTimer.setSingleShot(true);
    connect(&m_reconnectTimer, &QTimer::timeout, this, &WeatherDataManager::reconnectStratux);

    connect(&m_weatherSocket, &QWebSocket::connected, this, [this]() {
        if (!m_stratuxConnected) {
            m_stratuxConnected = true;
            emit stratuxStateChanged();
        }
    });

    connect(&m_weatherSocket, &QWebSocket::disconnected, this, [this]() {
        if (m_stratuxConnected) {
            m_stratuxConnected = false;
            emit stratuxStateChanged();
        }
        scheduleReconnect();
    });

    connect(&m_weatherSocket, &QWebSocket::textMessageReceived, this, [this](const QString &payload) {
        const QJsonDocument document = QJsonDocument::fromJson(payload.toUtf8());
        if (!document.isObject()) {
            return;
        }

        const QJsonObject object = document.object();
        QVariantMap message;
        message.insert(QStringLiteral("type"), object.value(QStringLiteral("Type")).toString());
        message.insert(QStringLiteral("location"), object.value(QStringLiteral("Location")).toString());
        message.insert(QStringLiteral("time"), object.value(QStringLiteral("Time")).toString());
        message.insert(QStringLiteral("data"), object.value(QStringLiteral("Data")).toString());
        message.insert(QStringLiteral("receivedAt"), object.value(QStringLiteral("LocaltimeReceived")).toString());
        appendWeatherMessage(message);
    });

    connect(&m_weatherSocket, &QWebSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        if (m_stratuxConnected) {
            m_stratuxConnected = false;
            emit stratuxStateChanged();
        }
        scheduleReconnect();
    });

    reconnectStratux();
}

QString WeatherDataManager::radarOverlayUrl() const
{
    return m_radarOverlayUrl;
}

QString WeatherDataManager::radarOverlayStatus() const
{
    return m_radarOverlayStatus;
}

QVariantList WeatherDataManager::fisbMessages() const
{
    return m_fisbMessages;
}

bool WeatherDataManager::stratuxConnected() const
{
    return m_stratuxConnected;
}

QString WeatherDataManager::stratuxEndpoint() const
{
    return m_stratuxEndpoint;
}

void WeatherDataManager::setStratuxEndpoint(const QString &endpoint)
{
    const QString normalized = normalizedEndpoint(endpoint);
    if (normalized.isEmpty() || normalized == m_stratuxEndpoint) {
        return;
    }

    m_stratuxEndpoint = normalized;
    emit stratuxStateChanged();
    reconnectStratux();
}

void WeatherDataManager::updateViewport(double southLat,
                                        double westLon,
                                        double northLat,
                                        double eastLon,
                                        int pixelWidth,
                                        int pixelHeight)
{
    const double nextSouthLat = std::min(southLat, northLat);
    const double nextNorthLat = std::max(southLat, northLat);
    const double nextWestLon = std::min(westLon, eastLon);
    const double nextEastLon = std::max(westLon, eastLon);
    const int nextPixelWidth = std::max(256, pixelWidth);
    const int nextPixelHeight = std::max(256, pixelHeight);

    const double currentLatSpan = std::max(0.001, std::abs(m_northLat - m_southLat));
    const double currentLonSpan = std::max(0.001, std::abs(m_eastLon - m_westLon));
    const double currentCenterLat = (m_southLat + m_northLat) / 2.0;
    const double currentCenterLon = (m_westLon + m_eastLon) / 2.0;
    const double nextCenterLat = (nextSouthLat + nextNorthLat) / 2.0;
    const double nextCenterLon = (nextWestLon + nextEastLon) / 2.0;

    const bool significantMove = std::abs(nextCenterLat - currentCenterLat) > (currentLatSpan * 0.25)
        || std::abs(nextCenterLon - currentCenterLon) > (currentLonSpan * 0.25);
    const bool sizeChanged = std::abs(nextPixelWidth - m_pixelWidth) > (m_pixelWidth * 0.15)
        || std::abs(nextPixelHeight - m_pixelHeight) > (m_pixelHeight * 0.15);

    m_southLat = nextSouthLat;
    m_northLat = nextNorthLat;
    m_westLon = nextWestLon;
    m_eastLon = nextEastLon;
    m_pixelWidth = nextPixelWidth;
    m_pixelHeight = nextPixelHeight;

    if (m_radarOverlayUrl.isEmpty() || significantMove || sizeChanged) {
        rebuildOverlayUrl();
    }
}

void WeatherDataManager::forceRefresh()
{
    rebuildOverlayUrl();
    reconnectStratux();
}

void WeatherDataManager::rebuildOverlayUrl()
{
    if (m_pixelWidth <= 0 || m_pixelHeight <= 0 || m_southLat == m_northLat || m_westLon == m_eastLon) {
        const QString idleStatus = QStringLiteral("NOAA radar idle");
        const bool changed = m_radarOverlayUrl != QString() || m_radarOverlayStatus != idleStatus;
        m_radarOverlayUrl.clear();
        m_radarOverlayStatus = idleStatus;
        if (changed) {
            emit overlayChanged();
        }
        return;
    }

    QUrl url(QStringLiteral("https://opengeo.ncep.noaa.gov/geoserver/conus/conus_bref_qcd/ows"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("service"), QStringLiteral("WMS"));
    query.addQueryItem(QStringLiteral("version"), QStringLiteral("1.3.0"));
    query.addQueryItem(QStringLiteral("request"), QStringLiteral("GetMap"));
    query.addQueryItem(QStringLiteral("layers"), QStringLiteral("conus_bref_qcd"));
    query.addQueryItem(QStringLiteral("styles"), QString());
    query.addQueryItem(QStringLiteral("crs"), QStringLiteral("EPSG:4326"));
    query.addQueryItem(
        QStringLiteral("bbox"),
        QStringLiteral("%1,%2,%3,%4")
            .arg(m_southLat, 0, 'f', 4)
            .arg(m_westLon, 0, 'f', 4)
            .arg(m_northLat, 0, 'f', 4)
            .arg(m_eastLon, 0, 'f', 4));
    query.addQueryItem(QStringLiteral("width"), QString::number(m_pixelWidth));
    query.addQueryItem(QStringLiteral("height"), QString::number(m_pixelHeight));
    query.addQueryItem(QStringLiteral("format"), QStringLiteral("image/png"));
    query.addQueryItem(QStringLiteral("transparent"), QStringLiteral("true"));
    query.addQueryItem(QStringLiteral("_ts"), QString::number(++m_overlayNonce));
    url.setQuery(query);

    const QString nextUrl = url.toString(QUrl::FullyEncoded);
    const QString nextStatus = QStringLiteral("NOAA live radar  |  %1  |  FIS-B %2")
        .arg(QDateTime::currentDateTimeUtc().toString(QStringLiteral("HH:mm:ss'Z'")))
        .arg(m_stratuxConnected ? QStringLiteral("connected") : QStringLiteral("standby"));

    if (m_radarOverlayUrl == nextUrl && m_radarOverlayStatus == nextStatus) {
        return;
    }

    m_radarOverlayUrl = nextUrl;
    m_radarOverlayStatus = nextStatus;
    emit overlayChanged();
}

void WeatherDataManager::reconnectStratux()
{
    if (m_stratuxEndpoint.isEmpty()) {
        return;
    }

    const QUrl baseUrl(m_stratuxEndpoint);
    if (!baseUrl.isValid()) {
        return;
    }

    QUrl weatherUrl(baseUrl);
    QString path = weatherUrl.path();
    if (path.endsWith(QLatin1Char('/'))) {
        path.chop(1);
    }
    if (!path.endsWith(QStringLiteral("/weather"))) {
        path += QStringLiteral("/weather");
    }
    weatherUrl.setPath(path);

    if (m_weatherSocket.state() == QAbstractSocket::ConnectedState
        || m_weatherSocket.state() == QAbstractSocket::ConnectingState) {
        return;
    }

    m_weatherSocket.open(weatherUrl);
}

void WeatherDataManager::scheduleReconnect()
{
    if (!m_reconnectTimer.isActive()) {
        m_reconnectTimer.start();
    }
}

void WeatherDataManager::appendWeatherMessage(const QVariantMap &message)
{
    QVariantMap normalized = message;
    const QString label = weatherTypeLabel(normalized);
    normalized.insert(QStringLiteral("type"), label);
    if (normalized.value(QStringLiteral("receivedAt")).toString().isEmpty()) {
        normalized.insert(QStringLiteral("receivedAt"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    }

    m_fisbMessages.push_front(normalized);
    while (m_fisbMessages.size() > kMaxMessages) {
        m_fisbMessages.removeLast();
    }
    emit fisbMessagesChanged();
}

QString WeatherDataManager::normalizedEndpoint(const QString &endpoint)
{
    QString value = endpoint.trimmed();
    if (value.isEmpty()) {
        return {};
    }
    if (!value.startsWith(QStringLiteral("ws://")) && !value.startsWith(QStringLiteral("wss://"))) {
        value.prepend(QStringLiteral("ws://"));
    }
    return value;
}
