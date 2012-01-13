#include "task.h"

#include <QDebug>

#include "branchitem.h"
#include "taskmodel.h"


Task::Task(TaskModel *tm)
{
//    qDebug()<<"Constr. Task";
    status=NotStarted;
    awake=Task::WideAwake;
    branch=NULL;
    prio='X';
    model=tm;
    date_creation=QDateTime::currentDateTime();
    date_sleep=QDate::currentDate();
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

void Task::cycleStatus(bool reverse)
{
    if (awake==Morning)
    {
	awake=WideAwake;
	return;
    }
    int i=status;
    reverse ?  i-- : i++;

    if (i<0) i=2;
    if (i>2) i=0;

    setStatus ( (Task::Status) i );

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
	qWarning()<<"Task::setStatus Unknown value: "<<s;
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

QString Task::getStatusString()	    
{
    switch (status)
    {
	case NotStarted: return "NotStarted";
	case WIP: return "WIP";
	case Finished: return "Finished";
    }
    return "Undefined";
}

void Task::setAwake(const QString &s)
{
    if (s=="Sleeping")
	setAwake(Sleeping);
    else if (s=="Morning")
	setAwake(Morning);
    else if (s=="WideAwake")
	setAwake(WideAwake);
    else
	qWarning()<<"Task::setAwake Unknown value: "<<s;
}

void Task::setAwake(Task::Awake a)
{
    awake=a;
}

Task::Awake Task::getAwake()
{
    return awake;
}

QString Task::getAwakeString()	    
{
    switch (getAwake() )
    {
	case Sleeping: return "Sleeping";
	case Morning: return "Morning";
	case WideAwake: return "WideAwake";
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

void Task::setDateSleep(int n)
{
    if (n>0)
    {
	date_sleep=QDate::currentDate().addDays (n);
	if (n>0) awake=Sleeping;
    }
    else
	date_sleep.setDate (-1,-1,-1);	// Create invalid date
}

void Task::setDateSleep(const QString &s)
{
    date_sleep=QDate().fromString (s,Qt::ISODate);
    if (getDaysSleep()>0) awake=Sleeping;
}

int Task::getDaysSleep()
{
    int d=0;
    if (date_sleep.isValid() )
	d=QDate::currentDate().daysTo (date_sleep);
    //if (d<0) d=0;
    return d;
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
	attribut ("awake",getAwakeString() ) +
	attribut ("date_creation",date_creation.toString (Qt::ISODate) ) +
	attribut ("date_modified",date_modified.toString (Qt::ISODate) ) +
	attribut ("date_sleep",date_sleep.toString (Qt::ISODate) )
     );
}

