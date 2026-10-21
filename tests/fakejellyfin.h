// A tiny stand-in for a Jellyfin server, for tests.
#pragma once

#include <QBuffer>
#include <QColor>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUrl>
#include <QUrlQuery>

class FakeJellyfin : public QObject
{
public:
    static constexpr const char *apiKey = "test-key";
    static constexpr qint64 hour = 36'000'000'000;

    FakeJellyfin()
    {
        m_server.listen(QHostAddress::LocalHost, 0);
        connect(&m_server, &QTcpServer::newConnection, this, [this] {
            while (QTcpSocket *socket = m_server.nextPendingConnection()) {
                connect(socket, &QTcpSocket::readyRead, this, [this, socket] { serve(socket); });
                connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            }
        });
    }

    QString url() const { return QStringLiteral("http://127.0.0.1:%1").arg(m_server.serverPort()); }

    static QJsonArray items()
    {
        return {
            QJsonObject{{"Id", "m1"}, {"Type", "Movie"}, {"Name", "Big Buck Bunny"}, {"ProductionYear", 2008},
                        {"OfficialRating", "PG"}, {"Overview", "A giant rabbit takes revenge."},
                        {"RunTimeTicks", hour / 6}, {"BackdropImageTags", QJsonArray{"b"}},
                        {"ImageTags", QJsonObject{{"Primary", "p"}}},
                        {"UserData", QJsonObject{{"PlaybackPositionTicks", hour / 12}}}},
            QJsonObject{{"Id", "e1"}, {"Type", "Episode"}, {"Name", "The Signal"}, {"SeriesName", "Northern Lights"},
                        {"ParentIndexNumber", 2}, {"IndexNumber", 4}, {"Overview", "A strange radio signal."},
                        {"RunTimeTicks", hour * 3 / 4}, {"ImageTags", QJsonObject{{"Primary", "p"}}},
                        {"ParentBackdropItemId", "s1"},
                        {"UserData", QJsonObject{{"PlaybackPositionTicks", hour / 2}}}},
            QJsonObject{{"Id", "m2"}, {"Type", "Movie"}, {"Name", "Sintel"}, {"ProductionYear", 2010},
                        {"Overview", "A girl searches for her dragon."}, {"RunTimeTicks", hour / 4},
                        {"BackdropImageTags", QJsonArray{"b"}},
                        {"ImageTags", QJsonObject{{"Primary", "p"}, {"Thumb", "t"}}}, {"UserData", QJsonObject{}}},
            QJsonObject{{"Id", "m3"}, {"Type", "Movie"}, {"Name", "Tears of Steel"}, {"ProductionYear", 2012},
                        {"RunTimeTicks", hour / 5}, {"BackdropImageTags", QJsonArray{"b"}},
                        {"ImageTags", QJsonObject{}}, {"UserData", QJsonObject{}}},
        };
    }

private:
    void serve(QTcpSocket *socket)
    {
        const QByteArray request = socket->readAll();
        const QList<QByteArray> lines = request.split('\n');
        const QUrl url = QUrl::fromEncoded(lines.value(0).split(' ').value(1));
        const QUrlQuery query(url);
        const QString path = url.path();
        // Qt sends header names in lower case; servers must not care.
        const bool authorized = request.toLower().contains(QByteArray("x-emby-token: ") + apiKey);

        if (path.startsWith("/Items/") && path.contains("/Images/")) {
            QImage image(64, 36, QImage::Format_RGB32);
            image.fill(QColor("#4f8a3a"));
            QBuffer buffer;
            buffer.open(QIODevice::WriteOnly);
            image.save(&buffer, "PNG");
            return reply(socket, 200, buffer.data(), "image/png");
        }
        if (!authorized)
            return reply(socket, 401, R"({"error": "unauthorized"})");
        const QJsonArray all = items();
        if (path == "/Users")
            return replyJson(socket, QJsonArray{
                QJsonObject{{"Name", "admin"}, {"Id", "u0"}, {"Policy", QJsonObject{{"IsHidden", true}}}},
                QJsonObject{{"Name", "Martin"}, {"Id", "u1"}, {"Policy", QJsonObject{}}}});
        if (path == "/Users/u1/Items/Resume")
            return replyJson(socket, QJsonObject{{"Items", QJsonArray{all[0], all[1]}}});
        if (path == "/Users/u1/Items/Latest")
            return replyJson(socket, QJsonArray{all[1], all[2], all[3]});
        if (path == "/Users/u1/Items") {
            const QString term = query.queryItemValue("SearchTerm").toLower();
            QJsonArray found;
            for (const QJsonValue &item : all) {
                const QJsonObject o = item.toObject();
                if ((o["SeriesName"].toString() + " " + o["Name"].toString()).toLower().contains(term))
                    found.append(o);
            }
            return replyJson(socket, QJsonObject{{"Items", found}});
        }
        reply(socket, 404, R"({"error": "not found"})");
    }

    static void replyJson(QTcpSocket *socket, const QJsonValue &data)
    {
        const QJsonDocument doc = data.isArray() ? QJsonDocument(data.toArray()) : QJsonDocument(data.toObject());
        reply(socket, 200, doc.toJson(QJsonDocument::Compact));
    }

    static void reply(QTcpSocket *socket, int status, const QByteArray &body,
                      const QByteArray &type = "application/json")
    {
        socket->write("HTTP/1.1 " + QByteArray::number(status) + " X\r\nContent-Type: " + type
                      + "\r\nContent-Length: " + QByteArray::number(body.size())
                      + "\r\nConnection: close\r\n\r\n" + body);
        socket->disconnectFromHost();
    }

    QTcpServer m_server;
};
