// Loads the real QML UI offscreen with a backend whose apps are fakes.
#pragma once

#include "apprunner.h"
#include "backend.h"
#include "testutil.h"

#include <QGuiApplication>
#include <QJSValue>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTest>

class FakeRunner : public AppRunner
{
public:
    QStringList started;
    QList<QStringList> commands;
    void start(const QString &appId, const QStringList &argv) override
    {
        started << appId;
        commands << argv;
    }
};

struct UiFixture
{
    IsolatedConfig env;
    QTemporaryDir bin;
    FakeRunner *runner = nullptr;
    Backend *backend = nullptr;
    QQmlApplicationEngine *engine = nullptr;
    QQuickWindow *window = nullptr;
    QStringList messages;

    // Pretends these programs are installed by putting stubs first on PATH.
    explicit UiFixture(const QStringList &installed, const QJsonObject &userConfig = {})
    {
        for (const QString &name : installed) {
            QFile file(bin.path() + "/" + name);
            file.open(QIODevice::WriteOnly);
            file.write("#!/bin/sh\n");
            file.setPermissions(file.permissions() | QFileDevice::ExeOwner);
        }
        qputenv("PATH", bin.path().toUtf8() + ":/usr/bin:/bin");
        if (!userConfig.isEmpty())
            IsolatedConfig::write(env.user() + "/config.json", userConfig);
    }

    bool load()
    {
        runner = new FakeRunner;
        backend = new Backend(runner);
        qApp->installEventFilter(new ActivityFilter(backend));
        engine = new QQmlApplicationEngine;
        QObject::connect(engine, &QQmlApplicationEngine::warnings, engine, [this](const QList<QQmlError> &errors) {
            for (const QQmlError &e : errors)
                messages << e.toString();
        });
        engine->setInitialProperties({{"windowed", true}});
        engine->loadFromModule("BSDTV", "Main");
        if (engine->rootObjects().isEmpty())
            return false;
        window = qobject_cast<QQuickWindow *>(engine->rootObjects().constFirst());
        window->show();
        return QTest::qWaitForWindowExposed(window);
    }

    ~UiFixture()
    {
        delete engine;
        delete backend;
    }

    // Name of whatever has keyboard focus: an app or film, a button, a key.
    QString focused() const
    {
        QQuickItem *item = window->activeFocusItem();
        if (!item)
            return {};
        QVariant data = item->property("modelData");
        if (data.metaType() == QMetaType::fromType<QJSValue>())
            data = data.value<QJSValue>().toVariant();
        if (data.typeId() == QMetaType::QVariantMap)
            return data.toMap().value("name", data.toMap().value("title")).toString();
        for (const char *prop : {"name", "label", "text"}) {
            const QString value = item->property(prop).toString();
            if (!value.isEmpty())
                return value;
        }
        return QString::fromLatin1(item->metaObject()->className());
    }

    void press(std::initializer_list<Qt::Key> keys)
    {
        for (Qt::Key key : keys)
            QTest::keyClick(window, key);
        QTest::qWait(50);
    }

    QQuickItem *home() const
    {
        return window->findChild<QQuickItem *>("home")->property("item").value<QQuickItem *>();
    }
};
