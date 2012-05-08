#include "activetasksfiltermodel.h"
#include "taskmodel.h"

#include <QSortFilterProxyModel>

extern TaskModel *taskModel;

bool ActiveTasksFilterModel::filterAcceptsRow(int sourceRow,
         const QModelIndex &sourceParent) const
{
    if (useFilter)
    {
	QModelIndex ix = sourceModel()->index(sourceRow, 0, sourceParent);
	return (taskModel->getTask(ix)->getDaysSleep() <=0) && 
	       (taskModel->getTask(ix)->getStatus()!=Task::Finished);
    }	       
    return true;
}

void ActiveTasksFilterModel::setFilter (bool b)
{
    useFilter=b;
}

