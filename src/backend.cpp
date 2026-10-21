#include "backend.h"

#include "apprunner.h"
#include "config.h"
#include "jellyfin.h"
#include "systemtools.h"

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QEvent>
#include <QJSEngine>
#include <QJsonArray>
#include <QLocale>
#include <QRandomGenerator>
#include <QStandardPaths>
#include <QUrl>

#include <algorithm>
#include <random>

static Backend *s_instance = nullptr;
static const QStringList languages{"en", "cs"};

QString resolveLanguage(const QString &setting)
{
    // "auto" follows the system locale; anything without a translation falls back to English.
    const QString lang = setting.isEmpty() || setting == "auto"
        ? QLocale::system().name().section('_', 0, 0) : setting;
    return languages.contains(lang) ? lang : QStringLiteral("en");
}

QStringList findPhotos(const QString &folder)
{
    static constexpr int maxPhotos = 300;
    if (folder.isEmpty())
        return {};
    QString root = folder;
    if (root.startsWith("~"))
        root = QDir::homePath() + root.mid(1);
    QStringList found;
    QDirIterator it(root, {"*.jpg", "*.jpeg", "*.png", "*.webp", "*.JPG", "*.JPEG", "*.PNG", "*.WEBP"},
                    QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext() && found.size() < maxPhotos * 4)
        found << it.next();
    std::shuffle(found.begin(), found.end(), std::mt19937(QRandomGenerator::global()->generate()));
    QStringList urls;
    for (const QString &path : found.mid(0, maxPhotos))
        urls << QUrl::fromLocalFile(path).toString();
    return urls;
}

Backend::Backend(AppRunner *runner, QObject *parent)
    : QObject(parent)
    , m_runner(runner ? runner : new AppRunner)
    , m_appModel(new AppModel(this))
    , m_dockModel(new AppModel(this))
    , m_allModel(new AppModel(this))
{
    s_instance = this;
    m_runner->setParent(this);
    connect(m_runner, &AppRunner::runningChanged, this, &Backend::onRunningChanged);
    connect(m_runner, &AppRunner::failed, this,
            [this](const QString &appId, const QString &error) { emit message(appId + ": " + error); });

    m_search = {{"term", ""}, {"apps", QVariantList()}, {"media", QVariantList()}, {"busy", false}};
    m_token = remote::loadToken();

    m_idle.setSingleShot(true);
    connect(&m_idle, &QTimer::timeout, this, &Backend::onIdle);
    m_mediaTimer.setInterval(5 * 60 * 1000);
    connect(&m_mediaTimer, &QTimer::timeout, this, &Backend::refreshMedia);
    m_mediaTimer.start();
    m_infoTimer.setInterval(30 * 1000);
    connect(&m_infoTimer, &QTimer::timeout, this, &Backend::refreshInfo);
    m_infoTimer.start();

    reload();
    refreshInfo();
}

Backend::~Backend()
{
    if (s_instance == this)
        s_instance = nullptr;
}

Backend *Backend::create(QQmlEngine *, QJSEngine *)
{
    Q_ASSERT_X(s_instance, "Backend::create", "create the Backend before loading QML");
    QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
    return s_instance;
}

// --- lifecycle ---------------------------------------------------------------

void Backend::startRemote()
{
    const QJsonObject opts = m_config.value("remote").toObject();
    if (!opts.value("enabled").toBool(true))
        return;
    m_server = new RemoteServer(this, m_token, this);
    const QHostAddress address(opts.value("bind").toString("0.0.0.0"));
    if (!m_server->listen(address, quint16(opts.value("port").toInt(8765)))) {
        qWarning("bsdtv: phone remote disabled: cannot listen on port %d", opts.value("port").toInt(8765));
        delete m_server;
        m_server = nullptr;
    }
}

void Backend::shutdown()
{
    m_runner->stop();
    delete m_server;
    m_server = nullptr;
}

void Backend::refreshInfo()
{
    m_ip = systemtools::ipAddress();
    m_stats = m_config.value("show_stats").toBool() ? systemtools::stats() : QString();
    emit infoChanged();
}

void Backend::onRunningChanged(const QString &appId)
{
    emit runningAppChanged();
    if (appId.isEmpty()) {
        emit raiseRequested();
        // Coming back from watching something: progress has changed.
        refreshMedia();
        touch();
    }
}

// --- properties --------------------------------------------------------------

QVariantList Backend::themes() const
{
    QVariantList out;
    const auto all = config::listThemes();
    for (auto it = all.begin(); it != all.end(); ++it) {
        out << QVariantMap{{"id", it.key()}, {"name", it->value("name").toString(it.key())},
                           {"accent", it->value("accent")}, {"background", it->value("background")}};
    }
    return out;
}

QString Backend::runningApp() const
{
    return m_runner->appId();
}

QString Backend::remoteUrl() const
{
    const QJsonObject opts = m_config.value("remote").toObject();
    if (!opts.value("enabled").toBool(true) || m_ip.isEmpty())
        return {};
    return QStringLiteral("http://%1:%2/?t=%3").arg(m_ip).arg(opts.value("port").toInt(8765)).arg(m_token);
}

QVariantList Backend::profiles() const
{
    return config::profiles(m_config).toVariantList();
}

QVariantMap Backend::profile() const
{
    return config::activeProfile(m_config).toVariantMap();
}

QStringList Backend::ambientPhotos() const
{
    if (!m_photos.isEmpty())
        return m_photos;
    QStringList backdrops;
    for (const QVariant &item : m_latest + m_resume) {
        const QString url = item.toMap().value("backdrop").toString();
        if (!url.isEmpty() && !backdrops.contains(url))
            backdrops << url;
    }
    return backdrops;
}

bool Backend::hasMpv() const
{
    return m_jellyfin && !QStandardPaths::findExecutable("mpv").isEmpty();
}

// --- actions -----------------------------------------------------------------

void Backend::reload()
{
    m_config = config::load();
    m_theme = config::loadTheme(m_config.value("theme").toString());
    const QString lang = resolveLanguage(m_config.value("language").toString());
    m_apps = apps::load(m_config, lang);

    QStringList dock;
    for (const QJsonValue &id : m_config.value("dock").toArray())
        dock << id.toString();
    m_appRows = apps::homeRows(m_apps, dock);
    m_appFeatured = apps::featured(m_apps, m_config);
    const QList<App> docked = apps::orderDock(m_apps, dock);
    QList<App> rest;
    for (const App &app : m_apps) {
        if (!dock.contains(app.id))
            rest << app;
    }
    m_dockModel->setApps(docked);
    m_appModel->setApps(rest);
    m_allModel->setApps(m_apps);

    JellyfinClient *client = JellyfinClient::fromConfig(m_config, this);
    const bool same = client && m_jellyfin && client->url() == m_jellyfin->url()
        && client->apiKey() == m_jellyfin->apiKey() && client->user() == m_jellyfin->user();
    if (same || (!client && !m_jellyfin)) {
        delete client;
    } else {
        delete m_jellyfin;
        m_jellyfin = client;
        m_resume.clear();
        m_latest.clear();
        refreshMedia();
    }
    m_photos = findPhotos(m_config.value("ambient").toObject().value("photos").toString());
    updateRows();

    emit settingsChanged();
    emit themeChanged();
    if (lang != m_lang) {
        m_lang = lang;
        emit languageChanged(lang);
    }
    touch();
}

void Backend::updateRows()
{
    m_rows = apps::withMedia(m_appRows, m_resume, m_latest);
    // Explicit "featured" entries win; otherwise films from Jellyfin, then apps.
    const QVariantList mediaSlides = JellyfinClient::slides(m_latest + m_resume);
    const bool explicitFeatured = !m_config.value("featured").toArray().isEmpty();
    m_featured = explicitFeatured || mediaSlides.isEmpty() ? m_appFeatured : mediaSlides;
    emit rowsChanged();
    emit ambientChanged();
}

void Backend::launch(const QString &appId)
{
    for (const App &app : m_apps) {
        if (app.id == appId)
            return m_runner->start(app.id, app.command(m_config.value("vars").toObject()));
    }
}

void Backend::home()
{
    m_runner->stop();
    emit raiseRequested();
}

void Backend::power(const QString &action)
{
    if (action == "restart_launcher") {
        // The session script restarts the launcher when it exits.
        QCoreApplication::exit(0);
        return;
    }
    if (action != "display_off")
        m_runner->stop();
    systemtools::power(action);
}

void Backend::volume(const QString &action)
{
    systemtools::volume(action);
}

void Backend::setSetting(const QString &key, const QVariant &value)
{
    config::saveUserSetting(key, QJsonValue::fromVariant(value));
    reload();
    refreshInfo();
}

// --- Jellyfin ----------------------------------------------------------------

void Backend::refreshMedia()
{
    if (!m_jellyfin) {
        if (!m_resume.isEmpty() || !m_latest.isEmpty()) {
            m_resume.clear();
            m_latest.clear();
            updateRows();
        }
        return;
    }
    const int generation = ++m_generation;
    QPointer<Backend> self(this);
    m_jellyfin->fetchHome([self, generation](const QVariantList &resume, const QVariantList &latest,
                                             const QString &error) {
        if (!self || generation != self->m_generation)
            return;
        if (!error.isEmpty()) {
            qWarning("bsdtv: jellyfin: %s", qPrintable(error));
            return;
        }
        self->m_resume = resume;
        self->m_latest = latest;
        self->updateRows();
    });
}

QVariantMap Backend::findMedia(const QString &mediaId) const
{
    for (const QVariant &item : m_resume + m_latest + m_search.value("media").toList()) {
        if (item.toMap().value("mediaId") == mediaId)
            return item.toMap();
    }
    return {};
}

void Backend::openMedia(const QString &mediaId)
{
    const QVariantMap item = findMedia(mediaId);
    if (!item.isEmpty())
        emit mediaRequested(item);
}

void Backend::watchInJellyfin(const QString &mediaId)
{
    for (const App &app : m_apps) {
        if (app.id != "jellyfin" || !m_jellyfin)
            continue;
        QJsonObject vars = m_config.value("vars").toObject();
        vars["jellyfin_url"] = m_jellyfin->webUrl(mediaId);
        return m_runner->start(app.id, app.command(vars));
    }
    emit message(QStringLiteral("Jellyfin: no Jellyfin app (set vars.jellyfin_url and install Firefox)"));
}

void Backend::playMedia(const QString &mediaId)
{
    const QVariantMap item = findMedia(mediaId);
    if (item.isEmpty() || !hasMpv())
        return;
    QStringList argv{"mpv", "--fs", "--force-media-title=" + item.value("title").toString()};
    const int resume = item.value("resumeSeconds").toInt();
    if (resume > 0)
        argv << QStringLiteral("--start=%1").arg(resume);
    m_runner->start("jellyfin", argv << m_jellyfin->streamUrl(mediaId));
}

// --- search ------------------------------------------------------------------

void Backend::search(const QString &rawTerm)
{
    const QString term = rawTerm.trimmed();
    const int generation = ++m_searchGeneration;
    m_search = {{"term", term}, {"apps", apps::search(m_apps, term)}, {"media", QVariantList()},
                {"busy", !term.isEmpty() && m_jellyfin}};
    emit searchChanged();
    if (term.isEmpty() || !m_jellyfin)
        return;
    QPointer<Backend> self(this);
    m_jellyfin->search(term, [self, generation](const QVariantList &items, const QString &error) {
        if (!self || generation != self->m_searchGeneration)
            return;
        if (!error.isEmpty())
            qWarning("bsdtv: jellyfin: %s", qPrintable(error));
        self->m_search["media"] = items;
        self->m_search["busy"] = false;
        emit self->searchChanged();
    });
}

// --- profiles ----------------------------------------------------------------

void Backend::setProfile(const QString &profileId)
{
    config::saveUserSetting("profile", profileId);
    reload();
}

// --- ambient mode ------------------------------------------------------------

void Backend::touch()
{
    if (m_ambient) {
        m_ambient = false;
        emit ambientChanged();
    }
    const QJsonObject opts = m_config.value("ambient").toObject();
    if (opts.value("enabled").toBool(true))
        m_idle.start(int(qMax(0.1, opts.value("after_minutes").toDouble(10)) * 60 * 1000));
    else
        m_idle.stop();
}

void Backend::startAmbient()
{
    if (!m_ambient) {
        m_ambient = true;
        emit ambientChanged();
    }
}

void Backend::onIdle()
{
    // Never cover a running app (someone may be watching a film).
    if (!m_runner->appId().isEmpty())
        touch();
    else
        startAmbient();
}

// --- RemoteController ----------------------------------------------------------

QVariantList Backend::remoteApps()
{
    QVariantList out;
    for (const App &app : m_apps)
        out << QVariantMap{{"id", app.id}, {"name", app.name}, {"color", app.color}};
    return out;
}

QVariantMap Backend::remoteStatus()
{
    return {{"running", m_runner->appId()}, {"version", version()}};
}

bool Backend::remoteLaunch(const QString &appId)
{
    const bool known = std::any_of(m_apps.begin(), m_apps.end(), [&](const App &a) { return a.id == appId; });
    if (known)
        launch(appId);
    return known;
}

bool Backend::remoteHome()
{
    home();
    return true;
}

bool Backend::remoteKey(const QString &name)
{
    return systemtools::sendKey(name);
}

bool Backend::remoteText(const QString &text)
{
    return systemtools::sendText(text);
}

bool Backend::remoteVolume(const QString &action)
{
    return systemtools::volume(action);
}

bool Backend::remotePower(const QString &action)
{
    if (!systemtools::powerCommands().contains(action))
        return false;
    // After the reply has gone out: the TV may be about to switch off.
    QTimer::singleShot(0, this, [this, action] { power(action); });
    return true;
}

bool Backend::remoteReload()
{
    QTimer::singleShot(0, this, &Backend::reload);
    return true;
}

// --- activity -----------------------------------------------------------------

bool ActivityFilter::eventFilter(QObject *, QEvent *event)
{
    switch (event->type()) {
    case QEvent::KeyPress:
    case QEvent::MouseButtonPress:
    case QEvent::MouseMove:
    case QEvent::Wheel: {
        const bool waking = m_backend->ambientActive() && event->type() == QEvent::KeyPress;
        m_backend->touch();
        return waking;
    }
    default:
        return false;
    }
}
