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
    model->recalcPriorities();
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
    return singleElement ("task", attribut ("status",getStatusString() ) );
}

