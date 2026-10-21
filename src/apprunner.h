// Runs one foreground app at a time, like a TV does.
#pragma once

#include <QObject>
#include <QPointer>
#include <QProcess>
#include <QStringList>

class AppRunner : public QObject
{
    Q_OBJECT

public:
    explicit AppRunner(QObject *parent = nullptr);
    ~AppRunner() override;

    QString appId() const { return m_appId; }

    // Stops the current app, then starts argv in its own process group.
    virtual void start(const QString &appId, const QStringList &argv);
    void stop();

signals:
    void runningChanged(const QString &appId);
    void failed(const QString &appId, const QString &error);

private:
    void clear();

    QPointer<QProcess> m_process;
    QString m_appId;
};
