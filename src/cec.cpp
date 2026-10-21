#include "cec.h"

#include "config.h"
#include "ctl.h"

#include <QCoreApplication>
#include <QJsonObject>
#include <QMap>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTextStream>

namespace cec {

static const QMap<QString, QPair<QString, QString>> &keys()
{
    static const QMap<QString, QPair<QString, QString>> map{
        {"up", {"key", "up"}},
        {"down", {"key", "down"}},
        {"left", {"key", "left"}},
        {"right", {"key", "right"}},
        {"select", {"key", "ok"}},
        {"exit", {"key", "back"}},
        {"back", {"key", "back"}},
        {"root menu", {"home", ""}},
        {"setup menu", {"key", "menu"}},
        {"contents menu", {"key", "menu"}},
        {"play", {"key", "playpause"}},
        {"pause", {"key", "playpause"}},
        {"stop", {"key", "stop"}},
        {"fast forward", {"key", "forward"}},
        {"rewind", {"key", "rewind"}},
        {"forward", {"key", "next"}},
        {"backward", {"key", "prev"}},
        {"electronic program guide", {"key", "info"}},
        {"display information", {"key", "info"}},
        {"volume up", {"volume", "up"}},
        {"volume down", {"volume", "down"}},
        {"mute", {"volume", "mute"}},
    };
    return map;
}

QPair<QString, QString> parseLine(const QString &line)
{
    // cec-client logs lines such as "key pressed: up (1)" / "key pressed: select (0, 0)".
    static const QRegularExpression keyLine(QStringLiteral("key pressed: ([a-z0-9 ._-]+?) \\("),
                                            QRegularExpression::CaseInsensitiveOption);
    const auto match = keyLine.match(line);
    if (!match.hasMatch())
        return {};
    return keys().value(match.captured(1).trimmed().toLower());
}

static void dispatch(const QPair<QString, QString> &action)
{
    QJsonObject body;
    QString path;
    if (action.first == "home") {
        path = "/api/home";
    } else if (action.first == "key") {
        path = "/api/key";
        body["key"] = action.second;
    } else if (action.first == "volume") {
        path = "/api/volume";
        body["action"] = action.second;
    } else {
        return;
    }
    const QJsonObject reply = ctl::call(path, &body);
    if (reply.contains("error"))
        QTextStream(stderr) << "bsdtv-cec: " << reply.value("error").toString() << "\n";
}

int run(const QStringList &args)
{
    QTextStream err(stderr);
    QString device, name = QStringLiteral("BSDTV");
    bool ifEnabled = false;
    for (qsizetype i = 0; i < args.size(); ++i) {
        if (args.at(i) == "--if-enabled")
            ifEnabled = true;
        else if (args.at(i) == "--name" && i + 1 < args.size())
            name = args.at(++i);
        else if (args.at(i).startsWith('-')) {
            err << "Usage: bsdtv-cec [--if-enabled] [--name NAME] [DEVICE]\n";
            return 2;
        } else
            device = args.at(i);
    }

    const QJsonObject options = config::load().value("cec").toObject();
    // Used by the X session: exit quietly unless cec.enabled is true.
    if (ifEnabled && !options.value("enabled").toBool())
        return 0;
    if (device.isEmpty())
        device = options.value("device").toString();
    if (QStandardPaths::findExecutable("cec-client").isEmpty()) {
        err << "bsdtv-cec: cec-client not found (pkg install libcec)\n";
        return 1;
    }

    // -t p: register as a playback device; -d 24: traffic + debug, where libcec logs key presses.
    QStringList argv{"-t", "p", "-o", name, "-d", "24"};
    if (!device.isEmpty())
        argv << device;
    QProcess process;
    QObject::connect(&process, &QProcess::readyReadStandardOutput, [&process] {
        while (process.canReadLine()) {
            const auto action = parseLine(QString::fromUtf8(process.readLine()));
            if (!action.first.isEmpty())
                dispatch(action);
        }
    });
    QObject::connect(&process, &QProcess::finished, QCoreApplication::instance(),
                     [](int code) { QCoreApplication::exit(code); });
    process.start("cec-client", argv);
    return QCoreApplication::exec();
}

} // namespace cec
