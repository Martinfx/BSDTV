#include "jellyfin.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>

static constexpr double ticksPerSecond = 10'000'000.0;
static const char fields[] = "Overview,ProductionYear,OfficialRating,CommunityRating,RunTimeTicks";

JellyfinClient::JellyfinClient(const QString &url, const QString &apiKey, const QString &user, QObject *parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
    , m_url(url.endsWith('/') ? url.chopped(1) : url)
    , m_apiKey(apiKey)
    , m_user(user)
{
}

void JellyfinClient::get(const QString &path, const QUrlQuery &query, JsonCallback done)
{
    QUrl url(m_url + path);
    url.setQuery(query);
    QNetworkRequest request(url);
    request.setRawHeader("X-Emby-Token", m_apiKey.toUtf8());
    request.setRawHeader("Accept", "application/json");
    request.setTransferTimeout(m_timeout);
    QNetworkReply *reply = m_network->get(request);
    connect(reply, &QNetworkReply::finished, this, [reply, path, done] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError)
            return done({}, path + ": " + reply->errorString());
        QJsonParseError error;
        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &error);
        if (error.error != QJsonParseError::NoError)
            return done({}, path + ": " + error.errorString());
        done(doc.isArray() ? QJsonValue(doc.array()) : QJsonValue(doc.object()), QString());
    });
}

void JellyfinClient::withUserId(std::function<void(const QString &, const QString &)> done)
{
    if (!m_userId.isEmpty())
        return done(m_userId, QString());
    QPointer<JellyfinClient> self(this);
    get("/Users", {}, [self, done](const QJsonValue &data, const QString &error) {
        if (!self)
            return;
        if (!error.isEmpty())
            return done({}, error);
        const QJsonArray users = data.toArray();
        if (users.isEmpty())
            return done({}, QStringLiteral("no users on the server"));
        // The configured user, else the first one that is not hidden.
        const QString wanted = self->m_user.toLower();
        QJsonObject match;
        for (const QJsonValue &value : users) {
            const QJsonObject user = value.toObject();
            if (!wanted.isEmpty() && user.value("Name").toString().toLower() == wanted)
                match = user;
        }
        if (match.isEmpty() && !wanted.isEmpty())
            return done({}, QStringLiteral("user '%1' not found").arg(self->m_user));
        for (const QJsonValue &value : users) {
            if (match.isEmpty() && !value.toObject().value("Policy").toObject().value("IsHidden").toBool())
                match = value.toObject();
        }
        if (match.isEmpty())
            match = users.first().toObject();
        self->m_userId = match.value("Id").toString();
        done(self->m_userId, QString());
    });
}

void JellyfinClient::items(const QString &path, QUrlQuery query, bool wrapped, ItemsCallback done)
{
    QPointer<JellyfinClient> self(this);
    withUserId([self, path, query, wrapped, done](const QString &userId, const QString &error) {
        if (!self)
            return;
        if (!error.isEmpty())
            return done({}, error);
        self->get(QString(path).arg(userId), query, [self, wrapped, done](const QJsonValue &data, const QString &error) {
            if (!self)
                return;
            if (!error.isEmpty())
                return done({}, error);
            QVariantList out;
            const QJsonArray list = wrapped ? data.toObject().value("Items").toArray() : data.toArray();
            for (const QJsonValue &item : list)
                out << self->card(item.toObject());
            done(out, QString());
        });
    });
}

void JellyfinClient::resume(ItemsCallback done)
{
    QUrlQuery query{{"Limit", "12"}, {"MediaTypes", "Video"}, {"Fields", fields},
                    {"EnableImageTypes", "Primary,Backdrop,Thumb"}};
    items("/Users/%1/Items/Resume", query, true, std::move(done));
}

void JellyfinClient::latest(ItemsCallback done)
{
    QUrlQuery query{{"Limit", "16"}, {"IncludeItemTypes", "Movie,Series,Episode"}, {"Fields", fields}};
    items("/Users/%1/Items/Latest", query, false, std::move(done));
}

void JellyfinClient::search(const QString &term, ItemsCallback done)
{
    QUrlQuery query{{"SearchTerm", term}, {"Recursive", "true"}, {"Limit", "20"},
                    {"IncludeItemTypes", "Movie,Series,Episode"}, {"Fields", fields}};
    items("/Users/%1/Items", query, true, std::move(done));
}

void JellyfinClient::fetchHome(HomeCallback done)
{
    QPointer<JellyfinClient> self(this);
    resume([self, done](const QVariantList &resumeItems, const QString &error) {
        if (!self)
            return;
        if (!error.isEmpty())
            return done({}, {}, error);
        self->latest([resumeItems, done](const QVariantList &latestItems, const QString &error) {
            done(resumeItems, latestItems, error);
        });
    });
}

QString JellyfinClient::imageUrl(const QString &itemId, const QString &kind, int maxWidth) const
{
    return QStringLiteral("%1/Items/%2/Images/%3?maxWidth=%4&quality=90").arg(m_url, itemId, kind).arg(maxWidth);
}

QString JellyfinClient::webUrl(const QString &itemId) const
{
    return QStringLiteral("%1/web/#/details?id=%2").arg(m_url, itemId);
}

QString JellyfinClient::streamUrl(const QString &itemId) const
{
    return QStringLiteral("%1/Videos/%2/stream?static=true&api_key=%3")
        .arg(m_url, itemId, QString::fromUtf8(QUrl::toPercentEncoding(m_apiKey)));
}

QVariantMap JellyfinClient::card(const QJsonObject &raw) const
{
    const QString id = raw.value("Id").toString();
    const QString kind = raw.value("Type").toString();
    const QJsonObject tags = raw.value("ImageTags").toObject();
    const QJsonObject user = raw.value("UserData").toObject();
    const bool hasBackdrop = !raw.value("BackdropImageTags").toArray().isEmpty();

    QString title, subtitle;
    if (kind == "Episode") {
        title = raw.value("SeriesName").toString(raw.value("Name").toString());
        const QJsonValue season = raw.value("ParentIndexNumber"), episode = raw.value("IndexNumber");
        const QString code = season.isDouble() && episode.isDouble()
            ? QStringLiteral("S%1:E%2 ").arg(season.toInt()).arg(episode.toInt()) : QString();
        subtitle = (code + raw.value("Name").toString()).trimmed();
    } else {
        title = raw.value("Name").toString();
        const int year = raw.value("ProductionYear").toInt();
        subtitle = year ? QString::number(year) : QString();
    }

    // Wide picture for the card: the item's own thumb or backdrop, then the series',
    // then an episode still (episode Primary images are 16:9).
    QString wide;
    if (tags.contains("Thumb"))
        wide = imageUrl(id, "Thumb", 640);
    else if (hasBackdrop)
        wide = imageUrl(id, "Backdrop", 640);
    else if (raw.contains("ParentThumbItemId"))
        wide = imageUrl(raw.value("ParentThumbItemId").toString(), "Thumb", 640);
    else if (tags.contains("Primary"))
        wide = imageUrl(id, "Primary", 640);

    QString backdrop;
    if (hasBackdrop)
        backdrop = imageUrl(id, "Backdrop", 1920);
    else if (raw.contains("ParentBackdropItemId"))
        backdrop = imageUrl(raw.value("ParentBackdropItemId").toString(), "Backdrop", 1920);

    const double ticks = raw.value("RunTimeTicks").toDouble();
    const double position = user.value("PlaybackPositionTicks").toDouble();
    return {
        {"mediaId", id},
        {"type", kind},
        {"title", title},
        {"subtitle", subtitle},
        {"overview", raw.value("Overview").toString()},
        {"year", raw.value("ProductionYear").toInt()},
        {"rating", raw.value("OfficialRating").toString()},
        {"minutes", int(ticks / ticksPerSecond / 60)},
        {"progress", ticks > 0 ? qMin(1.0, position / ticks) : 0.0},
        {"resumeSeconds", int(position / ticksPerSecond)},
        {"image", wide},
        {"backdrop", backdrop},
    };
}

JellyfinClient *JellyfinClient::fromConfig(const QJsonObject &config, QObject *parent)
{
    const QJsonObject opts = config.value("jellyfin").toObject();
    QString url = opts.value("url").toString();
    if (url.isEmpty())
        url = config.value("vars").toObject().value("jellyfin_url").toString();
    const QString key = opts.value("api_key").toString();
    if (url.isEmpty() || key.isEmpty())
        return nullptr;
    return new JellyfinClient(url, key, opts.value("user").toString(), parent);
}

QVariantList JellyfinClient::slides(const QVariantList &items, int limit)
{
    QVariantList out;
    QStringList seen;
    for (const QVariant &value : items) {
        const QVariantMap item = value.toMap();
        const QString id = item.value("mediaId").toString();
        if (item.value("backdrop").toString().isEmpty() || seen.contains(id))
            continue;
        seen << id;
        const QString overview = item.value("overview").toString();
        out << QVariantMap{
            {"appId", "jellyfin"}, {"name", "Jellyfin"}, {"title", item.value("title")},
            {"tagline", overview.isEmpty() ? item.value("subtitle").toString() : overview},
            {"image", item.value("backdrop")}, {"color", "#101018"}, {"textColor", "#aa5cc3"},
            {"icon", ""}, {"caption", ""}, {"mediaId", id},
        };
        if (out.size() >= limit)
            break;
    }
    return out;
}
