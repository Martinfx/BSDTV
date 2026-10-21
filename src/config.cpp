#include "config.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QSaveFile>

namespace config {

QJsonObject defaults()
{
    return QJsonObject{
        {"theme", "graphite"},
        {"layout", "tv"},
        {"language", "auto"},
        {"featured", QJsonArray{}},
        {"dock", QJsonArray{"kodi", "youtube", "jellyfin", "browser"}},
        {"hidden", QJsonArray{}},
        {"columns", 5},
        {"fullscreen", true},
        {"reduced_motion", false},
        {"show_stats", false},
        {"greeting", true},
        {"clock_24h", true},
        {"remote", QJsonObject{{"enabled", true}, {"bind", "0.0.0.0"}, {"port", 8765}}},
        {"cec", QJsonObject{{"enabled", false}, {"device", ""}}},
        {"jellyfin", QJsonObject{{"url", ""}, {"api_key", ""}, {"user", ""}}},
        {"ambient", QJsonObject{{"enabled", true}, {"after_minutes", 10}, {"photos", "~/Pictures"}}},
        // Each profile may override profileKeys, e.g.
        // {"id": "kids", "name": "Kids", "color": "#f6ae2d", "dock": ["youtube"], "jellyfin_user": "kids"}
        {"profiles", QJsonArray{}},
        {"profile", ""},
        {"vars", QJsonObject{
            {"jellyfin_url", ""},
            {"iptv_playlist", ""},
            {"music_dir", ""},
            {"firefox_profile", "~/.local/share/bsdtv/firefox"},
            {"youtube_profile", "~/.local/share/bsdtv/firefox-youtube"},
            {"youtube_url", "https://www.youtube.com/tv"},
            {"drm_browser", ""},
        }},
    };
}

QString systemDir()
{
    const QString env = qEnvironmentVariable("BSDTV_ETC");
    return env.isEmpty() ? QStringLiteral("/usr/local/etc/bsdtv") : env;
}

QString userDir()
{
    QString base = qEnvironmentVariable("XDG_CONFIG_HOME");
    if (base.isEmpty())
        base = QDir::homePath() + "/.config";
    return base + "/bsdtv";
}

static QStringList dataRoots()
{
    // Installed defaults, then the source tree (for running a build without installing).
    return {QStringLiteral(BSDTV_DATA_DIR), QStringLiteral(BSDTV_SOURCE_DIR)};
}

QString findDataFile(const QString &name)
{
    QStringList candidates{systemDir() + "/" + name};
    for (const QString &root : dataRoots())
        candidates << root + "/etc/" + name;
    for (const QString &path : candidates) {
        if (QFileInfo::exists(path))
            return path;
    }
    return {};
}

QStringList themeDirs()
{
    QStringList dirs{systemDir() + "/themes"};
    for (const QString &root : dataRoots())
        dirs << root + "/themes";
    return dirs;
}

QJsonObject deepMerge(const QJsonObject &base, const QJsonObject &over)
{
    QJsonObject result = base;
    for (auto it = over.begin(); it != over.end(); ++it) {
        if (it->isObject() && result.value(it.key()).isObject())
            result[it.key()] = deepMerge(result.value(it.key()).toObject(), it->toObject());
        else
            result[it.key()] = *it;
    }
    return result;
}

QJsonObject readJson(const QString &path)
{
    QFile file(path);
    if (!file.exists())
        return {};
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning("bsdtv: ignoring unreadable %s", qPrintable(path));
        return {};
    }
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning("bsdtv: ignoring unreadable %s: %s", qPrintable(path), qPrintable(error.errorString()));
        return {};
    }
    return doc.object();
}

bool writeJson(const QString &path, const QJsonObject &data)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    file.write(QJsonDocument(data).toJson(QJsonDocument::Indented));
    return file.commit();
}

QJsonObject load()
{
    QJsonObject config = defaults();
    for (const QString &path : {findDataFile("config.json"), userDir() + "/config.json"}) {
        if (!path.isEmpty())
            config = deepMerge(config, readJson(path));
    }
    return applyProfile(config);
}

void saveUserSetting(const QString &key, const QJsonValue &value)
{
    const QString path = userDir() + "/config.json";
    QJsonObject data = readJson(path);
    data[key] = value;
    if (!writeJson(path, data))
        qWarning("bsdtv: cannot write %s", qPrintable(path));
}

const QStringList profileKeys{"dock", "hidden", "theme", "language", "featured", "layout", "ambient"};

static QJsonObject defaultProfile()
{
    return {{"id", ""}, {"name", "BSDTV"}, {"color", "#8ab4f8"}};
}

QJsonArray profiles(const QJsonObject &config)
{
    QJsonArray found;
    for (const QJsonValue &value : config.value("profiles").toArray()) {
        const QJsonObject profile = value.toObject();
        if (!profile.value("id").toString().isEmpty())
            found.append(deepMerge(defaultProfile(), profile));
    }
    if (found.isEmpty())
        found.append(defaultProfile());
    return found;
}

QJsonObject activeProfile(const QJsonObject &config)
{
    const QJsonArray found = profiles(config);
    const QString wanted = config.value("profile").toString();
    for (const QJsonValue &value : found) {
        if (value.toObject().value("id").toString() == wanted)
            return value.toObject();
    }
    return found.first().toObject();
}

QJsonObject applyProfile(const QJsonObject &config)
{
    const QJsonObject profile = activeProfile(config);
    QJsonObject overrides;
    for (const QString &key : profileKeys) {
        if (profile.contains(key))
            overrides[key] = profile.value(key);
    }
    const QString user = profile.value("jellyfin_user").toString();
    if (!user.isEmpty())
        overrides["jellyfin"] = QJsonObject{{"user", user}};
    return deepMerge(config, overrides);
}

QJsonObject themeDefaults()
{
    return {
        {"name", "Graphite"},
        {"background", "#111214"},
        {"background2", "#1b1c1f"},
        {"surface", "#2a2b2f"},
        {"text", "#e8eaed"},
        {"subtext", "#9aa0a6"},
        {"accent", "#8ab4f8"},
        {"focus", "#e8eaed"},
        {"wallpaper", ""},
        // "gradient": glossy tiles in the app colour; "flat": solid brand-colour tiles like Android TV.
        {"tiles", "flat"},
    };
}

QMap<QString, QJsonObject> listThemes()
{
    QMap<QString, QJsonObject> themes;
    QStringList dirs = themeDirs();
    // Earlier directories win, so read them last.
    std::reverse(dirs.begin(), dirs.end());
    for (const QString &dir : dirs) {
        const QFileInfoList entries = QDir(dir).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        for (const QFileInfo &entry : entries) {
            const QString file = entry.absoluteFilePath() + "/theme.json";
            if (!QFileInfo::exists(file))
                continue;
            QJsonObject theme = deepMerge(themeDefaults(), readJson(file));
            const QString wallpaper = theme.value("wallpaper").toString();
            if (!wallpaper.isEmpty() && QFileInfo(wallpaper).isRelative())
                theme["wallpaper"] = entry.absoluteFilePath() + "/" + wallpaper;
            themes.insert(entry.fileName(), theme);
        }
    }
    return themes;
}

QJsonObject loadTheme(const QString &id)
{
    const auto themes = listThemes();
    if (themes.contains(id))
        return themes.value(id);
    return themes.value("graphite", themeDefaults());
}

} // namespace config
