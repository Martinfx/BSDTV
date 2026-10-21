// The QML-facing backend (the TV singleton): apps, settings, theme, profiles,
// Jellyfin rows, search, ambient mode and the phone remote.
#pragma once

#include "appmodel.h"
#include "apps.h"
#include "remoteserver.h"

#include <QJsonObject>
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

class AppRunner;
class JellyfinClient;
class QJSEngine;
class QQmlEngine;

QString resolveLanguage(const QString &setting);
// Shuffled image URLs from a folder tree, for the ambient screen.
QStringList findPhotos(const QString &folder);

class Backend : public QObject, public RemoteController
{
    Q_OBJECT
    QML_NAMED_ELEMENT(TV)
    QML_SINGLETON

    Q_PROPERTY(QObject *appModel READ appModel CONSTANT)
    Q_PROPERTY(QObject *dockModel READ dockModel CONSTANT)
    Q_PROPERTY(QObject *allAppsModel READ allAppsModel CONSTANT)
    Q_PROPERTY(QVariantMap settings READ settings NOTIFY settingsChanged)
    Q_PROPERTY(QVariantMap theme READ theme NOTIFY themeChanged)
    Q_PROPERTY(QVariantList themes READ themes NOTIFY themeChanged)
    Q_PROPERTY(QString runningApp READ runningApp NOTIFY runningAppChanged)
    Q_PROPERTY(QString ipAddress READ ipAddress NOTIFY infoChanged)
    Q_PROPERTY(QString stats READ stats NOTIFY infoChanged)
    Q_PROPERTY(QString remoteUrl READ remoteUrl NOTIFY infoChanged)
    Q_PROPERTY(QVariantList rows READ rows NOTIFY rowsChanged)
    Q_PROPERTY(QVariantList featured READ featured NOTIFY rowsChanged)
    Q_PROPERTY(QVariantList profiles READ profiles NOTIFY settingsChanged)
    Q_PROPERTY(QVariantMap profile READ profile NOTIFY settingsChanged)
    Q_PROPERTY(QVariantMap searchResults READ searchResults NOTIFY searchChanged)
    Q_PROPERTY(bool ambientActive READ ambientActive NOTIFY ambientChanged)
    Q_PROPERTY(QStringList ambientPhotos READ ambientPhotos NOTIFY ambientChanged)
    Q_PROPERTY(bool hasMpv READ hasMpv NOTIFY settingsChanged)
    Q_PROPERTY(QString language READ language NOTIFY settingsChanged)
    Q_PROPERTY(QString version READ version CONSTANT)

public:
    // The runner can be replaced in tests (nullptr: a real one); the backend takes
    // ownership. Deliberately not default-constructible: QML would then build its
    // own instance instead of asking create() for this one.
    explicit Backend(AppRunner *runner, QObject *parent = nullptr);
    ~Backend() override;

    static Backend *create(QQmlEngine *, QJSEngine *);

    void startRemote();
    void shutdown();
    AppRunner *runner() const { return m_runner; }
    QList<App> appList() const { return m_apps; }

    QObject *appModel() const { return m_appModel; }
    QObject *dockModel() const { return m_dockModel; }
    QObject *allAppsModel() const { return m_allModel; }
    QVariantMap settings() const { return m_config.toVariantMap(); }
    QVariantMap theme() const { return m_theme.toVariantMap(); }
    QVariantList themes() const;
    QString runningApp() const;
    QString ipAddress() const { return m_ip; }
    QString stats() const { return m_stats; }
    QString remoteUrl() const;
    QVariantList rows() const { return m_rows; }
    QVariantList featured() const { return m_featured; }
    QVariantList profiles() const;
    QVariantMap profile() const;
    QVariantMap searchResults() const { return m_search; }
    bool ambientActive() const { return m_ambient; }
    QStringList ambientPhotos() const;
    bool hasMpv() const;
    QString language() const { return m_lang; }
    QString version() const { return QStringLiteral(BSDTV_VERSION); }

    // RemoteController
    QVariantList remoteApps() override;
    QVariantMap remoteStatus() override;
    bool remoteLaunch(const QString &appId) override;
    bool remoteHome() override;
    bool remoteKey(const QString &name) override;
    bool remoteText(const QString &text) override;
    bool remoteVolume(const QString &action) override;
    bool remotePower(const QString &action) override;
    bool remoteReload() override;

public slots:
    void reload();
    void launch(const QString &appId);
    void home();
    void power(const QString &action);
    void volume(const QString &action);
    void setSetting(const QString &key, const QVariant &value);
    void refreshMedia();
    void openMedia(const QString &mediaId);
    void watchInJellyfin(const QString &mediaId);
    void playMedia(const QString &mediaId);
    void search(const QString &term);
    void setProfile(const QString &profileId);
    // User activity: leave the ambient screen and restart the idle countdown.
    void touch();
    void startAmbient();

signals:
    void settingsChanged();
    void themeChanged();
    void runningAppChanged();
    void infoChanged();
    void raiseRequested();
    void languageChanged(const QString &language);
    void message(const QString &text);
    void rowsChanged();
    void mediaRequested(const QVariantMap &item);
    void searchChanged();
    void ambientChanged();

private:
    void refreshInfo();
    void updateRows();
    void onRunningChanged(const QString &appId);
    void onIdle();
    QVariantMap findMedia(const QString &mediaId) const;

    AppRunner *m_runner;
    AppModel *m_appModel;
    AppModel *m_dockModel;
    AppModel *m_allModel;
    QList<App> m_apps;
    QVariantList m_appRows;
    QVariantList m_appFeatured;
    QVariantList m_rows;
    QVariantList m_featured;
    QVariantList m_resume;
    QVariantList m_latest;
    QVariantMap m_search;
    QPointer<JellyfinClient> m_jellyfin;
    int m_generation = 0;
    int m_searchGeneration = 0;
    bool m_ambient = false;
    QStringList m_photos;
    QJsonObject m_config;
    QJsonObject m_theme;
    QString m_lang;
    QString m_ip;
    QString m_stats;
    QString m_token;
    RemoteServer *m_server = nullptr;
    QTimer m_idle;
    QTimer m_mediaTimer;
    QTimer m_infoTimer;
};

// Reports remote, keyboard and mouse activity to the backend. The key that
// wakes the TV from ambient mode is swallowed so it does nothing else.
class ActivityFilter : public QObject
{
    Q_OBJECT

public:
    explicit ActivityFilter(Backend *backend) : QObject(backend), m_backend(backend) {}

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    Backend *m_backend;
};

