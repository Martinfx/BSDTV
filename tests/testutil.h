// Shared helpers: every test gets empty system and user config directories.
#pragma once

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>

struct IsolatedConfig
{
    QTemporaryDir dir;

    IsolatedConfig()
    {
        qputenv("BSDTV_ETC", (dir.path() + "/etc").toUtf8());
        qputenv("XDG_CONFIG_HOME", (dir.path() + "/home-config").toUtf8());
    }

    QString etc() const { return dir.path() + "/etc"; }
    QString user() const { return dir.path() + "/home-config/bsdtv"; }

    static void write(const QString &path, const QJsonObject &data)
    {
        QDir().mkpath(QFileInfo(path).absolutePath());
        QFile file(path);
        file.open(QIODevice::WriteOnly);
        file.write(QJsonDocument(data).toJson());
    }
};
