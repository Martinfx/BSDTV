#include "config.h"
#include "testutil.h"

#include <QJsonArray>
#include <QTest>

class TestConfig : public QObject
{
    Q_OBJECT

private slots:
    void deepMergeMergesObjectsAndReplacesArrays()
    {
        const QJsonObject base{{"a", 1}, {"nested", QJsonObject{{"x", 1}, {"y", 2}}}, {"list", QJsonArray{1, 2}}};
        const QJsonObject merged = config::deepMerge(base, {{"nested", QJsonObject{{"y", 3}}}, {"list", QJsonArray{9}}});
        QCOMPARE(merged, (QJsonObject{{"a", 1}, {"nested", QJsonObject{{"x", 1}, {"y", 3}}}, {"list", QJsonArray{9}}}));
        QCOMPARE(base["nested"].toObject()["y"].toInt(), 2);
    }

    void userConfigOverridesSystemConfig()
    {
        IsolatedConfig env;
        env.write(env.etc() + "/config.json",
                  {{"columns", 6}, {"vars", QJsonObject{{"jellyfin_url", "http://sys"}}}});
        config::saveUserSetting("columns", 4);
        const QJsonObject cfg = config::load();
        QCOMPARE(cfg["columns"].toInt(), 4);
        QCOMPARE(cfg["vars"].toObject()["jellyfin_url"].toString(), "http://sys");
        QCOMPARE(cfg["vars"].toObject()["youtube_url"].toString(), config::defaults()["vars"].toObject()["youtube_url"].toString());
    }

    void brokenJsonIsIgnored()
    {
        IsolatedConfig env;
        QDir().mkpath(env.user());
        QFile file(env.user() + "/config.json");
        file.open(QIODevice::WriteOnly);
        file.write("{nope");
        file.close();
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression("ignoring unreadable"));
        QCOMPARE(config::load()["theme"].toString(), "graphite");
    }

    void saveUserSettingKeepsOtherKeys()
    {
        IsolatedConfig env;
        config::saveUserSetting("theme", "aurora");
        config::saveUserSetting("columns", 7);
        QCOMPARE(config::readJson(env.user() + "/config.json"), (QJsonObject{{"theme", "aurora"}, {"columns", 7}}));
    }

    void bundledThemesAreComplete()
    {
        IsolatedConfig env;
        const auto themes = config::listThemes();
        for (const char *id : {"graphite", "midnight", "aurora", "sunrise", "beastie"})
            QVERIFY2(themes.contains(id), id);
        for (const QJsonObject &theme : themes) {
            for (const QString &key : config::themeDefaults().keys())
                QVERIFY(theme.contains(key));
        }
    }

    void unknownThemeFallsBackToGraphite()
    {
        IsolatedConfig env;
        const QJsonObject theme = config::loadTheme("does-not-exist");
        QCOMPARE(theme["name"].toString(), "Graphite");
        QCOMPARE(theme["tiles"].toString(), "flat");
    }

    void profilesOverrideSettings()
    {
        IsolatedConfig env;
        env.write(env.etc() + "/config.json", {
            {"dock", QJsonArray{"kodi"}},
            {"profiles", QJsonArray{
                QJsonObject{{"id", "dad"}, {"name", "Dad"}},
                QJsonObject{{"id", "kids"}, {"name", "Kids"}, {"dock", QJsonArray{"youtube"}},
                            {"theme", "sunrise"}, {"jellyfin_user", "kids"}}}},
        });
        QCOMPARE(config::load()["dock"].toArray(), QJsonArray{"kodi"});
        QCOMPARE(config::activeProfile(config::load())["id"].toString(), "dad");

        config::saveUserSetting("profile", "kids");
        const QJsonObject cfg = config::load();
        QCOMPARE(cfg["dock"].toArray(), QJsonArray{"youtube"});
        QCOMPARE(cfg["theme"].toString(), "sunrise");
        QCOMPARE(cfg["jellyfin"].toObject()["user"].toString(), "kids");
    }

    void withoutProfilesThereIsOneDefault()
    {
        IsolatedConfig env;
        const QJsonArray found = config::profiles(config::load());
        QCOMPARE(found.size(), 1);
        QCOMPARE(found[0].toObject()["id"].toString(), "");
    }
};

QTEST_GUILESS_MAIN(TestConfig)
#include "tst_config.moc"
