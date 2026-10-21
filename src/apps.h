// The app catalogue: which apps exist, how to start them and whether they are installed.
#pragma once

#include <QJsonObject>
#include <QList>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

#include <functional>

struct App
{
    QString id;
    QString name;
    QStringList exec;
    QString caption;
    QString color = QStringLiteral("#3a3f55");
    QString icon;
    QString pkg;
    QStringList requiredVars;
    QString category;
    QString tagline;
    QString textColor = QStringLiteral("#ffffff");

    // The argv to run, with {placeholders} filled from `vars`.
    QStringList command(const QJsonObject &vars) const;
    QVariantMap toMap() const;
};

namespace apps {

using Which = std::function<QString(const QString &)>;
QString which(const QString &program);

QString expand(const QString &text, const QJsonObject &vars);
// Text fields may be plain strings or {"en": ..., "cs": ...} maps.
QString localized(const QJsonValue &value, const QString &lang);

QList<App> parse(const QJsonObject &data, const QString &lang = QStringLiteral("en"));
// An app is usable when every required variable is set and its program exists.
bool isInstalled(const App &app, const QJsonObject &vars, const Which &which = apps::which);
// Installed, non-hidden apps in catalogue order (the user's apps.json before the system one).
QList<App> load(const QJsonObject &config, const QString &lang = QStringLiteral("en"),
                const Which &which = apps::which);
QList<App> orderDock(const QList<App> &apps, const QStringList &dockIds);

// Rows for the TV-style home: one row of wide cards per category, with "Your apps"
// (round icons, favourites first) as the second row.
QVariantList homeRows(const QList<App> &apps, const QStringList &dockIds);
// Banner slides: explicit "featured" entries from the config, else the favourite apps.
QVariantList featured(const QList<App> &apps, const QJsonObject &config);
// Jellyfin rows: "Continue watching" first, "Recently added" after "Your apps".
QVariantList withMedia(const QVariantList &rows, const QVariantList &resume, const QVariantList &latest);
// Apps whose name or caption contains every word of the query.
QVariantList search(const QList<App> &apps, const QString &term);

} // namespace apps
