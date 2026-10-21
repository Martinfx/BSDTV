// The Jellyfin parts of the UI against a fake server.
#include "fakejellyfin.h"
#include "uitest.h"

class TestMedia : public QObject
{
    Q_OBJECT

    std::unique_ptr<FakeJellyfin> server;
    std::unique_ptr<UiFixture> ui;

private slots:
    void initTestCase()
    {
        server = std::make_unique<FakeJellyfin>();
        ui = std::make_unique<UiFixture>(
            QStringList{"kodi", "firefox", "mpv"},
            QJsonObject{{"jellyfin", QJsonObject{{"url", server->url()}, {"api_key", FakeJellyfin::apiKey}}},
                        {"vars", QJsonObject{{"jellyfin_url", server->url()}}},
                        {"ambient", QJsonObject{{"photos", ""}}}});
        QVERIFY(ui->load());
        QTRY_VERIFY(!ui->backend->rows().isEmpty()
                    && ui->backend->rows().first().toMap()["key"].toString() == "continue");
    }
    void cleanupTestCase()
    {
        ui.reset();
        server.reset();
    }

    void bannerShowsFilmsFromJellyfin()
    {
        const QVariantMap slide = ui->backend->featured().first().toMap();
        QCOMPARE(slide["mediaId"].toString(), "e1");
        QCOMPARE(slide["title"].toString(), "Northern Lights");
    }

    void continueWatchingOpensDetails()
    {
        ui->press({Qt::Key_Down});
        QCOMPARE(ui->focused(), "Big Buck Bunny");
        ui->press({Qt::Key_Right});
        QCOMPARE(ui->focused(), "Northern Lights");
        ui->press({Qt::Key_Return});
        QTRY_COMPARE(ui->focused(), "Resume in Jellyfin");
    }

    void watchOnJellyfinOpensTheWebApp()
    {
        ui->press({Qt::Key_Return});
        QCOMPARE(ui->runner->started.last(), "jellyfin");
        QCOMPARE(ui->runner->commands.last().last(), server->url() + "/web/#/details?id=e1");
    }

    void playNowStreamsInMpvFromTheLastPosition()
    {
        ui->backend->openMedia("m1");
        QTRY_COMPARE(ui->focused(), "Resume in Jellyfin");
        ui->press({Qt::Key_Right, Qt::Key_Return});
        const QStringList argv = ui->runner->commands.last();
        QCOMPARE(argv.first(), "mpv");
        QVERIFY(argv.contains("--start=300"));
        QVERIFY(argv.last().startsWith(server->url() + "/Videos/m1/stream"));
    }

    void ambientUsesBackdropsWithoutPhotos()
    {
        QVERIFY(!ui->backend->ambientPhotos().isEmpty());
        QVERIFY(ui->backend->ambientPhotos().first().contains("/Images/Backdrop"));
    }

    void searchFindsFilms()
    {
        ui->backend->search("sintel");
        QTRY_COMPARE(ui->backend->searchResults()["busy"].toBool(), false);
        const QVariantList media = ui->backend->searchResults()["media"].toList();
        QCOMPARE(media.size(), 1);
        QCOMPARE(media[0].toMap()["title"].toString(), "Sintel");
    }
};

QTEST_MAIN(TestMedia)
#include "tst_media.moc"
