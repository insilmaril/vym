#include "adaptormodel.h"
#include <QtCore/QMetaObject>
#include <QtCore/QString>
#include <QtCore/QVariant>

#include "branchitem.h"
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

QDBusVariant AdaptorModel::branchCount()
{
    BranchItem *selbi=model->getSelectedBranch();
    if (selbi) 
	return QDBusVariant (selbi->branchCount() );
    else	
	return QDBusVariant (-1 );
}

QDBusVariant AdaptorModel::runScript (const QString &s)
{
    return QDBusVariant (model->runScript (s));
}

