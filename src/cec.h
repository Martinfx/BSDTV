// HDMI-CEC bridge: lets the TV's own remote drive BSDTV.
//
// Runs cec-client from libcec (a Pulse-Eight USB-CEC adapter or any adapter
// libcec supports on FreeBSD), parses the key presses it logs and forwards
// them to the launcher API.
#pragma once

#include <QPair>
#include <QStringList>

namespace cec {

// (action, argument) for a cec-client log line, e.g. ("key", "up"); empty action if none.
QPair<QString, QString> parseLine(const QString &line);
int run(const QStringList &args);

} // namespace cec
