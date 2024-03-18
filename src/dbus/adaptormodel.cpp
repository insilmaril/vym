#include "adaptormodel.h"
#include <QtCore/QMetaObject>
#include <QtCore/QString>
#include <QtCore/QVariant>

#include "branchitem.h"
#include "command.h"
#include "mainwindow.h"
#include "vymmodel.h"

extern QString vymInstanceName;
extern Main *mainWindow;

extern QList<Command *> modelCommands;

AdaptorModel::AdaptorModel(QObject *obj) : QDBusAbstractAdaptor(obj)
{
    model = static_cast<VymModel *>(obj);
    setAutoRelaySignals(true);
}

void AdaptorModel::setModel(VymModel *vm) { model = vm; }

QString AdaptorModel::caption() { return m_caption; }

void AdaptorModel::setCaption(const QString &newCaption)
{
    m_caption = newCaption;
}

QDBusVariant AdaptorModel::branchCount()
{
    BranchItem *selbi = model->getSelectedBranch();
    if (selbi)
        return QDBusVariant(selbi->branchCount());
    else
        return QDBusVariant(-1);
}

QDBusVariant AdaptorModel::execute(const QString &s)
{
    return QDBusVariant(model->execute(s));
}

QDBusVariant AdaptorModel::errorLevel()
{
    return QDBusVariant(); // model->parser.errorLevel() );     // FIXME-4
                           // really still needed? parser no longer used.
}

QDBusVariant AdaptorModel::errorDescription()
{
    return QDBusVariant(); // model->parser.errorDescription() );// FIXME-4
                           // really still needed? parser no longer used.
}

QDBusVariant AdaptorModel::listCommands()
{
    QStringList list;

    foreach (Command *command, modelCommands)
        list << command->name();

    return QDBusVariant(list.join(","));
}
