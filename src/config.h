// Configuration loading.
//
// Configuration is layered: built-in defaults, then the system file
// (/usr/local/etc/bsdtv/config.json), then the per-user file
// (~/.config/bsdtv/config.json), then the active profile. Settings changed
// from the TV are written to the user file only, so the system file can be
// managed by the installer.
#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QString>
#include <QStringList>

namespace config {

QJsonObject defaults();

QString systemDir();
QString userDir();
// Existing path of a data file such as apps.json (system dir, installed defaults, source tree).
QString findDataFile(const QString &name);
QStringList themeDirs();

QJsonObject deepMerge(const QJsonObject &base, const QJsonObject &over);
QJsonObject readJson(const QString &path);
bool writeJson(const QString &path, const QJsonObject &data);

QJsonObject load();
void saveUserSetting(const QString &key, const QJsonValue &value);

// Settings a profile can override.
extern const QStringList profileKeys;
QJsonArray profiles(const QJsonObject &config);
QJsonObject activeProfile(const QJsonObject &config);
QJsonObject applyProfile(const QJsonObject &config);

QJsonObject themeDefaults();
QMap<QString, QJsonObject> listThemes();
QJsonObject loadTheme(const QString &id);

} // namespace config
