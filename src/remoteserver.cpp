#include "remoteserver.h"

#include "config.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUrl>
#include <QUrlQuery>

namespace remote {

QString loadToken()
{
    const QString path = config::userDir() + "/remote-token";
    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        const QString token = QString::fromUtf8(file.readAll()).trimmed();
        if (!token.isEmpty())
            return token;
        file.close();
    }
    static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    QString token;
    for (int i = 0; i < 12; ++i)
        token += QLatin1Char(alphabet[QRandomGenerator::system()->bounded(int(sizeof(alphabet) - 1))]);
    QDir().mkpath(config::userDir());
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        file.write(token.toUtf8() + "\n");
    }
    return token;
}

} // namespace remote

static bool constantTimeEquals(const QByteArray &a, const QByteArray &b)
{
    unsigned char diff = a.size() == b.size() ? 0 : 1;
    for (qsizetype i = 0; i < a.size(); ++i)
        diff |= static_cast<unsigned char>(a.at(i) ^ (i < b.size() ? b.at(i) : 0));
    return diff == 0;
}

RemoteServer::RemoteServer(RemoteController *controller, const QString &token, QObject *parent)
    : QObject(parent)
    , m_server(new QTcpServer(this))
    , m_controller(controller)
    , m_token(token)
{
    connect(m_server, &QTcpServer::newConnection, this, [this] {
        while (QTcpSocket *socket = m_server->nextPendingConnection()) {
            connect(socket, &QTcpSocket::readyRead, this, [this, socket] { onReadyRead(socket); });
            connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
        }
    });
}

bool RemoteServer::listen(const QHostAddress &address, quint16 port)
{
    return m_server->listen(address, port);
}

quint16 RemoteServer::port() const
{
    return m_server->serverPort();
}

void RemoteServer::onReadyRead(QTcpSocket *socket)
{
    // Requests are small; wait until the headers and body are all there.
    QByteArray buffer = socket->property("buffer").toByteArray() + socket->readAll();
    const qsizetype end = buffer.indexOf("\r\n\r\n");
    if (end < 0) {
        if (buffer.size() > maxHeader)
            reply(socket, 431, R"({"error": "headers too large"})");
        else
            socket->setProperty("buffer", buffer);
        return;
    }

    Request request;
    const QList<QByteArray> lines = buffer.left(end).split('\n');
    const QList<QByteArray> requestLine = lines.value(0).trimmed().split(' ');
    if (requestLine.size() < 2)
        return reply(socket, 400, R"({"error": "bad request"})");
    request.method = requestLine.at(0);
    const QUrl url = QUrl::fromEncoded(requestLine.at(1));
    request.path = url.path();
    for (const auto &item : QUrlQuery(url).queryItems(QUrl::FullyDecoded))
        request.query.insert(item.first, item.second);
    for (const QByteArray &line : lines.mid(1)) {
        const qsizetype colon = line.indexOf(':');
        if (colon > 0)
            request.headers.insert(line.left(colon).trimmed().toLower(), line.mid(colon + 1).trimmed());
    }

    bool ok = true;
    const QByteArray lengthHeader = request.headers.value("content-length", "0");
    const qint64 length = lengthHeader.toLongLong(&ok);
    if (!ok || length < 0 || length > maxBody)
        return reply(socket, 413, R"({"error": "body too large"})");
    request.body = buffer.mid(end + 4);
    if (request.body.size() < length) {
        socket->setProperty("buffer", buffer);
        return;
    }
    request.body.truncate(length);
    socket->setProperty("buffer", QByteArray());
    handle(socket, request);
}

bool RemoteServer::authorized(QTcpSocket *socket, const Request &request) const
{
    if (m_trustLocalhost && socket->peerAddress().isLoopback())
        return true;
    QByteArray supplied = request.headers.value("x-bsdtv-token");
    if (supplied.isEmpty())
        supplied = request.query.value("t").toUtf8();
    return constantTimeEquals(supplied, m_token.toUtf8());
}

void RemoteServer::handle(QTcpSocket *socket, const Request &request)
{
    if (request.method == "GET" && (request.path == "/" || request.path == "/index.html")) {
        // The page itself is harmless; it asks for the token before calling the API.
        QFile page(QStringLiteral(":/qt/qml/BSDTV/web/remote.html"));
        page.open(QIODevice::ReadOnly);
        return reply(socket, 200, page.readAll(), "text/html; charset=utf-8");
    }
    if (!authorized(socket, request))
        return reply(socket, 403, R"({"error": "bad token"})");

    if (request.method == "GET") {
        if (request.path == "/api/apps")
            return replyJson(socket, 200, {{"apps", m_controller->remoteApps()}});
        if (request.path == "/api/status")
            return replyJson(socket, 200, m_controller->remoteStatus());
        return reply(socket, 404, R"({"error": "not found"})");
    }
    if (request.method != "POST")
        return reply(socket, 405, R"({"error": "method not allowed"})");

    QJsonParseError error;
    const QJsonDocument doc = request.body.isEmpty() ? QJsonDocument(QJsonObject())
                                                     : QJsonDocument::fromJson(request.body, &error);
    if (error.error != QJsonParseError::NoError)
        return reply(socket, 400, R"({"error": "invalid json"})");
    if (!doc.isObject())
        return reply(socket, 400, R"({"error": "expected an object"})");
    const QJsonObject body = doc.object();
    auto field = [&body](const char *key) { return body.value(QLatin1String(key)).toVariant().toString(); };

    bool ok;
    if (request.path == "/api/key")
        ok = m_controller->remoteKey(field("key"));
    else if (request.path == "/api/text")
        ok = m_controller->remoteText(field("text").left(500));
    else if (request.path == "/api/launch")
        ok = m_controller->remoteLaunch(field("app"));
    else if (request.path == "/api/home")
        ok = m_controller->remoteHome();
    else if (request.path == "/api/volume")
        ok = m_controller->remoteVolume(field("action"));
    else if (request.path == "/api/power")
        ok = m_controller->remotePower(field("action"));
    else if (request.path == "/api/reload")
        ok = m_controller->remoteReload();
    else
        return reply(socket, 404, R"({"error": "not found"})");
    replyJson(socket, ok ? 200 : 400, {{"ok", ok}});
}

void RemoteServer::reply(QTcpSocket *socket, int status, const QByteArray &body, const QByteArray &contentType)
{
    static const QMap<int, QByteArray> reasons{
        {200, "OK"}, {400, "Bad Request"}, {403, "Forbidden"}, {404, "Not Found"},
        {405, "Method Not Allowed"}, {413, "Payload Too Large"}, {431, "Request Header Fields Too Large"}};
    QByteArray response = "HTTP/1.1 " + QByteArray::number(status) + " " + reasons.value(status, "Error") + "\r\n";
    response += "Content-Type: " + contentType + "\r\n";
    response += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
    response += "Cache-Control: no-store\r\nConnection: close\r\nServer: BSDTV\r\n\r\n";
    socket->write(response + body);
    socket->disconnectFromHost();
}

void RemoteServer::replyJson(QTcpSocket *socket, int status, const QVariantMap &data)
{
    reply(socket, status, QJsonDocument(QJsonObject::fromVariantMap(data)).toJson(QJsonDocument::Compact));
}
