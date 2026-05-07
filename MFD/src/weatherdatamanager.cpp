#include "weatherdatamanager.h"

#include <QByteArray>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QUrlQuery>

#include <algorithm>
#include <cmath>

namespace {

constexpr int kRadarRefreshIntervalMs = 120000;
constexpr int kReconnectIntervalMs = 5000;
constexpr int kMaxMessages = 48;
constexpr int kMaxVisibleNexradBlocks = 2200;
constexpr qint64 kRegionalNexradTtlMs = 20LL * 60LL * 1000LL;
constexpr qint64 kConusNexradTtlMs = 35LL * 60LL * 1000LL;

constexpr double kBlockWidthDeg = 48.0 / 60.0;
constexpr double kWideBlockWidthDeg = 96.0 / 60.0;
constexpr double kBlockHeightDeg = 4.0 / 60.0;
constexpr int kBlockThreshold = 405000;
constexpr int kBlocksPerRing = 450;

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

bool isNexradProduct(const int productId)
{
    return productId >= 51 && productId <= 64;
}

qint64 nexradTtlMs(const int productId)
{
    return (productId == 64) ? kConusNexradTtlMs : kRegionalNexradTtlMs;
}

double normalizedLongitudeDeg(double longitudeDeg)
{
    while (longitudeDeg < -180.0) {
        longitudeDeg += 360.0;
    }
    while (longitudeDeg > 180.0) {
        longitudeDeg -= 360.0;
    }
    return longitudeDeg;
}

int positiveModulo(const int value, const int modulus)
{
    const int result = value % modulus;
    return result < 0 ? result + modulus : result;
}

double blockScaleMultiplier(const int scaleFactor)
{
    if (scaleFactor == 1) {
        return 5.0;
    }
    if (scaleFactor == 2) {
        return 9.0;
    }
    return 1.0;
}

struct BlockLocation {
    double latNorthDeg = 0.0;
    double lonWestDeg = 0.0;
    double latHeightDeg = 0.0;
    double lonWidthDeg = 0.0;
};

BlockLocation blockLocation(int blockNumber, const bool southernHemisphere, const int scaleFactor)
{
    const double realScale = blockScaleMultiplier(scaleFactor);

    if (blockNumber >= kBlockThreshold) {
        blockNumber &= ~1;
    }

    double rawLatDeg = kBlockHeightDeg * static_cast<double>(blockNumber / kBlocksPerRing);
    double rawLonDeg = static_cast<double>(blockNumber % kBlocksPerRing) * kBlockWidthDeg;

    const double lonSizeDeg = (blockNumber >= kBlockThreshold ? kWideBlockWidthDeg : kBlockWidthDeg) * realScale;
    const double latSizeDeg = kBlockHeightDeg * realScale;

    if (southernHemisphere) {
        rawLatDeg = -rawLatDeg;
    } else {
        rawLatDeg += kBlockHeightDeg;
    }

    if (rawLonDeg > 180.0) {
        rawLonDeg -= 360.0;
    }

    return {
        rawLatDeg,
        rawLonDeg,
        latSizeDeg,
        lonSizeDeg
    };
}

QVariantMap nexradBlockToVariant(const WeatherDataManager::NexradBlock &block)
{
    QVariantMap map;
    map.insert(QStringLiteral("key"), block.key);
    map.insert(QStringLiteral("productId"), block.productId);
    map.insert(QStringLiteral("scaleFactor"), block.scaleFactor);
    map.insert(QStringLiteral("blockNumber"), block.blockNumber);
    map.insert(QStringLiteral("latNorthDeg"), block.latNorthDeg);
    map.insert(QStringLiteral("latSouthDeg"), block.latNorthDeg - block.latHeightDeg);
    map.insert(QStringLiteral("lonWestDeg"), block.lonWestDeg);
    map.insert(QStringLiteral("lonEastDeg"), normalizedLongitudeDeg(block.lonWestDeg + block.lonWidthDeg));
    map.insert(QStringLiteral("latHeightDeg"), block.latHeightDeg);
    map.insert(QStringLiteral("lonWidthDeg"), block.lonWidthDeg);

    QVariantList intensities;
    intensities.reserve(block.intensity.size());
    for (const quint8 value : block.intensity) {
        intensities.push_back(value);
    }
    map.insert(QStringLiteral("intensity"), intensities);
    map.insert(QStringLiteral("receivedAtUtc"), block.receivedAtUtc.toString(Qt::ISODate));
    return map;
}

bool blockIntersectsViewport(const WeatherDataManager::NexradBlock &block,
                             const double southLat,
                             const double westLon,
                             const double northLat,
                             const double eastLon)
{
    const double blockSouth = block.latNorthDeg - block.latHeightDeg;
    const double blockNorth = block.latNorthDeg;
    const double blockWest = block.lonWestDeg;
    const double blockEast = block.lonWestDeg + block.lonWidthDeg;

    return blockNorth >= southLat
        && blockSouth <= northLat
        && blockEast >= westLon
        && blockWest <= eastLon;
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
        const bool changed = !m_weatherSocketConnected;
        m_weatherSocketConnected = true;
        if (changed) {
            emit stratuxStateChanged();
            rebuildOverlayUrl();
        }
    });

    connect(&m_weatherSocket, &QWebSocket::disconnected, this, [this]() {
        const bool changed = m_weatherSocketConnected;
        m_weatherSocketConnected = false;
        if (changed) {
            emit stratuxStateChanged();
            rebuildOverlayUrl();
        }
        scheduleReconnect();
    });

    connect(&m_weatherSocket, &QWebSocket::textMessageReceived, this, &WeatherDataManager::handleWeatherSocketMessage);

    connect(&m_weatherSocket, &QWebSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        const bool changed = m_weatherSocketConnected;
        m_weatherSocketConnected = false;
        if (changed) {
            emit stratuxStateChanged();
            rebuildOverlayUrl();
        }
        scheduleReconnect();
    });

    connect(&m_jsonSocket, &QWebSocket::connected, this, [this]() {
        const bool changed = !m_jsonSocketConnected;
        m_jsonSocketConnected = true;
        if (changed) {
            emit stratuxStateChanged();
            rebuildOverlayUrl();
        }
    });

    connect(&m_jsonSocket, &QWebSocket::disconnected, this, [this]() {
        const bool changed = m_jsonSocketConnected;
        m_jsonSocketConnected = false;
        if (changed) {
            emit stratuxStateChanged();
            rebuildOverlayUrl();
        }
        scheduleReconnect();
    });

    connect(&m_jsonSocket, &QWebSocket::textMessageReceived, this, &WeatherDataManager::handleJsonSocketMessage);

    connect(&m_jsonSocket, &QWebSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        const bool changed = m_jsonSocketConnected;
        m_jsonSocketConnected = false;
        if (changed) {
            emit stratuxStateChanged();
            rebuildOverlayUrl();
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

QVariantList WeatherDataManager::nexradBlocks() const
{
    return m_visibleNexradBlocks;
}

bool WeatherDataManager::fisbNexradAvailable() const
{
    return !m_visibleNexradBlocks.isEmpty();
}

bool WeatherDataManager::stratuxConnected() const
{
    return m_weatherSocketConnected || m_jsonSocketConnected;
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

    rebuildVisibleNexradBlocks();

    if (m_radarOverlayUrl.isEmpty() || significantMove || sizeChanged) {
        rebuildOverlayUrl();
    }
}

void WeatherDataManager::forceRefresh()
{
    rebuildOverlayUrl();
    reconnectStratux();
}

void WeatherDataManager::ingestExternalNexradBlock(const QVariantMap &blockMap)
{
    NexradBlock block;
    block.productId = blockMap.value(QStringLiteral("productId")).toInt();
    block.scaleFactor = blockMap.value(QStringLiteral("scaleFactor")).toInt();
    block.blockNumber = blockMap.value(QStringLiteral("blockNumber")).toInt();
    block.latNorthDeg = blockMap.value(QStringLiteral("latNorthDeg")).toDouble();
    block.lonWestDeg = blockMap.value(QStringLiteral("lonWestDeg")).toDouble();
    block.latHeightDeg = blockMap.value(QStringLiteral("latHeightDeg")).toDouble();
    block.lonWidthDeg = blockMap.value(QStringLiteral("lonWidthDeg")).toDouble();
    block.receivedAtUtc = QDateTime::currentDateTimeUtc();
    block.key = QStringLiteral("%1:%2:%3").arg(block.productId).arg(block.scaleFactor).arg(block.blockNumber);

    const QVariantList intensityValues = blockMap.value(QStringLiteral("intensity")).toList();
    block.intensity.reserve(intensityValues.size());
    for (const QVariant &value : intensityValues) {
        block.intensity.push_back(static_cast<quint8>(value.toInt() & 0xFF));
    }

    if (block.key.isEmpty() || block.intensity.isEmpty()) {
        return;
    }

    upsertNexradBlock(block);
    pruneExpiredNexradBlocks();
    rebuildVisibleNexradBlocks();
    rebuildOverlayUrl();
}

void WeatherDataManager::rebuildOverlayUrl()
{
    const bool rawActive = !m_visibleNexradBlocks.isEmpty();
    const QString fisbState = m_jsonSocketConnected
        ? QStringLiteral("raw")
        : (m_weatherSocketConnected ? QStringLiteral("text") : QStringLiteral("standby"));

    if (m_pixelWidth <= 0 || m_pixelHeight <= 0 || m_southLat == m_northLat || m_westLon == m_eastLon) {
        const QString idleStatus = rawActive
            ? QStringLiteral("FIS-B NEXRAD idle viewport  |  blocks %1").arg(m_visibleNexradBlocks.size())
            : QStringLiteral("NOAA radar idle");
        const bool changed = m_radarOverlayUrl != QString() || m_radarOverlayStatus != idleStatus;
        m_radarOverlayUrl.clear();
        m_radarOverlayStatus = idleStatus;
        if (changed) {
            emit overlayChanged();
        }
        return;
    }

    if (rawActive) {
        const QString nextStatus = QStringLiteral("FIS-B NEXRAD blocks %1 / cache %2  |  %3  |  %4")
            .arg(m_visibleNexradBlocks.size())
            .arg(m_nexradBlockCache.size())
            .arg(QDateTime::currentDateTimeUtc().toString(QStringLiteral("HH:mm:ss'Z'")))
            .arg(fisbState);
        const bool changed = !m_radarOverlayUrl.isEmpty() || m_radarOverlayStatus != nextStatus;
        m_radarOverlayUrl.clear();
        m_radarOverlayStatus = nextStatus;
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
        .arg(fisbState);

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

    const QUrl weatherUrl(websocketPath(m_stratuxEndpoint, QStringLiteral("/weather")));
    const QUrl jsonUrl(websocketPath(m_stratuxEndpoint, QStringLiteral("/jsonio")));

    if (m_weatherSocket.state() != QAbstractSocket::ConnectedState
        && m_weatherSocket.state() != QAbstractSocket::ConnectingState) {
        m_weatherSocket.open(weatherUrl);
    }

    if (m_jsonSocket.state() != QAbstractSocket::ConnectedState
        && m_jsonSocket.state() != QAbstractSocket::ConnectingState) {
        m_jsonSocket.open(jsonUrl);
    }
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

void WeatherDataManager::rebuildVisibleNexradBlocks()
{
    pruneExpiredNexradBlocks();

    QVariantList nextVisibleBlocks;
    if (m_southLat == m_northLat || m_westLon == m_eastLon) {
        if (m_visibleNexradBlocks != nextVisibleBlocks) {
            m_visibleNexradBlocks = nextVisibleBlocks;
            emit nexradBlocksChanged();
            rebuildOverlayUrl();
        }
        return;
    }

    QVector<NexradBlock> visibleBlocks;
    visibleBlocks.reserve(m_nexradBlockCache.size());
    for (auto it = m_nexradBlockCache.cbegin(); it != m_nexradBlockCache.cend(); ++it) {
        if (blockIntersectsViewport(it.value(), m_southLat, m_westLon, m_northLat, m_eastLon)) {
            visibleBlocks.push_back(it.value());
        }
    }

    std::sort(visibleBlocks.begin(), visibleBlocks.end(), [](const NexradBlock &left, const NexradBlock &right) {
        const double leftArea = left.latHeightDeg * left.lonWidthDeg;
        const double rightArea = right.latHeightDeg * right.lonWidthDeg;
        if (!qFuzzyCompare(leftArea + 1.0, rightArea + 1.0)) {
            return leftArea > rightArea;
        }
        if (left.productId != right.productId) {
            return left.productId > right.productId;
        }
        return left.receivedAtUtc < right.receivedAtUtc;
    });

    if (visibleBlocks.size() > kMaxVisibleNexradBlocks) {
        visibleBlocks.resize(kMaxVisibleNexradBlocks);
    }

    nextVisibleBlocks.reserve(visibleBlocks.size());
    for (const NexradBlock &block : visibleBlocks) {
        nextVisibleBlocks.push_back(nexradBlockToVariant(block));
    }

    if (m_visibleNexradBlocks == nextVisibleBlocks) {
        return;
    }

    m_visibleNexradBlocks = nextVisibleBlocks;
    emit nexradBlocksChanged();
    rebuildOverlayUrl();
}

void WeatherDataManager::handleWeatherSocketMessage(const QString &payload)
{
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
}

void WeatherDataManager::handleJsonSocketMessage(const QString &payload)
{
    const QJsonDocument document = QJsonDocument::fromJson(payload.toUtf8());
    if (!document.isObject()) {
        return;
    }

    const QJsonObject object = document.object();
    if (!object.contains(QStringLiteral("Product_id"))) {
        return;
    }

    const int productId = object.value(QStringLiteral("Product_id")).toInt(-1);
    if (!isNexradProduct(productId)) {
        return;
    }

    const QString fisbDataBase64 = object.value(QStringLiteral("FISB_data")).toString();
    if (fisbDataBase64.isEmpty()) {
        return;
    }

    const QByteArray fisbData = QByteArray::fromBase64(fisbDataBase64.toUtf8());
    if (fisbData.isEmpty()) {
        return;
    }

    decodeNexradFrame(productId, fisbData, QDateTime::currentDateTimeUtc());
}

void WeatherDataManager::decodeNexradFrame(const int productId,
                                           const QByteArray &fisbData,
                                           const QDateTime &receivedAtUtc)
{
    if (fisbData.size() < 4) {
        return;
    }

    const auto byte0 = static_cast<quint8>(fisbData.at(0));
    const bool runLengthEncoded = (byte0 & 0x80U) != 0U;
    const bool southernHemisphere = (byte0 & 0x40U) != 0U;
    const int blockNumber = ((static_cast<int>(byte0) & 0x0f) << 16)
        | (static_cast<int>(static_cast<quint8>(fisbData.at(1))) << 8)
        | static_cast<int>(static_cast<quint8>(fisbData.at(2)));
    const int scaleFactor = (static_cast<int>(byte0) & 0x30) >> 4;

    if (runLengthEncoded) {
        const BlockLocation location = blockLocation(blockNumber, southernHemisphere, scaleFactor);

        NexradBlock block;
        block.key = QStringLiteral("%1:%2:%3:%4")
                        .arg(productId)
                        .arg(southernHemisphere ? 1 : 0)
                        .arg(scaleFactor)
                        .arg(blockNumber);
        block.productId = productId;
        block.scaleFactor = scaleFactor;
        block.blockNumber = blockNumber;
        block.latNorthDeg = location.latNorthDeg;
        block.lonWestDeg = location.lonWestDeg;
        block.latHeightDeg = location.latHeightDeg;
        block.lonWidthDeg = location.lonWidthDeg;
        block.receivedAtUtc = receivedAtUtc;

        for (int index = 3; index < fisbData.size(); ++index) {
            const quint8 value = static_cast<quint8>(fisbData.at(index));
            const quint8 intensity = value & 0x07U;
            int runLength = (value >> 3) + 1;
            while (runLength-- > 0) {
                block.intensity.push_back(intensity);
            }
        }

        if (!block.intensity.isEmpty()) {
            upsertNexradBlock(block);
            rebuildVisibleNexradBlocks();
        }
        return;
    }

    int rowStart = 0;
    int rowSize = 0;
    if (blockNumber >= kBlockThreshold) {
        rowStart = blockNumber - ((blockNumber - kBlockThreshold) % 225);
        rowSize = 225;
    } else {
        rowStart = blockNumber - (blockNumber % 450);
        rowSize = 450;
    }

    const int rowOffset = blockNumber - rowStart;
    const int blockMaskBytes = static_cast<int>(static_cast<quint8>(fisbData.at(3)) & 0x0fU);
    if (fisbData.size() < blockMaskBytes + 3) {
        return;
    }

    bool updatedAny = false;
    for (int i = 0; i < blockMaskBytes; ++i) {
        int maskByte = 0;
        if (i == 0) {
            maskByte = (static_cast<int>(static_cast<quint8>(fisbData.at(3))) & 0xf0) | 0x08;
        } else {
            maskByte = static_cast<int>(static_cast<quint8>(fisbData.at(i + 3)));
        }

        for (int bit = 0; bit < 8; ++bit) {
            if ((maskByte & (1 << bit)) == 0) {
                continue;
            }

            const int rowX = positiveModulo(rowOffset + 8 * i + bit - 3, rowSize);
            const int derivedBlockNumber = rowStart + rowX;
            const BlockLocation location = blockLocation(derivedBlockNumber, southernHemisphere, scaleFactor);

            NexradBlock block;
            block.key = QStringLiteral("%1:%2:%3:%4")
                            .arg(productId)
                            .arg(southernHemisphere ? 1 : 0)
                            .arg(scaleFactor)
                            .arg(derivedBlockNumber);
            block.productId = productId;
            block.scaleFactor = scaleFactor;
            block.blockNumber = derivedBlockNumber;
            block.latNorthDeg = location.latNorthDeg;
            block.lonWestDeg = location.lonWestDeg;
            block.latHeightDeg = location.latHeightDeg;
            block.lonWidthDeg = location.lonWidthDeg;
            block.receivedAtUtc = receivedAtUtc;
            block.intensity.fill(productId == 64 ? 1U : 0U, 128);
            upsertNexradBlock(block);
            updatedAny = true;
        }
    }

    if (updatedAny) {
        rebuildVisibleNexradBlocks();
    }
}

void WeatherDataManager::upsertNexradBlock(const NexradBlock &block)
{
    m_nexradBlockCache.insert(block.key, block);
}

void WeatherDataManager::pruneExpiredNexradBlocks()
{
    const QDateTime nowUtc = QDateTime::currentDateTimeUtc();
    bool removedAny = false;

    for (auto it = m_nexradBlockCache.begin(); it != m_nexradBlockCache.end();) {
        const qint64 ageMs = it.value().receivedAtUtc.msecsTo(nowUtc);
        if (ageMs > nexradTtlMs(it.value().productId)) {
            it = m_nexradBlockCache.erase(it);
            removedAny = true;
        } else {
            ++it;
        }
    }

    if (removedAny && m_visibleNexradBlocks.isEmpty()) {
        rebuildOverlayUrl();
    }
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

QString WeatherDataManager::websocketPath(const QString &endpoint, const QString &pathSuffix)
{
    QUrl url(endpoint);
    QString path = url.path();
    if (path.endsWith(QLatin1Char('/'))) {
        path.chop(1);
    }
    if (path.endsWith(QStringLiteral("/weather"))) {
        path.chop(QStringLiteral("/weather").size());
    } else if (path.endsWith(QStringLiteral("/jsonio"))) {
        path.chop(QStringLiteral("/jsonio").size());
    }
    if (!path.endsWith(pathSuffix)) {
        path += pathSuffix;
    }
    url.setPath(path);
    return url.toString();
}
