#include "modulelistmodel.h"

ModuleListModel::ModuleListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int ModuleListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_modules.size();
}

QVariant ModuleListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_modules.size()) {
        return {};
    }

    const ModuleEntry &entry = m_modules.at(index.row());

    switch (role) {
    case KeyRole:
        return entry.key;
    case NameRole:
        return entry.name;
    case PresentRole:
        return entry.present;
    case HealthyRole:
        return entry.healthy;
    case ModeRole:
        return entry.mode;
    case DetailsRole:
        return entry.details;
    default:
        return {};
    }
}

QHash<int, QByteArray> ModuleListModel::roleNames() const
{
    return {
        { KeyRole, "moduleKey" },
        { NameRole, "name" },
        { PresentRole, "present" },
        { HealthyRole, "healthy" },
        { ModeRole, "mode" },
        { DetailsRole, "details" },
    };
}

void ModuleListModel::setModules(const QVector<ModuleEntry> &modules)
{
    beginResetModel();
    m_modules = modules;
    endResetModel();
}

bool ModuleListModel::setModuleState(const QString &key,
                                     bool present,
                                     bool healthy,
                                     const QString &mode,
                                     const QString &details)
{
    const int row = indexOf(key);
    if (row < 0) {
        return false;
    }

    ModuleEntry &entry = m_modules[row];
    const bool changed = entry.present != present
        || entry.healthy != healthy
        || entry.mode != mode
        || entry.details != details;

    if (!changed) {
        return false;
    }

    entry.present = present;
    entry.healthy = healthy;
    entry.mode = mode;
    entry.details = details;

    const QModelIndex modelIndex = index(row, 0);
    emit dataChanged(modelIndex, modelIndex, {
        PresentRole,
        HealthyRole,
        ModeRole,
        DetailsRole,
    });
    return true;
}

bool ModuleListModel::isPresent(const QString &key) const
{
    const int row = indexOf(key);
    return row >= 0 ? m_modules.at(row).present : false;
}

bool ModuleListModel::isHealthy(const QString &key) const
{
    const int row = indexOf(key);
    return row >= 0 ? m_modules.at(row).healthy : false;
}

QString ModuleListModel::details(const QString &key) const
{
    const int row = indexOf(key);
    return row >= 0 ? m_modules.at(row).details : QString {};
}

int ModuleListModel::indexOf(const QString &key) const
{
    for (int i = 0; i < m_modules.size(); ++i) {
        if (m_modules.at(i).key == key) {
            return i;
        }
    }

    return -1;
}
