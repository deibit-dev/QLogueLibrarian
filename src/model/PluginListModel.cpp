#include "PluginListModel.h"

namespace qlogue {

PluginListModel::PluginListModel(QObject *parent)
    : QAbstractListModel(parent)
{}

int PluginListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_units.size();
}

QVariant PluginListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_units.size())
        return {};

    const UnitInfo &u = m_units.at(index.row());
    switch (role) {
    case NameRole:
        return u.isValid && !u.name.trimmed().isEmpty()
                   ? u.name.trimmed()
                   : u.fileName;
    case FilePathRole: return u.filePath;
    case IsValidRole:  return u.isValid;
    }
    return {};
}

QHash<int, QByteArray> PluginListModel::roleNames() const
{
    return {
        { NameRole,     "name"     },
        { FilePathRole, "filePath" },
        { IsValidRole,  "isValid"  },
    };
}

void PluginListModel::setUnits(const QVector<UnitInfo> &units)
{
    beginResetModel();
    m_units = units;
    endResetModel();
}

const UnitInfo &PluginListModel::unitAt(int row) const
{
    return m_units.at(row);
}

} // namespace qlogue
