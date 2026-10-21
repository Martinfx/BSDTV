#include "systemtools.h"

#include <QTest>

class TestSystemTools : public QObject
{
    Q_OBJECT

    QList<QStringList> calls;

private slots:
    void init()
    {
        calls.clear();
        systemtools::setRunner([this](const QStringList &argv) { calls << argv; return true; });
    }
    void cleanup() { systemtools::setRunner(nullptr); }

    void unknownActionsDoNothing()
    {
        QVERIFY(!systemtools::sendKey("selfdestruct"));
        QVERIFY(!systemtools::volume("louder"));
        QVERIFY(!systemtools::power("format"));
        QVERIFY(calls.isEmpty());
    }

    void volumeUsesFreeBsdMixerSyntax()
    {
        systemtools::volume("up");
        systemtools::volume("mute");
        QCOMPARE(calls, (QList<QStringList>{{"mixer", "vol.volume=+5%"}, {"mixer", "vol.mute=^"}}));
    }

    void powerGoesThroughDoas()
    {
        systemtools::power("poweroff");
        QCOMPARE(calls, (QList<QStringList>{{"doas", "/sbin/shutdown", "-p", "now"}}));
    }

    void keysAreSentWithXdotool()
    {
        systemtools::sendKey("ok");
        QCOMPARE(calls, (QList<QStringList>{{"xdotool", "key", "--clearmodifiers", "Return"}}));
    }

    void missingToolIsReportedNotRaised()
    {
        systemtools::setRunner(nullptr);
        qputenv("PATH", "/nonexistent");
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression("xdotool not found"));
        QVERIFY(!systemtools::sendKey("up"));
    }
};

QTEST_GUILESS_MAIN(TestSystemTools)
#include "tst_systemtools.moc"
