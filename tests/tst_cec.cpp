#include "cec.h"
#include "systemtools.h"

#include <QTest>

using Action = QPair<QString, QString>;

class TestCec : public QObject
{
    Q_OBJECT

private slots:
    void parseLine_data()
    {
        QTest::addColumn<QString>("line");
        QTest::addColumn<QString>("action");
        QTest::addColumn<QString>("argument");
        QTest::newRow("up") << "DEBUG:   [   12345]\tkey pressed: up (1) current(ff) duration(0)" << "key" << "up";
        QTest::newRow("select") << "DEBUG:   [   12345]\tkey pressed: select (0, 0)" << "key" << "ok";
        QTest::newRow("root menu") << "key pressed: root menu (9)" << "home" << "";
        QTest::newRow("volume") << "key pressed: volume up (41)" << "volume" << "up";
        QTest::newRow("case") << "key pressed: Play (44)" << "key" << "playpause";
        QTest::newRow("release") << "key released: up (1) D:120ms" << "" << "";
        QTest::newRow("unknown") << "key pressed: unknown thing (99)" << "" << "";
        QTest::newRow("traffic") << "TRAFFIC: [ 1] >> 01:44:01" << "" << "";
    }

    void parseLine()
    {
        QFETCH(QString, line);
        QFETCH(QString, action);
        QFETCH(QString, argument);
        const Action parsed = cec::parseLine(line);
        QCOMPARE(parsed.first, action);
        QCOMPARE(parsed.second, argument);
    }

    void everyMappedKeyIsAKnownRemoteKey()
    {
        for (const QString &line : {"up", "down", "left", "right", "select", "exit", "back", "setup menu",
                                    "play", "pause", "stop", "fast forward", "rewind", "forward", "backward",
                                    "electronic program guide", "display information"}) {
            const Action parsed = cec::parseLine("key pressed: " + line + " (0)");
            QCOMPARE(parsed.first, QStringLiteral("key"));
            QVERIFY2(systemtools::keysyms().contains(parsed.second), qPrintable(line));
        }
    }
};

QTEST_GUILESS_MAIN(TestCec)
#include "tst_cec.moc"
