#include "ctl.h"

#include "config.h"
#include "remoteserver.h"
#include "systemtools.h"

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTextStream>

namespace ctl {

static int configuredPort()
{
    return config::load().value("remote").toObject().value("port").toInt(8765);
}

QJsonObject call(const QString &path, const QJsonObject *body, int port)
{
    QNetworkAccessManager network;
    QNetworkRequest request(QUrl(QStringLiteral("http://127.0.0.1:%1%2").arg(port ? port : configuredPort()).arg(path)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setTransferTimeout(5000);
    QNetworkReply *reply = body ? network.post(request, QJsonDocument(*body).toJson(QJsonDocument::Compact))
                                : network.get(request);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    reply->deleteLater();
    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (doc.isObject())
        return doc.object();
    return {{"error", "launcher is not reachable: " + reply->errorString()}};
}

static const char usage[] =
    "Usage: bsdtv-ctl COMMAND\n"
    "\n"
    "  home                 close the running app and show the home screen\n"
    "  apps                 list installed apps\n"
    "  status               show what is running\n"
    "  reload               re-read config, apps and theme\n"
    "  url                  print the phone remote URL\n"
    "  launch APP           start an app\n"
    "  key KEY...           send remote keys (up down left right ok back menu playpause ...)\n"
    "  text TEXT            type text into the focused app\n"
    "  volume up|down|mute\n"
    "  power reboot|poweroff|display_off\n"
    "  theme THEME          switch theme\n";

int run(const QStringList &args)
{
    QTextStream out(stdout), err(stderr);
    const QString cmd = args.value(0);
    const QStringList rest = args.mid(1);
    auto fail = [&err](const QString &text) {
        err << "bsdtv-ctl: " << text << "\n";
        return 2;
    };

    QJsonObject result;
    if (cmd == "url") {
        const QString ip = systemtools::ipAddress();
        out << QStringLiteral("http://%1:%2/?t=%3\n").arg(ip.isEmpty() ? "TV-IP" : ip).arg(configuredPort())
                   .arg(remote::loadToken());
        return 0;
    } else if (cmd == "apps" || cmd == "status") {
        result = call("/api/" + cmd);
    } else if (cmd == "home" || cmd == "reload") {
        const QJsonObject body;
        result = call("/api/" + cmd, &body);
    } else if (cmd == "launch" && rest.size() == 1) {
        const QJsonObject body{{"app", rest.first()}};
        result = call("/api/launch", &body);
    } else if (cmd == "key" && !rest.isEmpty()) {
        bool ok = true;
        for (const QString &key : rest) {
            if (!systemtools::keysyms().contains(key))
                return fail("unknown key " + key);
            const QJsonObject body{{"key", key}};
            const QJsonObject reply = call("/api/key", &body);
            ok = ok && reply.value("ok").toBool();
            if (reply.contains("error"))
                result = reply;
        }
        if (!result.contains("error"))
            result = {{"ok", ok}};
    } else if (cmd == "text" && rest.size() == 1) {
        const QJsonObject body{{"text", rest.first()}};
        result = call("/api/text", &body);
    } else if ((cmd == "volume" || cmd == "power") && rest.size() == 1) {
        const QJsonObject body{{"action", rest.first()}};
        result = call("/api/" + cmd, &body);
    } else if (cmd == "theme" && rest.size() == 1) {
        if (!config::listThemes().contains(rest.first()))
            return fail("unknown theme " + rest.first());
        config::saveUserSetting("theme", rest.first());
        const QJsonObject body;
        result = call("/api/reload", &body);
    } else {
        err << usage;
        return 2;
    }

    if (result.contains("error"))
        return fail(result.value("error").toString());
    if (cmd == "apps") {
        for (const QJsonValue &app : result.value("apps").toArray())
            out << app.toObject().value("id").toString().leftJustified(12) << " "
                << app.toObject().value("name").toString() << "\n";
    } else if (cmd == "status") {
        out << QJsonDocument(result).toJson(QJsonDocument::Indented);
    }
    return result.value("ok").toBool(true) ? 0 : 1;
}

} // namespace ctl
