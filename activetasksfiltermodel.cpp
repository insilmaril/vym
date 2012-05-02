#include "activetasksfiltermodel.h"
#include "taskmodel.h"

#include <QSortFilterProxyModel>

extern TaskModel *taskModel;

bool ActiveTasksFilterModel::filterAcceptsRow(int sourceRow,
         const QModelIndex &sourceParent) const
{
    QModelIndex ix = sourceModel()->index(sourceRow, 0, sourceParent);
    if (useFilter)
	return taskModel->getTask(ix)->getDaysSleep() <=0 && 
	       taskModel->getTask(ix)->getStatus()!=Task::Finished;
    else
	return true;
}

void ActiveTasksFilterModel::setFilter (bool b)
{
    useFilter=b;
}

