#include <QtCore/QMetaObject>
#include <QtCore/QString>
#include <QtCore/QVariant>

#include "adaptorvym.h"
#include "command.h"
#include "mainwindow.h"

extern QString vymInstanceName;
extern QString vymVersion;
extern Main *mainWindow;

extern QList<Command *> vymCommands;

AdaptorVym::AdaptorVym(QObject *obj) : QDBusAbstractAdaptor(obj)
{
    setAutoRelaySignals(true);
}

QDBusVariant AdaptorVym::mapCount()
{
    return QDBusVariant(mainWindow->modelCount());
}

void AdaptorVym::gotoMapID(const uint &id) { mainWindow->gotoModelID(id); }

QDBusVariant AdaptorVym::getInstanceName()
{
    return QDBusVariant(vymInstanceName);
}

QDBusVariant AdaptorVym::getVersion() { return QDBusVariant(vymVersion); }

QDBusVariant AdaptorVym::execute(const QString &s)
{
    return QDBusVariant(mainWindow->runScript(s));
}

QDBusVariant AdaptorVym::listCommands()
{
    QStringList list;

    foreach (Command *command, vymCommands)
        list << command->getName();

    return QDBusVariant(list.join(","));
}

QDBusVariant AdaptorVym::currentMapID()
{
    return QDBusVariant(mainWindow->currentMapID());
}
