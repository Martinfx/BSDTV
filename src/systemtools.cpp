#include "systemtools.h"

#include <QDir>
#include <QNetworkInterface>
#include <QProcess>
#include <QStandardPaths>
#include <QStorageInfo>

namespace systemtools {

const QMap<QString, QString> &keysyms()
{
    static const QMap<QString, QString> map{
        {"up", "Up"},
        {"down", "Down"},
        {"left", "Left"},
        {"right", "Right"},
        {"ok", "Return"},
        {"back", "Escape"},
        {"menu", "Menu"},
        {"playpause", "space"},
        {"stop", "XF86AudioStop"},
        {"next", "XF86AudioNext"},
        {"prev", "XF86AudioPrev"},
        {"rewind", "Left"},
        {"forward", "Right"},
        {"info", "i"},
        {"backspace", "BackSpace"},
    };
    return map;
}

const QMap<QString, QStringList> &powerCommands()
{
    static const QMap<QString, QStringList> map{
        {"reboot", {"doas", "/sbin/shutdown", "-r", "now"}},
        {"poweroff", {"doas", "/sbin/shutdown", "-p", "now"}},
        {"display_off", {"xset", "dpms", "force", "off"}},
    };
    return map;
}

static bool runProcess(const QStringList &argv)
{
    if (QStandardPaths::findExecutable(argv.first()).isEmpty()) {
        qWarning("bsdtv: %s not found, skipping %s", qPrintable(argv.first()), qPrintable(argv.join(' ')));
        return false;
    }
    QProcess process;
    process.setProcessChannelMode(QProcess::ForwardedErrorChannel);
    process.start(argv.first(), argv.mid(1));
    if (!process.waitForFinished(10000)) {
        process.kill();
        qWarning("bsdtv: %s failed or timed out", qPrintable(argv.join(' ')));
        return false;
    }
    return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
}

static Runner &currentRunner()
{
    static Runner runner = runProcess;
    return runner;
}

void setRunner(Runner runner)
{
    currentRunner() = runner ? std::move(runner) : Runner(runProcess);
}

bool run(const QStringList &argv)
{
    return !argv.isEmpty() && currentRunner()(argv);
}

bool sendKey(const QString &name)
{
    const auto it = keysyms().find(name);
    return it != keysyms().end() && run({"xdotool", "key", "--clearmodifiers", *it});
}

bool sendText(const QString &text)
{
    return !text.isEmpty() && run({"xdotool", "type", "--delay", "40", "--", text});
}

bool volume(const QString &action)
{
    static const QMap<QString, QString> args{
        {"up", "vol.volume=+5%"}, {"down", "vol.volume=-5%"}, {"mute", "vol.mute=^"}};
    const auto it = args.find(action);
    return it != args.end() && run({"mixer", *it});
}

bool power(const QString &action)
{
    const auto it = powerCommands().find(action);
    return it != powerCommands().end() && run(*it);
}

QString ipAddress()
{
    for (const QNetworkInterface &iface : QNetworkInterface::allInterfaces()) {
        const auto flags = iface.flags();
        if (!(flags & QNetworkInterface::IsUp) || !(flags & QNetworkInterface::IsRunning)
            || (flags & QNetworkInterface::IsLoopBack))
            continue;
        for (const QNetworkAddressEntry &entry : iface.addressEntries()) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol)
                return entry.ip().toString();
        }
    }
    return {};
}

static QString sysctl(const QString &name)
{
    QProcess process;
    process.start("sysctl", {"-n", name});
    if (!process.waitForFinished(2000) || process.exitCode() != 0)
        return {};
    return QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed();
}

QString stats()
{
    QStringList parts;
    const QString temp = sysctl("dev.cpu.0.temperature");
    if (!temp.isEmpty())
        parts << QStringLiteral("CPU %1°C").arg(temp.section('.', 0, 0).remove('C'));
    bool okPages = false, okSize = false;
    const qint64 pages = sysctl("vm.stats.vm.v_free_count").toLongLong(&okPages);
    const qint64 pageSize = sysctl("hw.pagesize").toLongLong(&okSize);
    if (okPages && okSize)
        parts << QStringLiteral("RAM %1 MB free").arg(pages * pageSize / (1 << 20));
    const QStorageInfo disk(QDir::homePath());
    if (disk.isValid())
        parts << QStringLiteral("Disk %1 GB free").arg(disk.bytesAvailable() / (qint64(1) << 30));
    return parts.join(QStringLiteral("  ·  "));
}

} // namespace systemtools
