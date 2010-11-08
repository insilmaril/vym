#include "adaptormodel.h"
#include <QtCore/QMetaObject>
#include <QtCore/QString>
#include <QtCore/QVariant>

#include "mainwindow.h"
#include "vymmodel.h"

extern QString vymInstanceName;
extern Main *mainWindow;

AdaptorModel::AdaptorModel(QObject *obj)
         : QDBusAbstractAdaptor(obj)
{
    model=static_cast <VymModel*> (obj);
    setAutoRelaySignals (true);
}

AdaptorModel::~AdaptorModel()
{
    // destructor
}

void AdaptorModel::setModel(VymModel *vm)
{
    model=vm;
}

QString AdaptorModel::caption()
{
    return m_caption;
}

void AdaptorModel::setCaption (const QString &newCaption)
{
    m_caption=newCaption;
}

QDBusVariant AdaptorModel::query(const QString &)
{
    QString s;
    if (model)
    s=model->getHeading();
    else
    s="oops, no vymModel?";

    return QDBusVariant (s);
}

QDBusVariant AdaptorModel::getCurrentModelID()
{
    return QDBusVariant (mainWindow->currentModelID());
}

QDBusVariant AdaptorModel::getHeading()
{
    QString s;
    if (model)
    s=model->getHeading();
    else
    s="oops, no vymModel?";

    return QDBusVariant (s);
}


void AdaptorModel::setHeading (const QString &s)
{
    model->setHeading(s);
}

QDBusVariant AdaptorModel::getInstanceName()
{
    return QDBusVariant (vymInstanceName);
}

QDBusVariant AdaptorModel::execute (const QString &s)
{
    if (model)
    return QDBusVariant (model->runScript (s));
    else
    return QDBusVariant ("No model.");
}

