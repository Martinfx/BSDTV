// bsdtv: the TV home screen. "bsdtv ctl ..." and "bsdtv cec ..." are the
// bsdtv-ctl and bsdtv-cec tools; "bsdtv config-set" is used by install.sh.

#include "backend.h"
#include "cec.h"
#include "config.h"
#include "ctl.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QLocale>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QSocketNotifier>
#include <QTextStream>
#include <QTimer>
#include <QTranslator>
#include <QtQml/qqmlextensionplugin.h>

#include <csignal>
#include <functional>
#include <sys/socket.h>
#include <unistd.h>

Q_IMPORT_QML_PLUGIN(BSDTVPlugin)

static int signalPipe[2];

// SIGINT/SIGTERM from the session script stop the Qt loop cleanly (self-pipe trick).
static void watchSignals()
{
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, signalPipe) != 0)
        return;
    auto *notifier = new QSocketNotifier(signalPipe[1], QSocketNotifier::Read, qApp);
    QObject::connect(notifier, &QSocketNotifier::activated, qApp, [] {
        char byte;
        if (::read(signalPipe[1], &byte, 1) > 0)
            QCoreApplication::quit();
    });
    auto handler = [](int) {
        const char byte = 1;
        [[maybe_unused]] auto written = ::write(signalPipe[0], &byte, 1);
    };
    std::signal(SIGINT, handler);
    std::signal(SIGTERM, handler);
}

// bsdtv config-set FILE KEY VALUE: set a (dotted) key in a JSON config file.
static int configSet(const QStringList &args)
{
    if (args.size() != 3) {
        QTextStream(stderr) << "Usage: bsdtv config-set FILE KEY VALUE (KEY may be dotted, e.g. vars.drm_browser)\n";
        return 2;
    }
    QJsonObject root = config::readJson(args.at(0));
    const QStringList path = args.at(1).split('.');
    std::function<QJsonObject(QJsonObject, int)> set = [&](QJsonObject node, int depth) {
        if (depth == path.size() - 1)
            node[path.at(depth)] = args.at(2);
        else
            node[path.at(depth)] = set(node.value(path.at(depth)).toObject(), depth + 1);
        return node;
    };
    return config::writeJson(args.at(0), set(root, 0)) ? 0 : 1;
}

static int runLauncher(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName("BSDTV");
    app.setOrganizationName("BSDTV");

    QCommandLineParser parser;
    parser.setApplicationDescription("BSDTV home screen");
    parser.addHelpOption();
    parser.addOption({"windowed", "Run in a 1280x720 window (development)."});
    parser.addOption({"no-remote", "Do not start the phone remote server."});
    parser.addOption({"screenshot", "Render the home screen to PNG and exit.", "png"});
    parser.process(app);

    Backend backend(nullptr);
    app.installEventFilter(new ActivityFilter(&backend));
    const QString screenshot = parser.value("screenshot");
    if (!parser.isSet("no-remote") && screenshot.isEmpty())
        backend.startRemote();

    QQmlApplicationEngine engine;
    QTranslator translator;
    auto applyLanguage = [&](const QString &lang) {
        QCoreApplication::removeTranslator(&translator);
        if (lang != "en" && translator.load(":/i18n/bsdtv_" + lang + ".qm"))
            QCoreApplication::installTranslator(&translator);
        // Dates and times follow the UI language.
        QLocale::setDefault(QLocale(lang));
        engine.retranslate();
    };
    applyLanguage(backend.language());
    QObject::connect(&backend, &Backend::languageChanged, &app, applyLanguage);

    const bool windowed = parser.isSet("windowed") || !screenshot.isEmpty()
        || !backend.settings().value("fullscreen", true).toBool();
    engine.setInitialProperties({{"windowed", windowed}});
    engine.loadFromModule("BSDTV", "Main");
    if (engine.rootObjects().isEmpty())
        return 1;
    auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst());

    QObject::connect(&backend, &Backend::raiseRequested, window, [window] {
        window->raise();
        window->requestActivate();
    });
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &backend, &Backend::shutdown);
    watchSignals();

    if (!screenshot.isEmpty()) {
        QTimer::singleShot(1500, window, [window, screenshot] {
            window->grabWindow().save(screenshot);
            QCoreApplication::quit();
        });
    }
    return app.exec();
}

int main(int argc, char *argv[])
{
    const QString command = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QString();
    if (command == "ctl" || command == "cec" || command == "config-set") {
        QCoreApplication app(argc, argv);
        const QStringList args = app.arguments().mid(2);
        if (command == "ctl")
            return ctl::run(args);
        if (command == "cec")
            return cec::run(args);
        return configSet(args);
    }
    return runLauncher(argc, argv);
}
