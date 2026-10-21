// Thin wrappers around FreeBSD system tools (mixer, shutdown, sysctl, xdotool).
#pragma once

#include <QMap>
#include <QString>
#include <QStringList>

#include <functional>

namespace systemtools {

// Remote key name -> X keysym sent with xdotool.
const QMap<QString, QString> &keysyms();
const QMap<QString, QStringList> &powerCommands();

// Runs a command and reports success. Tests replace it to see what would run.
using Runner = std::function<bool(const QStringList &argv)>;
void setRunner(Runner runner);
bool run(const QStringList &argv);

bool sendKey(const QString &name);
bool sendText(const QString &text);
// Default mixer volume with FreeBSD's mixer(8): up, down or mute.
bool volume(const QString &action);
bool power(const QString &action);

// Best-effort LAN address.
QString ipAddress();
// Short CPU temperature / free memory / free disk line for the home screen.
QString stats();

} // namespace systemtools
