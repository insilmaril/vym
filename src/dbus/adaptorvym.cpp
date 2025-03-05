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

QDBusVariant AdaptorVym::mapCount() // FIXME-4 duplicate in VymWrapper
{
    return QDBusVariant(mainWindow->modelCount());
}

void AdaptorVym::gotoMapID(const uint &id) { mainWindow->gotoModelWithId(id); } // FIXME-4 duplicate in VymWrapper

QDBusVariant AdaptorVym::getInstanceName()
{
    return QDBusVariant(vymInstanceName);
}

QDBusVariant AdaptorVym::getVersion() { return QDBusVariant(vymVersion); } // FIXME-4 duplicate in VymWrapper

QDBusVariant AdaptorVym::runScript(const QString &s)
{
    //qDebug() << "AdaptorVym::runScript s=" << s;
    QVariant v = mainWindow->runScript(s);
    //qDebug() << "                    v=" << v;
    return QDBusVariant(v);
}

QDBusVariant AdaptorVym::listCommands()
{
    QStringList list;

    foreach (Command *command, vymCommands)
        list << command->name();

    list << "runScript";

    return QDBusVariant(list.join(","));
}

QDBusVariant AdaptorVym::currentMapID() // FIXME-4 duplicate in VymWrapper
{
    return QDBusVariant(mainWindow->currentMapId());
}
