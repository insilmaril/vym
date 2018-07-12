#include "taskfiltermodel.h"
#include "taskmodel.h"

#include <QSortFilterProxyModel>

#include "branchitem.h"

extern TaskModel *taskModel;

void TaskFilterModel::setFilter (bool b)
{
    useFilter = b;
}

void TaskFilterModel::setMapFilter (const QString &s)	
{
    mapFilter = s;
}

void TaskFilterModel::setFilterNew (bool b)
{
    filterNew = b;
}

void TaskFilterModel::setFilterFlags1 (bool b)
{
    filterFlags1 = b;
}

void TaskFilterModel::setFilterFlags2 (bool b)
{
    filterFlags2 = b;
}

void TaskFilterModel::setFilterFlags3 (bool b)
{
    filterFlags3 = b;
}

bool TaskFilterModel::filterAcceptsRow(int sourceRow, 
         const QModelIndex &sourceParent) const
{
    QModelIndex ix = sourceModel()->index(sourceRow, 0, sourceParent);

    // Filter by mapname
    QString mapname = taskModel->getTask(ix)->getMapName();
    if (mapname.isEmpty()) mapname = "justSomePseudoMapNameForFiltering";
    if ( !mapFilter.isEmpty() && mapname != mapFilter )
        return false;

    // Filter new tasks
    if (filterNew && taskModel->getTask(ix)->getAwake() != Task::Morning)
        return false;

    // Filter active tasks
    if (useFilter && ((taskModel->getTask(ix)->getSecsSleep() > 0) || (taskModel->getTask(ix)->getStatus() == Task::Finished) ) )
        return false;

    // Filter arrow flags
    if (filterFlags1 && filterFlags2)
    {
        if ( taskModel->getTask(ix)->getBranch()->hasActiveStandardFlag("arrow-up") ||
             taskModel->getTask(ix)->getBranch()->hasActiveStandardFlag("2arrow-up") )
            return true;
        else
            return false;
    }

    if (filterFlags1 && !taskModel->getTask(ix)->getBranch()->hasActiveStandardFlag("arrow-up") )
        return false;

    if (filterFlags2 && !taskModel->getTask(ix)->getBranch()->hasActiveStandardFlag("2arrow-up") )
        return false;

    // Filter flags: Flags, which have neither arrow-up nor 2arrow-up   // FIXME-1 review
    if (filterFlags3 && (taskModel->getTask(ix)->getBranch()->hasActiveStandardFlag("arrow-up") 
                     ||  taskModel->getTask(ix)->getBranch()->hasActiveStandardFlag("2arrow-up") ) )
        return false;
    return true;
}

