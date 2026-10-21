// bsdtv-ctl: control a running launcher from the shell or from window manager key bindings.
#pragma once

#include <QJsonObject>
#include <QStringList>

namespace ctl {

// One request to the launcher on localhost; {"error": ...} when it is unreachable.
QJsonObject call(const QString &path, const QJsonObject *body = nullptr, int port = 0);
int run(const QStringList &args);

} // namespace ctl
