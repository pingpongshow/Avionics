#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QVector>

class ModuleListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    struct ModuleEntry {
        QString key;
        QString name;
        bool present = false;
        bool healthy = false;
        QString mode;
        QString details;
    };

    enum Role {
        KeyRole = Qt::UserRole + 1,
        NameRole,
        PresentRole,
        HealthyRole,
        ModeRole,
        DetailsRole
    };

    explicit ModuleListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setModules(const QVector<ModuleEntry> &modules);
    bool setModuleState(const QString &key,
                        bool present,
                        bool healthy,
                        const QString &mode,
                        const QString &details);

    bool isPresent(const QString &key) const;
    bool isHealthy(const QString &key) const;
    QString details(const QString &key) const;

private:
    int indexOf(const QString &key) const;

    QVector<ModuleEntry> m_modules;
};
