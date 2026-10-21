#include "appmodel.h"

void AppModel::setApps(const QList<App> &apps)
{
    beginResetModel();
    m_apps = apps;
    endResetModel();
}

int AppModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_apps.size());
}

QVariant AppModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_apps.size())
        return {};
    const App &app = m_apps.at(index.row());
    switch (role) {
    case AppIdRole: return app.id;
    case NameRole: return app.name;
    case CaptionRole: return app.caption;
    case ColorRole: return app.color;
    case IconRole: return app.icon;
    case TextColorRole: return app.textColor;
    }
    return {};
}

QHash<int, QByteArray> AppModel::roleNames() const
{
    return {
        {AppIdRole, "appId"},
        {NameRole, "name"},
        {CaptionRole, "caption"},
        {ColorRole, "color"},
        {IconRole, "icon"},
        {TextColorRole, "textColor"},
    };
}
