// List model of apps for the classic home screen's dock and grid.
#pragma once

#include "apps.h"

#include <QAbstractListModel>

class AppModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role { AppIdRole = Qt::UserRole + 1, NameRole, CaptionRole, ColorRole, IconRole, TextColorRole };

    using QAbstractListModel::QAbstractListModel;

    void setApps(const QList<App> &apps);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    QList<App> m_apps;
};
