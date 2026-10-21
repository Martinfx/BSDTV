#include "apps.h"
#include "config.h"
#include "testutil.h"

#include <QJsonArray>
#include <QTest>

static apps::Which fakeWhich(const QStringList &present)
{
    return [present](const QString &name) {
        return present.contains(name) ? QStringLiteral("/usr/local/bin/") + name : QString();
    };
}

static QStringList ids(const QList<App> &list)
{
    QStringList out;
    for (const App &app : list)
        out << app.id;
    return out;
}

static QStringList ids(const QVariantList &list, const char *key = "appId")
{
    QStringList out;
    for (const QVariant &item : list)
        out << item.toMap().value(key).toString();
    return out;
}

class TestApps : public QObject
{
    Q_OBJECT

private slots:
    void expandFillsKnownPlaceholdersAndKeepsUnknown()
    {
        QCOMPARE(apps::expand("{a}/{missing}", {{"a", "x"}}), "x/{missing}");
        QVERIFY(!apps::expand("{p}", {{"p", "~/x"}}).startsWith("~"));
    }

    void localizedPicksLanguageThenEnglish()
    {
        const QJsonObject text{{"en", "Music"}, {"cs", "Hudba"}};
        QCOMPARE(apps::localized(text, "cs"), "Hudba");
        QCOMPARE(apps::localized(text, "de"), "Music");
        QCOMPARE(apps::localized("plain", "cs"), "plain");
    }

    void parseSkipsInvalidAndDuplicateEntries()
    {
        const QJsonObject data{{"apps", QJsonArray{
            QJsonObject{{"id", "a"}, {"exec", QJsonArray{"a"}}},
            QJsonObject{{"id", "a"}, {"exec", QJsonArray{"b"}}},
            QJsonObject{{"name", "no id"}},
            "junk",
            QJsonObject{{"id", "b"}},
        }}};
        const QList<App> parsed = apps::parse(data);
        QCOMPARE(ids(parsed), QStringList{"a"});
        QCOMPARE(parsed.first().exec, QStringList{"a"});
    }

    void bundledCatalogueFiltersByBinaryAndVars()
    {
        IsolatedConfig env;
        QJsonObject cfg = config::load();
        QCOMPARE(ids(apps::load(cfg, "en", fakeWhich({"kodi", "firefox"}))),
                 (QStringList{"kodi", "youtube", "browser"}));
        QJsonObject vars = cfg["vars"].toObject();
        vars["jellyfin_url"] = "http://media:8096";
        cfg["vars"] = vars;
        QVERIFY(ids(apps::load(cfg, "en", fakeWhich({"firefox"}))).contains("jellyfin"));
    }

    void hiddenAppsAreDropped()
    {
        IsolatedConfig env;
        QJsonObject cfg = config::load();
        cfg["hidden"] = QJsonArray{"youtube"};
        QVERIFY(!ids(apps::load(cfg, "en", fakeWhich({"firefox"}))).contains("youtube"));
    }

    void userAppsOverrideSystemApps()
    {
        IsolatedConfig env;
        env.write(env.user() + "/apps.json", {{"apps", QJsonArray{
            QJsonObject{{"id", "kodi"}, {"name", "My Kodi"}, {"exec", QJsonArray{"kodi"}}},
            QJsonObject{{"id", "plex"}, {"name", "Plex"}, {"exec", QJsonArray{"plexhtpc"}}},
        }}});
        const QList<App> found = apps::load(config::load(), "en", fakeWhich({"kodi", "plexhtpc"}));
        QCOMPARE(found.first().name, "My Kodi");
        QVERIFY(ids(found).contains("plex"));
    }

    void captionsAreLocalized()
    {
        IsolatedConfig env;
        const auto cs = apps::load(config::load(), "cs", fakeWhich({"kodi"}));
        QCOMPARE(cs.first().caption, "Filmy, seriály, hudba a živá TV");
        const auto en = apps::load(config::load(), "xx", fakeWhich({"kodi"}));
        QCOMPARE(en.first().caption, "Movies, series, music and live TV");
    }

    void youtubeUsesItsOwnFirefoxProfile()
    {
        IsolatedConfig env;
        const QJsonObject cfg = config::load();
        for (const App &app : apps::load(cfg, "en", fakeWhich({"firefox"}))) {
            if (app.id != "youtube")
                continue;
            const QStringList argv = app.command(cfg["vars"].toObject());
            QCOMPARE(argv.first(), "firefox");
            QCOMPARE(argv.last(), "https://www.youtube.com/tv");
            QVERIFY(argv.at(argv.indexOf("--profile") + 1).endsWith("firefox-youtube"));
            return;
        }
        QFAIL("youtube missing");
    }

    void streamingAppsNeedTheDrmBrowser()
    {
        IsolatedConfig env;
        QJsonObject cfg = config::load();
        QVERIFY(!ids(apps::load(cfg, "en", fakeWhich({"bsdtv-drm-browser", "firefox"}))).contains("netflix"));
        QJsonObject vars = cfg["vars"].toObject();
        vars["drm_browser"] = "/usr/local/bin/bsdtv-drm-browser";
        cfg["vars"] = vars;
        const auto found = apps::load(cfg, "en", [](const QString &p) { return p.endsWith("bsdtv-drm-browser") ? p : QString(); });
        for (const char *id : {"netflix", "disneyplus", "primevideo", "max", "spotify"})
            QVERIFY2(ids(found).contains(id), id);
        QCOMPARE(found.first().command(vars), (QStringList{"/usr/local/bin/bsdtv-drm-browser", "https://www.netflix.com/"}));
    }

    void placeholderProgramIsLookedUpAfterExpansion()
    {
        App app;
        app.id = "x";
        app.exec = {"{prog}"};
        QVERIFY(apps::isInstalled(app, {{"prog", "mpv"}}, fakeWhich({"mpv"})));
        QVERIFY(!apps::isInstalled(app, {{"prog", "nope"}}, fakeWhich({"mpv"})));
    }

    void orderDockFollowsConfigAndSkipsMissing()
    {
        const auto items = apps::parse({{"apps", QJsonArray{
            QJsonObject{{"id", "a"}, {"exec", QJsonArray{"a"}}},
            QJsonObject{{"id", "b"}, {"exec", QJsonArray{"b"}}},
            QJsonObject{{"id", "c"}, {"exec", QJsonArray{"c"}}}}}});
        QCOMPARE(ids(apps::orderDock(items, {"c", "x", "a"})), (QStringList{"c", "a"}));
    }

    void homeRowsPutYourAppsSecond()
    {
        IsolatedConfig env;
        const auto found = apps::load(config::load(), "en", fakeWhich({"kodi", "firefox", "moonlight"}));
        const QVariantList rows = apps::homeRows(found, {"youtube", "kodi"});
        QCOMPARE(ids(rows, "key"), (QStringList{"streaming", "your_apps", "media", "games"}));
        QCOMPARE(rows[1].toMap()["style"].toString(), "round");
        QCOMPARE(ids(rows[1].toMap()["apps"].toList()).mid(0, 2), (QStringList{"youtube", "kodi"}));
        QCOMPARE(ids(rows[3].toMap()["apps"].toList()), QStringList{"moonlight"});
        QVERIFY(apps::homeRows({}, {}).isEmpty());
    }

    void featuredUsesConfigEntriesOrDock()
    {
        const auto found = apps::parse({{"apps", QJsonArray{
            QJsonObject{{"id", "a"}, {"exec", QJsonArray{"a"}}},
            QJsonObject{{"id", "b"}, {"exec", QJsonArray{"b"}}},
            QJsonObject{{"id", "c"}, {"exec", QJsonArray{"c"}}}}}});
        QCOMPARE(ids(apps::featured(found, {{"dock", QJsonArray{"c", "a"}}})), (QStringList{"c", "a"}));
        const QVariantList slides = apps::featured(found, {{"featured", QJsonArray{
            QJsonObject{{"app", "b"}, {"title", "Movie night"}, {"image", "/x.jpg"}}, QJsonObject{{"app", "missing"}}}}});
        QCOMPARE(slides.size(), 1);
        QCOMPARE(slides[0].toMap()["title"].toString(), "Movie night");
        QCOMPARE(slides[0].toMap()["image"].toString(), "/x.jpg");
        QCOMPARE(slides[0].toMap()["name"].toString(), "b");
    }

    void mediaRowsArePlacedAroundAppRows()
    {
        const QVariantList rows{QVariantMap{{"key", "streaming"}}, QVariantMap{{"key", "your_apps"}},
                                QVariantMap{{"key", "games"}}};
        const QVariantList merged = apps::withMedia(rows, {QVariantMap{{"mediaId", "r"}}}, {QVariantMap{{"mediaId", "l"}}});
        QCOMPARE(ids(merged, "key"), (QStringList{"continue", "streaming", "your_apps", "latest", "games"}));
        QCOMPARE(apps::withMedia(rows, {}, {}), rows);
    }

    void searchMatchesAllWords()
    {
        const auto found = apps::parse({{"apps", QJsonArray{
            QJsonObject{{"id", "a"}, {"name", "Kodi"}, {"caption", "Movies and music"}, {"exec", QJsonArray{"a"}}},
            QJsonObject{{"id", "b"}, {"name", "Spotify"}, {"caption", "Music and podcasts"}, {"exec", QJsonArray{"b"}}}}}});
        QCOMPARE(ids(apps::search(found, "music")), (QStringList{"a", "b"}));
        QCOMPARE(ids(apps::search(found, "MUSIC pod")), QStringList{"b"});
        QVERIFY(apps::search(found, "  ").isEmpty());
    }
};

QTEST_GUILESS_MAIN(TestApps)
#include "tst_apps.moc"
