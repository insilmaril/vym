#include "task.h"

#include <QDebug>

#include "branchitem.h"


Task::Task()
{
    status=NotStarted;
    branch=NULL;
    prio='X';
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
