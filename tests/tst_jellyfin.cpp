#include "fakejellyfin.h"
#include "jellyfin.h"

#include <QSignalSpy>
#include <QTest>

static QStringList field(const QVariantList &items, const char *key)
{
    QStringList out;
    for (const QVariant &item : items)
        out << item.toMap().value(key).toString();
    return out;
}

class TestJellyfin : public QObject
{
    Q_OBJECT

    FakeJellyfin server;

    // Runs an async call and waits for its callback.
    QPair<QVariantList, QString> wait(const std::function<void(JellyfinClient::ItemsCallback)> &call)
    {
        QPair<QVariantList, QString> result;
        bool done = false;
        call([&](const QVariantList &items, const QString &error) {
            result = {items, error};
            done = true;
        });
        if (!QTest::qWaitFor([&] { return done; }, 5000))
            return {{}, "timeout"};
        return result;
    }

private slots:
    void picksFirstVisibleUserAndReadsResume()
    {
        JellyfinClient client(server.url(), FakeJellyfin::apiKey);
        const auto [resume, error] = wait([&](auto cb) { client.resume(cb); });
        QCOMPARE(error, QString());
        QCOMPARE(field(resume, "mediaId"), (QStringList{"m1", "e1"}));
        const QVariantMap movie = resume[0].toMap(), episode = resume[1].toMap();
        QCOMPARE(movie["title"].toString(), "Big Buck Bunny");
        QCOMPARE(movie["subtitle"].toString(), "2008");
        QCOMPARE(movie["minutes"].toInt(), 10);
        QCOMPARE(movie["progress"].toDouble(), 0.5);
        QCOMPARE(movie["resumeSeconds"].toInt(), 300);
        QCOMPARE(episode["title"].toString(), "Northern Lights");
        QCOMPARE(episode["subtitle"].toString(), "S2:E4 The Signal");
        QVERIFY(episode["backdrop"].toString().endsWith("/Items/s1/Images/Backdrop?maxWidth=1920&quality=90"));
        QVERIFY(episode["image"].toString().startsWith(server.url() + "/Items/e1/Images/Primary"));
    }

    void userByNameAndUnknownUser()
    {
        JellyfinClient byName(server.url(), FakeJellyfin::apiKey, "martin");
        QCOMPARE(wait([&](auto cb) { byName.resume(cb); }).second, QString());
        JellyfinClient unknown(server.url(), FakeJellyfin::apiKey, "nobody");
        QVERIFY(wait([&](auto cb) { unknown.resume(cb); }).second.contains("not found"));
    }

    void wrongKeyAndDeadServerReportErrors()
    {
        JellyfinClient wrong(server.url(), "wrong");
        QVERIFY(!wait([&](auto cb) { wrong.resume(cb); }).second.isEmpty());
        JellyfinClient dead("http://127.0.0.1:9", FakeJellyfin::apiKey);
        dead.setTimeout(1000);
        QVERIFY(!wait([&](auto cb) { dead.resume(cb); }).second.isEmpty());
    }

    void searchAndLatest()
    {
        JellyfinClient client(server.url(), FakeJellyfin::apiKey);
        QCOMPARE(field(wait([&](auto cb) { client.search("sintel", cb); }).first, "title"), QStringList{"Sintel"});
        const QVariantList latest = wait([&](auto cb) { client.latest(cb); }).first;
        QVERIFY(latest[1].toMap()["image"].toString().endsWith("/Items/m2/Images/Thumb?maxWidth=640&quality=90"));
        QCOMPARE(latest[2].toMap()["progress"].toDouble(), 0.0);
    }

    void fetchHomeReturnsBothRows()
    {
        JellyfinClient client(server.url(), FakeJellyfin::apiKey);
        bool done = false;
        client.fetchHome([&](const QVariantList &resume, const QVariantList &latest, const QString &error) {
            QCOMPARE(error, QString());
            QCOMPARE(resume.size(), 2);
            QCOMPARE(latest.size(), 3);
            done = true;
        });
        QTRY_VERIFY(done);
    }

    void slidesNeedABackdropAndAreUnique()
    {
        JellyfinClient client(server.url(), FakeJellyfin::apiKey);
        const QVariantList items = wait([&](auto cb) { client.latest(cb); }).first
                                   + wait([&](auto cb) { client.resume(cb); }).first;
        const QVariantList slides = JellyfinClient::slides(items);
        QCOMPARE(field(slides, "mediaId"), (QStringList{"e1", "m2", "m3", "m1"}));
        QCOMPARE(slides[0].toMap()["appId"].toString(), "jellyfin");
    }

    void fromConfigNeedsUrlAndKey()
    {
        QVERIFY(!JellyfinClient::fromConfig({{"jellyfin", QJsonObject{{"api_key", "k"}}}}));
        QVERIFY(!JellyfinClient::fromConfig({{"jellyfin", QJsonObject{{"url", "http://x"}}}}));
        std::unique_ptr<JellyfinClient> client(JellyfinClient::fromConfig(
            {{"jellyfin", QJsonObject{{"api_key", "k"}, {"user", "a"}}}, {"vars", QJsonObject{{"jellyfin_url", "http://x/"}}}}));
        QVERIFY(client);
        QCOMPARE(client->url(), "http://x");
        QCOMPARE(client->user(), "a");
        QCOMPARE(client->webUrl("abc"), "http://x/web/#/details?id=abc");
        QCOMPARE(client->streamUrl("abc"), "http://x/Videos/abc/stream?static=true&api_key=k");
    }
};

QTEST_MAIN(TestJellyfin)
#include "tst_jellyfin.moc"
