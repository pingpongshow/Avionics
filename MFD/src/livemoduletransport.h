#pragma once

#include <QObject>
#include <QByteArray>
#include <QHash>
#include <QTimer>
#include <QVariantMap>

#ifdef Q_OS_LINUX
class QSocketNotifier;
#endif

class LiveModuleTransport : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool transportAvailable READ transportAvailable CONSTANT)
    Q_PROPERTY(bool connected READ connected NOTIFY stateChanged)
    Q_PROPERTY(QString interfaceName READ interfaceName WRITE setInterfaceName NOTIFY stateChanged)
    Q_PROPERTY(QString statusSummary READ statusSummary NOTIFY stateChanged)

public:
    explicit LiveModuleTransport(QObject *parent = nullptr);
    ~LiveModuleTransport() override;

    bool transportAvailable() const;
    bool connected() const;
    QString interfaceName() const;
    void setInterfaceName(const QString &interfaceName);
    QString statusSummary() const;

    Q_INVOKABLE void reconnect();

signals:
    void stateChanged();
    void heartbeatReceived(const QVariantMap &message);
    void adcFlightReceived(const QVariantMap &message);
    void gnssNavReceived(const QVariantMap &message);
    void eisSummaryReceived(const QVariantMap &message);
    void eisCylindersReceived(const QVariantMap &message);
    void eisFuelTankReceived(const QVariantMap &message);
    void magHeadingReceived(const QVariantMap &message);
    void adsbTrafficReceived(const QVariantMap &message);
    void adsbWeatherBlockReceived(const QVariantMap &message);

private:
    struct PendingWeatherBlock {
        QVariantMap header;
        QByteArray intensity;
        int expectedLength = 0;
    };

    void connectSocket();
    void closeSocket();
    void scheduleReconnect();
    void setConnectionState(bool connected, const QString &statusSummary);
    void handleDecodedFrame(quint32 frameId, const QVariantMap &fields);
    void handleWeatherHeader(const QVariantMap &fields);
    void handleWeatherPayload(const QVariantMap &fields);

#ifdef Q_OS_LINUX
    void processSocketReadable();

    int m_socketFd = -1;
    QSocketNotifier *m_socketNotifier = nullptr;
#endif

    bool m_connected = false;
    QString m_interfaceName;
    QString m_statusSummary;
    QTimer m_reconnectTimer;
    QHash<quint32, PendingWeatherBlock> m_pendingWeatherBlocks;
};
