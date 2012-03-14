#include <QtCore/QMetaObject>
#include <QtCore/QString>
#include <QtCore/QVariant>

#include "adaptorvym.h"
#include "mainwindow.h"

extern QString vymInstanceName;
extern QString vymVersion;
extern Main *mainWindow;

AdaptorVym::AdaptorVym(QObject *obj)
         : QDBusAbstractAdaptor(obj)
{
    setAutoRelaySignals (true);
}

QDBusVariant AdaptorVym::modelCount()
{
    return QDBusVariant (mainWindow->modelCount() );
}

QDBusVariant AdaptorVym::getInstanceName()
{
    return QDBusVariant (vymInstanceName);
}

QDBusVariant AdaptorVym::getVersion()
{
    return QDBusVariant (vymVersion);
}
