#include "task.h"

#include <QDebug>

#include "branchitem.h"
#include "taskmodel.h"
#include "vymmodel.h"


Task::Task(TaskModel *tm)
{
//    qDebug()<<"Constr. Task";
    status = NotStarted;
    awake  = Task::WideAwake;
    branch = NULL;
    prio   = 'X';
    model  = tm;
    date_creation = QDateTime::currentDateTime();
}

Task::~Task()
{
//    qDebug()<<"Destr. Task";
    if (branch) branch->setTask (NULL);
}

void Task::setModel (TaskModel* tm)
{
    model = tm;
}

void Task::cycleStatus(bool reverse)
{
    if (awake == Morning)
    {
	setAwake (WideAwake);
	return;
    }
    int i = status;
    reverse ?  i-- : i++;

    if ( i < 0) i = 2;
    if ( i > 2) i = 0;

    setStatus ( (Task::Status) i );

    if (branch) branch->updateTaskFlag ();
}

void Task::setStatus(const QString &s)
{
    if (s == "NotStarted")
	setStatus(NotStarted);
    else if (s == "WIP")
	setStatus(WIP);
    else if (s == "Finished")
	setStatus(Finished);
    else
	qWarning() << "Task::setStatus Unknown value: " << s;
}

void Task::setStatus(Status s)
{
    if (s == status) return;
    status = s;
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

QString Task::getIconString()
{
    QString s;
    switch (status) 
    {
        case NotStarted: 
            s = "task-new";
            break;
        case WIP: 
            s = "task-wip";
            break;
        case Finished: 
            s = "task-finished";
        break;
    }
    if (status != Finished)
        switch (awake) 
        {
            case Sleeping: 
                s += "-sleeping";
                break;
            case Morning: 
                s += "-morning";
                break;
            default: break;
        }
    return s;
}

void Task::setAwake(const QString &s)
{
    if (s == "Sleeping")
	setAwake(Sleeping);
    else if (s == "Morning")
	setAwake(Morning);
    else if (s == "WideAwake")
	setAwake(WideAwake);
    else
	qWarning() << "Task::setAwake Unknown value: " << s;
}

void Task::setAwake(Task::Awake a)
{
    awake = a;
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

bool Task::updateAwake()
{
    qint64 secs = getSecsSleep();

    if ( secs < 0 )
    {
        if ( awake == Task::Sleeping)
        {
            setAwake(Task::Morning);
            return true;
        }
    } else if ( secs > 0 )
    {
        if ( awake != Task::Sleeping)
        {
            setAwake(Task::Sleeping);
            return true;
        }
    }
    return false;
}

void Task::setPriority (int p)
{
    prio = p;
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
    date_creation = QDateTime().fromString (s,Qt::ISODate);
}


void Task::setDateModified()
{
    date_modified = QDateTime::currentDateTime();
}

void Task::setDateModified(const QString &s)
{
    date_modified = QDateTime().fromString (s,Qt::ISODate);
}

bool Task::setDaysSleep(qint64 n) 
{
    return setDateSleep ( QDate::currentDate().addDays (n).toString(Qt::ISODate) );
}

bool Task::setHoursSleep(qint64 n) 
{
    return setDateSleep ( QDateTime::currentDateTime().addSecs (n * 3600 ).toString(Qt::ISODate) );
}

bool Task::setSecsSleep(qint64 n) 
{
    return setDateSleep ( QDateTime::currentDateTime().addSecs (n).toString(Qt::ISODate) );
}

bool Task::setDateSleep(const QString &s)
{
    if (setDateSleep( QDateTime().fromString (s, Qt::ISODate) ))
        return true;
    else if (setDateSleep( QDateTime().fromString (s, Qt::TextDate) ))
        return true;
    else if (setDateSleep( QDateTime().fromString (s, Qt::DefaultLocaleShortDate) ))
        return true;
    else if (setDateSleep( QDateTime().fromString (s, Qt::DefaultLocaleLongDate) ))
        return true;
    else
        return false;
}

bool Task::setDateSleep(const QDateTime &d)
{
    if (!d.isValid() ) return false;

    date_sleep = d;
    updateAwake();
    if (status == Finished) setStatus(WIP); 
    return true;
}

qint64 Task::getDaysSleep()
{
    qint64 d = 1;
    if (date_sleep.isValid() )
	d = QDateTime::currentDateTime().daysTo (date_sleep);
    else
        qWarning() << "Task::getDaysSleep date_sleep is invalid";
    return d;
}

qint64 Task::getSecsSleep()
{
    qint64 d = 0;   // Meaning: No sleep time set so far
    if (date_sleep.isValid() )
	d = QDateTime::currentDateTime().secsTo (date_sleep);
    return d;
}

QDateTime Task::getSleep()
{
    return date_sleep;
}

void Task::setBranch (BranchItem *bi)
{
    branch  = bi;
    mapName = bi->getModel()->getMapName();
}

BranchItem* Task::getBranch ()
{
    return branch;
}

QString Task::getName ()
{
    if (branch)
        return branch->getHeadingPlain();
    else
    {
        qWarning()<<"Task::getName  no branch!";
        return "UNDEFINED";
    }
}

QString Task::getMapName ()
{
    return mapName;
}

QString Task::saveToDir()
{
    QString sleepAttr;
    if (getSecsSleep() > 0)
	sleepAttr = attribut ("date_sleep", date_sleep.toString (Qt::ISODate) );
    return singleElement ("task",
	attribut ("status", getStatusString() ) +
	attribut ("awake",  getAwakeString() ) +
	attribut ("date_creation", date_creation.toString (Qt::ISODate) ) +
	attribut ("date_modified", date_modified.toString (Qt::ISODate) ) +
	sleepAttr
     );
}

