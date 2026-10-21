#include "apps.h"

#include "config.h"

#include <QDir>
#include <QJsonArray>
#include <QRegularExpression>
#include <QSet>
#include <QStandardPaths>

static QString expandHome(const QString &path)
{
    if (path == "~")
        return QDir::homePath();
    if (path.startsWith("~/"))
        return QDir::homePath() + path.mid(1);
    return path;
}

static QStringList toStringList(const QJsonValue &value)
{
    QStringList out;
    for (const QJsonValue &item : value.toArray())
        out << item.toString();
    return out;
}

QStringList App::command(const QJsonObject &vars) const
{
    QStringList argv;
    for (const QString &arg : exec)
        argv << apps::expand(arg, vars);
    return argv;
}

QVariantMap App::toMap() const
{
    return {
        {"appId", id},
        {"name", name},
        {"caption", caption},
        {"color", color},
        {"icon", icon},
        {"tagline", tagline.isEmpty() ? caption : tagline},
        {"image", ""},
        {"textColor", textColor},
    };
}

namespace apps {

QString which(const QString &program)
{
    if (program.isEmpty())
        return {};
    return QStandardPaths::findExecutable(program);
}

QString expand(const QString &text, const QJsonObject &vars)
{
    static const QRegularExpression placeholder(QStringLiteral("\\{([a-z0-9_]+)\\}"));
    QString out;
    qsizetype last = 0;
    auto it = placeholder.globalMatch(text);
    while (it.hasNext()) {
        const auto match = it.next();
        out += text.mid(last, match.capturedStart() - last);
        const QJsonValue value = vars.value(match.captured(1));
        out += value.isUndefined() || value.isNull() ? match.captured(0) : expandHome(value.toVariant().toString());
        last = match.capturedEnd();
    }
    return out + text.mid(last);
}

QString localized(const QJsonValue &value, const QString &lang)
{
    if (value.isObject()) {
        const QJsonObject map = value.toObject();
        for (const QString &key : {lang, QStringLiteral("en")}) {
            const QString text = map.value(key).toString();
            if (!text.isEmpty())
                return text;
        }
        return map.isEmpty() ? QString() : map.begin()->toString();
    }
    return value.toString();
}

QList<App> parse(const QJsonObject &data, const QString &lang)
{
    QList<App> out;
    QSet<QString> seen;
    for (const QJsonValue &value : data.value("apps").toArray()) {
        const QJsonObject raw = value.toObject();
        const QString id = raw.value("id").toString();
        const QStringList exec = toStringList(raw.value("exec"));
        if (id.isEmpty() || exec.isEmpty() || seen.contains(id))
            continue;
        seen.insert(id);
        App app;
        app.id = id;
        app.name = localized(raw.value("name"), lang);
        if (app.name.isEmpty())
            app.name = id;
        app.exec = exec;
        app.caption = localized(raw.value("caption"), lang);
        app.color = raw.value("color").toString(app.color);
        app.icon = raw.value("icon").toString();
        app.pkg = raw.value("pkg").toString();
        app.requiredVars = toStringList(raw.value("requires"));
        app.category = raw.value("category").toString();
        app.tagline = localized(raw.value("tagline"), lang);
        app.textColor = raw.value("text_color").toString(app.textColor);
        out << app;
    }
    return out;
}

bool isInstalled(const App &app, const QJsonObject &vars, const Which &which)
{
    if (app.exec.isEmpty())
        return false;
    for (const QString &name : app.requiredVars) {
        if (vars.value(name).toVariant().toString().isEmpty())
            return false;
    }
    return !which(expand(app.exec.first(), vars)).isEmpty();
}

QList<App> load(const QJsonObject &config, const QString &lang, const Which &which)
{
    QJsonArray entries;
    for (const QString &path : {config::findDataFile("apps.json"), config::userDir() + "/apps.json"}) {
        if (path.isEmpty())
            continue;
        // User entries come first so they win over system entries with the same id.
        QJsonArray merged = config::readJson(path).value("apps").toArray();
        for (const QJsonValue &value : entries)
            merged.append(value);
        entries = merged;
    }
    const QJsonObject vars = config.value("vars").toObject();
    const QStringList hidden = toStringList(config.value("hidden"));
    QList<App> out;
    for (const App &app : parse(QJsonObject{{"apps", entries}}, lang)) {
        if (!hidden.contains(app.id) && isInstalled(app, vars, which))
            out << app;
    }
    return out;
}

QList<App> orderDock(const QList<App> &apps, const QStringList &dockIds)
{
    QList<App> out;
    for (const QString &id : dockIds) {
        for (const App &app : apps) {
            if (app.id == id)
                out << app;
        }
    }
    return out;
}

static const QStringList categoryRows{"streaming", "media", "games"};

QVariantList homeRows(const QList<App> &apps, const QStringList &dockIds)
{
    QVariantList rows;
    for (const QString &category : categoryRows) {
        QVariantList members;
        for (const App &app : apps) {
            if (app.category == category)
                members << app.toMap();
        }
        if (!members.isEmpty())
            rows << QVariantMap{{"key", category}, {"style", "cards"}, {"apps", members}};
    }
    if (!apps.isEmpty()) {
        const QList<App> favourites = orderDock(apps, dockIds);
        QVariantList yours;
        for (const App &app : favourites)
            yours << app.toMap();
        for (const App &app : apps) {
            if (!dockIds.contains(app.id))
                yours << app.toMap();
        }
        rows.insert(qMin<qsizetype>(1, rows.size()),
                    QVariantMap{{"key", "your_apps"}, {"style", "round"}, {"apps", yours}});
    }
    return rows;
}

QVariantList featured(const QList<App> &apps, const QJsonObject &config)
{
    QJsonArray entries = config.value("featured").toArray();
    if (entries.isEmpty()) {
        for (const QJsonValue &id : config.value("dock").toArray())
            entries.append(QJsonObject{{"app", id}});
    }
    QVariantList slides;
    for (const QJsonValue &value : entries) {
        const QJsonObject entry = value.toObject();
        for (const App &app : apps) {
            if (app.id != entry.value("app").toString())
                continue;
            QVariantMap slide = app.toMap();
            for (const char *key : {"title", "tagline", "image", "color"}) {
                const QString text = entry.value(key).toString();
                if (!text.isEmpty())
                    slide[key] = text;
            }
            slides << slide;
        }
    }
    if (slides.isEmpty()) {
        for (const App &app : apps.mid(0, 5))
            slides << app.toMap();
    }
    return slides;
}

QVariantList withMedia(const QVariantList &rows, const QVariantList &resume, const QVariantList &latest)
{
    QVariantList out = rows;
    if (!latest.isEmpty()) {
        qsizetype after = out.size();
        for (qsizetype i = 0; i < out.size(); ++i) {
            if (out.at(i).toMap().value("key") == "your_apps")
                after = i + 1;
        }
        out.insert(after, QVariantMap{{"key", "latest"}, {"style", "media"}, {"items", latest}});
    }
    if (!resume.isEmpty())
        out.prepend(QVariantMap{{"key", "continue"}, {"style", "media"}, {"items", resume}});
    return out;
}

QVariantList search(const QList<App> &apps, const QString &term)
{
    const QStringList words = term.toLower().split(' ', Qt::SkipEmptyParts);
    QVariantList out;
    if (words.isEmpty())
        return out;
    for (const App &app : apps) {
        const QString haystack = (app.name + " " + app.caption).toLower();
        if (std::all_of(words.begin(), words.end(), [&](const QString &w) { return haystack.contains(w); }))
            out << app.toMap();
    }
    return out;
}

} // namespace apps
