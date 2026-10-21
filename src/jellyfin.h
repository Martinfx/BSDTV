// Jellyfin client for the home screen: continue watching, latest additions, search.
//
// Uses the Jellyfin HTTP API with an API key (Dashboard → API Keys). Only reads
// data; playback happens in the Jellyfin web app or mpv. All requests are
// asynchronous, so the home screen never waits for the server.
#pragma once

#include <QJsonObject>
#include <QObject>
#include <QUrlQuery>
#include <QVariantList>
#include <QVariantMap>

#include <functional>

class QNetworkAccessManager;

class JellyfinClient : public QObject
{
    Q_OBJECT

public:
    // Called with the result, or with an error message (empty on success).
    using ItemsCallback = std::function<void(const QVariantList &items, const QString &error)>;
    using HomeCallback = std::function<void(const QVariantList &resume, const QVariantList &latest,
                                            const QString &error)>;

    JellyfinClient(const QString &url, const QString &apiKey, const QString &user = QString(),
                   QObject *parent = nullptr);

    QString url() const { return m_url; }
    QString apiKey() const { return m_apiKey; }
    QString user() const { return m_user; }
    void setTimeout(int ms) { m_timeout = ms; }

    void fetchHome(HomeCallback done);
    void resume(ItemsCallback done);
    void latest(ItemsCallback done);
    void search(const QString &term, ItemsCallback done);

    QString imageUrl(const QString &itemId, const QString &kind, int maxWidth) const;
    QString webUrl(const QString &itemId) const;
    QString streamUrl(const QString &itemId) const;

    // Turn a Jellyfin item into what the home screen shows.
    QVariantMap card(const QJsonObject &raw) const;

    // A client when Jellyfin is configured with a URL and an API key, else nullptr.
    static JellyfinClient *fromConfig(const QJsonObject &config, QObject *parent = nullptr);
    // Banner slides for items that have a backdrop.
    static QVariantList slides(const QVariantList &items, int limit = 5);

private:
    using JsonCallback = std::function<void(const QJsonValue &data, const QString &error)>;
    void get(const QString &path, const QUrlQuery &query, JsonCallback done);
    void withUserId(std::function<void(const QString &userId, const QString &error)> done);
    void items(const QString &path, QUrlQuery query, bool wrapped, ItemsCallback done);

    QNetworkAccessManager *m_network;
    QString m_url;
    QString m_apiKey;
    QString m_user;
    QString m_userId;
    int m_timeout = 8000;
};
