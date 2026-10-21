#include "uitest.h"

class TestUi : public QObject
{
    Q_OBJECT

    std::unique_ptr<UiFixture> ui;

private slots:
    void initTestCase()
    {
        ui = std::make_unique<UiFixture>(QStringList{"kodi", "firefox", "moonlight"});
        QVERIFY(ui->load());
    }
    void cleanupTestCase() { ui.reset(); }

    void tvHomeLoadsWithoutWarnings()
    {
        QCOMPARE(ui->backend->settings()["layout"].toString(), "tv");
        QVERIFY(ui->focused().contains("HeroBanner"));
        QVERIFY2(ui->messages.isEmpty(), qPrintable(ui->messages.join('\n')));
    }

    void bannerRowsAndLaunch()
    {
        ui->press({Qt::Key_Return});
        QCOMPARE(ui->runner->started, QStringList{"kodi"}); // first featured slide

        ui->press({Qt::Key_Down});
        QCOMPARE(ui->focused(), "YouTube"); // "Streaming" cards
        ui->press({Qt::Key_Down});
        QCOMPARE(ui->focused(), "Kodi"); // "Your apps" round icons, favourites first
        ui->press({Qt::Key_Right});
        QCOMPARE(ui->focused(), "YouTube");
        ui->press({Qt::Key_Down, Qt::Key_Down});
        QCOMPARE(ui->focused(), "Moonlight"); // "Movies & music", then "Games"
        ui->press({Qt::Key_Return});
        QCOMPARE(ui->runner->started.last(), "moonlight");
    }

    void backGoesToTabsAndAppsTab()
    {
        ui->press({Qt::Key_Escape});
        QCOMPARE(ui->focused(), "For you");
        ui->press({Qt::Key_Right, Qt::Key_Down});
        QCOMPARE(ui->focused(), "Kodi"); // Apps tab grid
        ui->press({Qt::Key_Right, Qt::Key_Return});
        QCOMPARE(ui->runner->started.last(), "youtube");
    }

    void searchFindsAppsByTyping()
    {
        ui->press({Qt::Key_Escape});
        QCOMPARE(ui->focused(), "Apps");
        ui->press({Qt::Key_Right});
        QCOMPARE(ui->focused(), "Search");
        ui->press({Qt::Key_Return});
        QCOMPARE(ui->focused(), "a"); // on-screen keyboard
        ui->press({Qt::Key_K, Qt::Key_O, Qt::Key_D});
        QTRY_COMPARE(ui->backend->searchResults()["term"].toString(), "kod");
        const QVariantList found = ui->backend->searchResults()["apps"].toList();
        QCOMPARE(found.size(), 1);
        QCOMPARE(found[0].toMap()["appId"].toString(), "kodi");
        ui->press({Qt::Key_Right});
        QCOMPARE(ui->focused(), "b");
        ui->press({Qt::Key_Escape});
        QCOMPARE(ui->focused(), "Search");
    }

    void profilePickerShowsTheDefaultProfile()
    {
        const QString name = ui->backend->profile()["name"].toString();
        ui->press({Qt::Key_Left, Qt::Key_Left, Qt::Key_Left});
        QCOMPARE(ui->focused(), name);
        ui->press({Qt::Key_Return});
        QCOMPARE(ui->focused(), name); // the entry in "Who's watching?"
        ui->press({Qt::Key_Escape});
    }

    void ambientModeComesAndGoesWithoutActing()
    {
        const qsizetype before = ui->runner->started.size();
        ui->backend->startAmbient();
        QTest::qWait(50);
        QVERIFY(ui->backend->ambientActive());
        QVERIFY(ui->focused().contains("Ambient"));
        ui->press({Qt::Key_Return}); // wakes the TV only
        QVERIFY(!ui->backend->ambientActive());
        QCOMPARE(ui->runner->started.size(), before);
        QCOMPARE(ui->focused(), ui->backend->profile()["name"].toString());
    }

    void settingsSwitchToClassicLayout()
    {
        ui->press({Qt::Key_Right, Qt::Key_Right, Qt::Key_Right, Qt::Key_Right, Qt::Key_Return});
        QCOMPARE(ui->focused(), "Theme");
        ui->press({Qt::Key_Down});
        QCOMPARE(ui->focused(), "Home screen");
        ui->press({Qt::Key_Right});
        QCOMPARE(ui->backend->settings()["layout"].toString(), "classic");
        ui->press({Qt::Key_Escape});
        QVERIFY(QString(ui->home()->metaObject()->className()).startsWith("HomeScreen"));
    }

    void classicLayoutNavigation()
    {
        QCOMPARE(ui->focused(), "Kodi"); // the classic home starts on the dock
        ui->press({Qt::Key_Right});
        QCOMPARE(ui->focused(), "YouTube");
        QCOMPARE(ui->home()->property("focusedName").toString(), "YouTube");
        ui->press({Qt::Key_Return});
        QCOMPARE(ui->runner->started.last(), "youtube");
    }
};

QTEST_MAIN(TestUi)
#include "tst_ui.moc"
