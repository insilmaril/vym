#include "taskfiltermodel.h"
#include "taskmodel.h"

#include <QSortFilterProxyModel>

#include "branchitem.h"

extern TaskModel *taskModel;

void TaskFilterModel::setFilter(bool b) { useFilter = b; }

void TaskFilterModel::setMapFilter(const QString &s) { mapFilter = s; }

void TaskFilterModel::setFilterNew(bool b) { filterNew = b; }

void TaskFilterModel::setFilterBlocker(bool b) { filterBlocker = b; }

void TaskFilterModel::setFilterFlags1(bool b) { filterFlags1 = b; }

void TaskFilterModel::setFilterFlags2(bool b) { filterFlags2 = b; }

void TaskFilterModel::setFilterFlags3(bool b) { filterFlags3 = b; }

bool TaskFilterModel::taskVisible(Task* task) const
{
    // Filter by mapname
    QString mapname = task->getMapName();
    if (mapname.isEmpty())
        mapname = "justSomePseudoMapNameForFiltering";
    if (!mapFilter.isEmpty() && mapname != mapFilter)
        return false;

    // Filter new tasks
    if (filterNew && task->getAwake() != Task::Morning)
        return false;

    BranchItem *bi = task->getBranch();

    // Filter blocker tasks (stopsign)
    if (filterBlocker && !bi->hasActiveFlag("stopsign") )
        return false;

    // Filter active tasks
    if (useFilter && ((task->getSecsSleep() > 0) ||
                      (task->getStatus() == Task::Finished)))
        return false;

    // Filter arrow flags
    if (filterFlags1 && filterFlags2) {
        if (bi->hasActiveFlag("arrow-up") ||
            bi->hasActiveFlag("2arrow-up"))
            return true;
        else
            return false;
    }

    if (filterFlags1 &&
        !bi->hasActiveFlag("arrow-up"))
        return false;

    if (filterFlags2 &&
        !bi->hasActiveFlag("2arrow-up"))
        return false;

    // Filter flags: Flags, which have neither arrow-up nor 2arrow-up
    if (filterFlags3 &&
        (bi->hasActiveFlag("arrow-up") ||
         bi->hasActiveFlag("2arrow-up")))
        return false;
    return true;
}

bool TaskFilterModel::filterAcceptsRow(int sourceRow,
                                       const QModelIndex &sourceParent) const
{
    QModelIndex ix = sourceModel()->index(sourceRow, 0, sourceParent);

    return taskVisible(taskModel->getTask(ix));
}
