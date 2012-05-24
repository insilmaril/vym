#include "taskfiltermodel.h"
#include "taskmodel.h"

#include <QSortFilterProxyModel>

#include "branchitem.h"

extern TaskModel *taskModel;

void TaskFilterModel::setFilter (bool b)
{
    useFilter=b;
}

void TaskFilterModel::setMapFilter (const QString &s)
{
    mapFilter=s;
}

bool TaskFilterModel::filterAcceptsRow(int sourceRow, 
         const QModelIndex &sourceParent) const
{
    QModelIndex ix = sourceModel()->index(sourceRow, 0, sourceParent);
    bool r=true;
    if ( !mapFilter.isEmpty() && (taskModel->getTask(ix)->getMapName() != mapFilter ) ) 
	r=false;

    if (useFilter)
    {
	r=r && (taskModel->getTask(ix)->getDaysSleep() <=0) && (taskModel->getTask(ix)->getStatus()!=Task::Finished);
    } 
    return r;
}

