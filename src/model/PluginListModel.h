#ifndef PLUGINLISTMODEL_H
#define PLUGINLISTMODEL_H

#include <QAbstractListModel>
#include <QVector>
#include "UnitInfo.h"

namespace qlogue {

class PluginListModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        FilePathRole,
        IsValidRole,
    };

    explicit PluginListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setUnits(const QVector<UnitInfo> &units);
    const UnitInfo &unitAt(int row) const;
    int count() const { return m_units.size(); }

private:
    QVector<UnitInfo> m_units;
};

} // namespace qlogue

#endif // PLUGINLISTMODEL_H
