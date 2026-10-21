// Phone remote: a tiny HTTP server serving the web remote and a JSON API.
//
// Requests from localhost are trusted (bsdtv-ctl, openbox key bindings).
// Every other client must send the pairing token shown in the TV settings,
// either as the X-BSDTV-Token header or the "t" query parameter.
#pragma once

#include <QHostAddress>
#include <QObject>
#include <QVariantList>
#include <QVariantMap>

class QTcpServer;
class QTcpSocket;

// What the remote can do. The launcher backend implements this.
class RemoteController
{
public:
    virtual ~RemoteController() = default;
    virtual QVariantList remoteApps() { return {}; }
    virtual QVariantMap remoteStatus() { return {}; }
    virtual bool remoteLaunch(const QString &) { return false; }
    virtual bool remoteHome() { return false; }
    virtual bool remoteKey(const QString &) { return false; }
    virtual bool remoteText(const QString &) { return false; }
    virtual bool remoteVolume(const QString &) { return false; }
    virtual bool remotePower(const QString &) { return false; }
    virtual bool remoteReload() { return false; }
};

namespace remote {
// The pairing token, created on first use.
QString loadToken();
}

class RemoteServer : public QObject
{
    Q_OBJECT

public:
    static constexpr int maxBody = 4096;
    static constexpr int maxHeader = 16384;

    RemoteServer(RemoteController *controller, const QString &token, QObject *parent = nullptr);

    bool listen(const QHostAddress &address, quint16 port);
    quint16 port() const;

    // For tests: treat every client as if it came from another machine.
    void setTrustLocalhost(bool trust) { m_trustLocalhost = trust; }

private:
    struct Request
    {
        QByteArray method;
        QString path;
        QMap<QString, QString> query;
        QMap<QByteArray, QByteArray> headers;
        QByteArray body;
    };

    void onReadyRead(QTcpSocket *socket);
    void handle(QTcpSocket *socket, const Request &request);
    bool authorized(QTcpSocket *socket, const Request &request) const;
    static void reply(QTcpSocket *socket, int status, const QByteArray &body,
                      const QByteArray &contentType = "application/json");
    static void replyJson(QTcpSocket *socket, int status, const QVariantMap &data);

    QTcpServer *m_server;
    RemoteController *m_controller;
    QString m_token;
    bool m_trustLocalhost = true;
};
