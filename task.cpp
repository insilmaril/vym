#include "task.h"

#include <QDebug>

#include "branchitem.h"
#include "taskmodel.h"


Task::Task(TaskModel *tm)
{
//    qDebug()<<"Constr. Task";
    status=NotStarted;
    branch=NULL;
    prio='X';
    model=tm;
    date_creation=QDateTime::currentDateTime();
    date_sleep=QDateTime::currentDateTime();
}

Task::~Task()
{
//    qDebug()<<"Destr. Task";
    if (branch) branch->setTask (NULL);
}

void Task::setModel (TaskModel* tm)
{
    model=tm;
}

void Task::cycleStatus()
{
    switch (status)
    {
	case Task::NotStarted: 
	    setStatus(WIP);
	    break;
	case Task::WIP: 
	    setStatus(Finished);
	    break;
	case Task::Finished: 
	    setStatus(NotStarted);
	    break;
    }
    if (model) model->emitDataHasChanged (this);
    if (branch) branch->updateTaskFlag();
}

void Task::setStatus(const QString &s)
{
    if (s=="NotStarted")
	setStatus(NotStarted);
    else if (s=="WIP")
	setStatus(WIP);
    else if (s=="Finished")
	setStatus(Finished);
    else
	qWarning()<<"Task::setStatus s="<<s;
}

void Task::setStatus(Status s)
{
    status=s;
    if (branch) branch->updateTaskFlag();
}

Task::Status Task::getStatus()
{
    return status;
}

QString Task::getStatusString()	    // FIXME-2 translate? 
{
    switch (status)
    {
	case NotStarted: return "NotStarted";
	case WIP: return "WIP";
	case Finished: return "Finished";
    }
    return "Undefined";
}

void Task::setPriority (int p)
{
    prio=p;
}

int Task::getPriority()
{
    return prio;
}

int Task::getAgeCreation()
{
    return date_creation.daysTo (QDateTime::currentDateTime() );
}

int Task::getAgeModified()
{
    if (date_modified.isValid() )
	return date_modified.daysTo (QDateTime::currentDateTime() );
    else
	return getAgeCreation();
}

void Task::setDateCreation (const QString &s)
{
    date_creation=QDateTime().fromString (s,Qt::ISODate);
}


void Task::setDateModified()
{
    date_modified=QDateTime::currentDateTime();
}

void Task::setDateModified(const QString &s)
{
    date_modified=QDateTime().fromString (s,Qt::ISODate);
}

void Task::setDateSleep(const QString &s)
{
    date_sleep=QDateTime().fromString (s,Qt::ISODate);
    // Let's forget the time here:
    date_sleep.setTime (QTime (0,0,0) );
}

int Task::getDaysSleep()
{
    if (date_sleep.isValid() )
	return QDateTime::currentDateTime().daysTo (date_sleep);
    else
	return 0;
}

void Task::setBranch (BranchItem *bi)
{
    branch=bi;
}

BranchItem* Task::getBranch ()
{
    return branch;
}

QString Task::getName ()
{
    if (branch)
	return branch->getHeading();
    else
    {
	qWarning()<<"Task::getName  no branch!";
	return "UNDEFINED";
    }
}
QString Task::saveToDir()
{
    return singleElement ("task",
	attribut ("status",getStatusString() ) +
	attribut ("date_creation",date_creation.toString (Qt::ISODate) ) +
	attribut ("date_modified",date_modified.toString (Qt::ISODate) ) +
	attribut ("date_sleep",date_sleep.toString (Qt::ISODate) )
     );
}

