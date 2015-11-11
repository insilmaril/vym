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
//FIXME-2 changing mapname does not retrigger filtering
{
    mapFilter = s;
}

void TaskFilterModel::setFilterNew (bool b)
{
    filterNew = b;
}

bool TaskFilterModel::filterAcceptsRow(int sourceRow, 
         const QModelIndex &sourceParent) const
{
    QModelIndex ix = sourceModel()->index(sourceRow, 0, sourceParent);

    // Filter by mapname
    QString mapname = taskModel->getTask(ix)->getMapName();
    if (mapname.isEmpty()) mapname = "barfoo";
    if ( !mapFilter.isEmpty() && mapname != mapFilter )
        //FIXME-2 new (unnamed) map does not filter, but shows tasks from all maps
        return false;

    // Filter new tasks
    if (filterNew && taskModel->getTask(ix)->getAwake() != Task::Morning)
        return false;

    // Filter active tasks
    if (useFilter && ((taskModel->getTask(ix)->getDaysSleep() > 0) || (taskModel->getTask(ix)->getStatus() == Task::Finished) ) )
        return false;
    return true;
}

