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
	    status=WIP;
	    break;
	case Task::WIP: 
	    status=Finished;
	    break;
	case Task::Finished: 
	    status=NotStarted;
	    break;
    }
    if (model) model->emitDataHasChanged (this);
    if (branch) branch->updateTaskFlag();
}

void Task::setStatus(const QString &s)
{
    if (s=="NotStarted")
	status=NotStarted;
    else if (s=="WIP")
	status=WIP;
    else if (s=="Finished")
	status=Finished;
    else
	qWarning()<<"Task::setStatus s="<<s;
}

void Task::setStatus(Status s)
{
    status=s;
}

Task::Status Task::getStatus()
{
    return status;
}

QString Task::getStatusString()	    // FIXME-2 translate? 
{
    switch (status)
    {
	case NotStarted: return "Not started";
	case WIP: return "WIP";
	case Finished: return "Finished";
    }
    return "Undefined";
}

void Task::setPriority (QChar p)
{
    prio=p;
}

QChar Task::getPriority()
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

