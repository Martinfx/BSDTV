#include "apprunner.h"

#include <QTimer>

#include <csignal>
#include <unistd.h>

AppRunner::AppRunner(QObject *parent)
    : QObject(parent)
{
}

AppRunner::~AppRunner()
{
    stop();
}

static void signalGroup(qint64 pid, int sig)
{
    if (pid > 0)
        ::kill(-pid, sig);
}

void AppRunner::start(const QString &appId, const QStringList &argv)
{
    stop();
    if (argv.isEmpty())
        return;

    auto *process = new QProcess(this);
    process->setProgram(argv.first());
    process->setArguments(argv.mid(1));
    process->setStandardInputFile(QProcess::nullDevice());
    process->setProcessChannelMode(QProcess::ForwardedChannels);
    // Own session and process group, so stopping the app also stops its helpers
    // (Firefox content processes, Kodi add-ons, ...).
    process->setChildProcessModifier([] { ::setsid(); });

    connect(process, &QProcess::errorOccurred, this, [this, process, appId](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart)
            return;
        emit failed(appId, process->errorString());
        if (process == m_process)
            clear();
        process->deleteLater();
    });
    connect(process, &QProcess::finished, this, [this, process] {
        if (process == m_process)
            clear();
        process->deleteLater();
    });

    m_process = process;
    m_appId = appId;
    process->start();
    emit runningChanged(appId);
}

void AppRunner::stop()
{
    QProcess *process = m_process;
    if (!process)
        return;
    clear();
    if (process->state() == QProcess::NotRunning) {
        process->deleteLater();
        return;
    }
    const qint64 pid = process->processId();
    signalGroup(pid, SIGTERM);
    // Anything left in the group after the grace period gets killed.
    QTimer::singleShot(3000, this, [pid] { signalGroup(pid, SIGKILL); });
}

void AppRunner::clear()
{
    m_process = nullptr;
    m_appId.clear();
    emit runningChanged(QString());
}
