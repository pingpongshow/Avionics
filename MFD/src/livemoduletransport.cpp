#include "livemoduletransport.h"

#include "moduleprotocol.h"

#include <QDateTime>
#include <QSocketNotifier>

#ifdef Q_OS_LINUX
#include <cerrno>
#include <cstring>

#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace {

QString defaultCanInterface()
{
    const QString value = qEnvironmentVariable("PFD_CAN_IF");
    return value.trimmed().isEmpty() ? QStringLiteral("can0") : value.trimmed();
}

} // namespace

LiveModuleTransport::LiveModuleTransport(QObject *parent)
    : QObject(parent)
    , m_interfaceName(defaultCanInterface())
{
    m_reconnectTimer.setSingleShot(true);
    m_reconnectTimer.setInterval(3000);
    connect(&m_reconnectTimer, &QTimer::timeout, this, &LiveModuleTransport::connectSocket);
    connectSocket();
}

LiveModuleTransport::~LiveModuleTransport()
{
    closeSocket();
}

bool LiveModuleTransport::transportAvailable() const
{
#ifdef Q_OS_LINUX
    return true;
#else
    return false;
#endif
}

bool LiveModuleTransport::connected() const
{
    return m_connected;
}

QString LiveModuleTransport::interfaceName() const
{
    return m_interfaceName;
}

void LiveModuleTransport::setInterfaceName(const QString &interfaceName)
{
    const QString normalized = interfaceName.trimmed();
    if (normalized.isEmpty() || normalized == m_interfaceName) {
        return;
    }

    m_interfaceName = normalized;
    emit stateChanged();
    reconnect();
}

QString LiveModuleTransport::statusSummary() const
{
    return m_statusSummary;
}

void LiveModuleTransport::reconnect()
{
    closeSocket();
    connectSocket();
}

void LiveModuleTransport::connectSocket()
{
#ifdef Q_OS_LINUX
    if (m_socketFd >= 0) {
        return;
    }

    m_socketFd = ::socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (m_socketFd < 0) {
        setConnectionState(false, QStringLiteral("CAN open failed: %1").arg(QString::fromLocal8Bit(std::strerror(errno))));
        scheduleReconnect();
        return;
    }

    const int enableCanFd = 1;
    if (::setsockopt(m_socketFd, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &enableCanFd, sizeof(enableCanFd)) < 0) {
        setConnectionState(false, QStringLiteral("CAN FD not available on %1").arg(m_interfaceName));
        closeSocket();
        scheduleReconnect();
        return;
    }

    struct ifreq ifr {};
    const QByteArray interfaceBytes = m_interfaceName.toLocal8Bit();
    std::strncpy(ifr.ifr_name, interfaceBytes.constData(), IFNAMSIZ - 1);
    if (::ioctl(m_socketFd, SIOCGIFINDEX, &ifr) < 0) {
        setConnectionState(false, QStringLiteral("CAN interface %1 not found").arg(m_interfaceName));
        closeSocket();
        scheduleReconnect();
        return;
    }

    struct sockaddr_can address {};
    address.can_family = AF_CAN;
    address.can_ifindex = ifr.ifr_ifindex;
    if (::bind(m_socketFd, reinterpret_cast<struct sockaddr *>(&address), sizeof(address)) < 0) {
        setConnectionState(false, QStringLiteral("CAN bind failed on %1").arg(m_interfaceName));
        closeSocket();
        scheduleReconnect();
        return;
    }

    m_socketNotifier = new QSocketNotifier(m_socketFd, QSocketNotifier::Read, this);
    connect(m_socketNotifier, &QSocketNotifier::activated, this, [this]() {
        processSocketReadable();
    });

    setConnectionState(true, QStringLiteral("Listening on %1").arg(m_interfaceName));
#else
    setConnectionState(false, QStringLiteral("SocketCAN live input is only available on Linux / Yocto targets"));
#endif
}

void LiveModuleTransport::closeSocket()
{
#ifdef Q_OS_LINUX
    if (m_socketNotifier != nullptr) {
        m_socketNotifier->deleteLater();
        m_socketNotifier = nullptr;
    }

    if (m_socketFd >= 0) {
        ::close(m_socketFd);
        m_socketFd = -1;
    }
#endif

    if (m_connected) {
        setConnectionState(false, QStringLiteral("CAN disconnected"));
    }
}

void LiveModuleTransport::scheduleReconnect()
{
    if (!transportAvailable()) {
        return;
    }
    if (!m_reconnectTimer.isActive()) {
        m_reconnectTimer.start();
    }
}

void LiveModuleTransport::setConnectionState(const bool connectedValue, const QString &statusSummaryValue)
{
    const bool changed = (m_connected != connectedValue) || (m_statusSummary != statusSummaryValue);
    m_connected = connectedValue;
    m_statusSummary = statusSummaryValue;
    if (changed) {
        emit stateChanged();
    }
}

void LiveModuleTransport::handleDecodedFrame(const quint32 frameId, const QVariantMap &fields)
{
    switch (frameId) {
    case 0x110:
        emit adcFlightReceived(fields);
        break;
    case 0x150:
        emit gnssNavReceived(fields);
        break;
    case 0x210:
        emit eisSummaryReceived(fields);
        break;
    case 0x211:
        emit eisCylindersReceived(fields);
        break;
    case 0x212:
        emit eisFuelTankReceived(fields);
        break;
    case 0x310:
        emit magHeadingReceived(fields);
        break;
    case 0x510:
        emit adsbTrafficReceived(fields);
        break;
    case 0x520:
        handleWeatherHeader(fields);
        break;
    case 0x521:
        handleWeatherPayload(fields);
        break;
    default:
        if (frameId >= 0x100 && frameId < 0x110) {
            emit heartbeatReceived(fields);
        }
        break;
    }
}

void LiveModuleTransport::handleWeatherHeader(const QVariantMap &fields)
{
    const quint32 blockNumber = fields.value(QStringLiteral("blockNumber")).toUInt();
    if (blockNumber == 0 && !fields.contains(QStringLiteral("blockNumber"))) {
        return;
    }

    PendingWeatherBlock pending;
    pending.header = fields;
    pending.expectedLength = std::max(0, fields.value(QStringLiteral("intensityCount")).toInt());
    pending.intensity.resize(pending.expectedLength);
    pending.intensity.fill('\0');
    m_pendingWeatherBlocks.insert(blockNumber, pending);
}

void LiveModuleTransport::handleWeatherPayload(const QVariantMap &fields)
{
    const quint32 blockNumber = fields.value(QStringLiteral("blockNumber")).toUInt();
    const int offset = std::max(0, fields.value(QStringLiteral("offset")).toInt());
    auto it = m_pendingWeatherBlocks.find(blockNumber);
    if (it == m_pendingWeatherBlocks.end()) {
        return;
    }

    PendingWeatherBlock &pending = it.value();
    QVariantList chunkValues = fields.value(QStringLiteral("intensity")).toList();
    if (offset >= pending.intensity.size()) {
        return;
    }

    const int writableCount = std::min(static_cast<int>(chunkValues.size()), pending.intensity.size() - offset);
    for (int index = 0; index < writableCount; ++index) {
        pending.intensity[offset + index] = static_cast<char>(chunkValues.at(index).toInt() & 0xFF);
    }

    bool complete = true;
    QVariantList intensity;
    intensity.reserve(pending.intensity.size());
    for (const char value : std::as_const(pending.intensity)) {
        intensity.push_back(static_cast<quint8>(value));
    }

    for (int index = 0; index < pending.expectedLength; ++index) {
        if (offset <= index && index < (offset + writableCount)) {
            continue;
        }
        if (pending.intensity.at(index) == '\0' && intensity.at(index).toInt() == 0) {
            complete = false;
        }
    }

    if (!complete) {
        return;
    }

    QVariantMap completed = pending.header;
    completed.insert(QStringLiteral("intensity"), intensity);
    completed.insert(QStringLiteral("receivedAtUtc"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    emit adsbWeatherBlockReceived(completed);
    m_pendingWeatherBlocks.erase(it);
}

#ifdef Q_OS_LINUX
void LiveModuleTransport::processSocketReadable()
{
    if (m_socketFd < 0) {
        return;
    }

    struct canfd_frame frame {};
    const ssize_t bytesRead = ::read(m_socketFd, &frame, sizeof(frame));
    if (bytesRead < 0) {
        setConnectionState(false, QStringLiteral("CAN read failed on %1").arg(m_interfaceName));
        closeSocket();
        scheduleReconnect();
        return;
    }

    const quint32 frameId = frame.can_id & CAN_SFF_MASK;
    const QByteArray payload(reinterpret_cast<const char *>(frame.data), std::min<int>(frame.len, 64));

    ModuleProtocol::DecodedFrame decodedFrame;
    if (!ModuleProtocol::decodeFrame(frameId, payload, &decodedFrame)) {
        return;
    }

    handleDecodedFrame(frameId, decodedFrame.fields);
}
#endif
