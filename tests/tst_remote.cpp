#include "remoteserver.h"
#include "testutil.h"

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTcpSocket>
#include <QTest>

class FakeController : public RemoteController
{
public:
    QStringList calls;
    QVariantList remoteApps() override { return {QVariantMap{{"id", "kodi"}, {"name", "Kodi"}}}; }
    QVariantMap remoteStatus() override { return {{"running", ""}}; }
    bool remoteLaunch(const QString &id) override { calls << "launch " + id; return id == "kodi"; }
    bool remoteHome() override { calls << "home"; return true; }
    bool remoteKey(const QString &key) override { calls << "key " + key; return true; }
};

struct Response
{
    int status = 0;
    QByteArray body;
    QJsonObject json() const { return QJsonDocument::fromJson(body).object(); }
};

class TestRemote : public QObject
{
    Q_OBJECT

    FakeController controller;
    RemoteServer *server = nullptr;
    QNetworkAccessManager network;

    Response request(const QString &path, const QByteArray &body = QByteArray(), bool post = false,
                     const QByteArray &token = QByteArray())
    {
        QNetworkRequest req(QUrl(QStringLiteral("http://127.0.0.1:%1%2").arg(server->port()).arg(path)));
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        if (!token.isEmpty())
            req.setRawHeader("X-BSDTV-Token", token);
        QNetworkReply *reply = post || !body.isEmpty() ? network.post(req, body) : network.get(req);
        QEventLoop loop;
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();
        reply->deleteLater();
        return {reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), reply->readAll()};
    }

private slots:
    void init()
    {
        controller.calls.clear();
        server = new RemoteServer(&controller, "secret", this);
        QVERIFY(server->listen(QHostAddress::LocalHost, 0));
    }
    void cleanup() { delete server; }

    void servesTheRemotePage()
    {
        const Response r = request("/");
        QCOMPARE(r.status, 200);
        QVERIFY(r.body.contains("BSDTV Remote"));
    }

    void localhostIsTrusted()
    {
        const Response r = request("/api/launch", R"({"app": "kodi"})");
        QCOMPARE(r.status, 200);
        QCOMPARE(r.json()["ok"].toBool(), true);
        QCOMPARE(controller.calls, QStringList{"launch kodi"});
    }

    void failedActionReturns400()
    {
        const Response r = request("/api/launch", R"({"app": "nope"})");
        QCOMPARE(r.status, 400);
        QCOMPARE(r.json()["ok"].toBool(), false);
    }

    void unknownRouteAndBadJson()
    {
        QCOMPARE(request("/api/nope", "{}").status, 404);
        QCOMPARE(request("/api/key", "{bad").status, 400);
        QCOMPARE(request("/api/key", "[1]").status, 400);
    }

    void oversizedBodyIsRejected()
    {
        QCOMPARE(request("/api/text", "{\"text\": \"" + QByteArray(10000, 'x') + "\"}").status, 413);
    }

    void lanClientsNeedTheToken()
    {
        server->setTrustLocalhost(false);
        QCOMPARE(request("/api/apps").status, 403);
        QCOMPARE(request("/api/apps", {}, false, "wrong").status, 403);
        const Response ok = request("/api/apps", {}, false, "secret");
        QCOMPARE(ok.status, 200);
        QCOMPARE(ok.json()["apps"].toArray()[0].toObject()["id"].toString(), "kodi");
        QCOMPARE(request("/api/home?t=secret", "{}").status, 200);
        QCOMPARE(controller.calls, QStringList{"home"});
    }

    void requestsSplitAcrossPacketsWork()
    {
        QTcpSocket socket;
        socket.connectToHost(QHostAddress::LocalHost, server->port());
        QVERIFY(socket.waitForConnected());
        socket.write("POST /api/key HTTP/1.1\r\nContent-Length: 13\r\n\r\n");
        socket.flush();
        QTest::qWait(50);
        socket.write(R"({"key":"up"})" "\n");
        QTRY_VERIFY(controller.calls.contains("key up"));
    }

    void tokenIsCreatedOnceAndPrivate()
    {
        IsolatedConfig env;
        const QString token = remote::loadToken();
        QVERIFY(token.size() >= 10);
        QCOMPARE(remote::loadToken(), token);
        const auto perms = QFile(env.user() + "/remote-token").permissions();
        QVERIFY(!(perms & (QFileDevice::ReadGroup | QFileDevice::ReadOther)));
    }
};

QTEST_GUILESS_MAIN(TestRemote)
#include "tst_remote.moc"
