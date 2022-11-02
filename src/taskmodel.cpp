#include "taskmodel.h"

#include <QDebug>

#include "branchitem.h"
#include "task.h"
#include "vymmodel.h"

TaskModel::TaskModel(QObject *parent) : QAbstractTableModel(parent)
{
    showParentsLevel = 0;

    QSize size = QSize(22, 22);
    QSize size2 = QSize(44, 22);

    arrow_up_icon =
        QIcon(QPixmap(":/flag-arrow-up.svg").scaled(size, Qt::KeepAspectRatio));
    arrow_2up_icon = QIcon(
        QPixmap(":/flag-arrow-2up.svg").scaled(size, Qt::KeepAspectRatio));

    task_new_icon =
        QIcon(QPixmap(":/flag-task-new.svg").scaled(size, Qt::KeepAspectRatio));
    task_new_morning_icon = QIcon(QPixmap(":/flag-task-new-morning.svg")
                                      .scaled(size, Qt::KeepAspectRatio));
    task_new_sleeping_icon = QIcon(QPixmap(":/flag-task-new-sleeping.svg")
                                       .scaled(size, Qt::KeepAspectRatio));

    task_wip_icon =
        QIcon(QPixmap(":/flag-task-wip.svg").scaled(size, Qt::KeepAspectRatio));
    task_wip_morning_icon = QIcon(QPixmap(":/flag-task-wip-morning.svg")
                                      .scaled(size, Qt::KeepAspectRatio));
    task_wip_sleeping_icon = QIcon(QPixmap(":/flag-task-wip-sleeping.svg")
                                       .scaled(size, Qt::KeepAspectRatio));

    task_finished_icon = QIcon(
        QPixmap(":/flag-task-finished.svg").scaled(size, Qt::KeepAspectRatio));

    taskfilter_stopsign_icon =
        QIcon(QPixmap(":/flag-stopsign.svg").scaled(size, Qt::KeepAspectRatio));
    taskfilter_stopsign_arrow_up_icon =
        QIcon(QPixmap(":/flag-stopsign-arrow-up.png")
                  .scaled(size2, Qt::KeepAspectRatio));
    taskfilter_stopsign_arrow_2up_icon =
        QIcon(QPixmap(":/flag-stopsign-arrow-2up.png")
                  .scaled(size2, Qt::KeepAspectRatio));
}

QModelIndex TaskModel::index(Task *t) const
{
    int n = tasks.indexOf(t);
    if (n < 0)
        return QModelIndex();
    else
        return createIndex(n, 0, t);
}

QModelIndex TaskModel::indexRowEnd(Task *t)
{
    int n = tasks.indexOf(t);
    if (n < 0)
        return QModelIndex();
    else
        return createIndex(n, 8, t);
}

Task *TaskModel::getTask(const QModelIndex &ix) const
{
    if (ix.isValid())
        return tasks.at(ix.row());
    else
        return nullptr;
}

Task *TaskModel::getTask(const int i) const
{
    if (i >= 0 && i < count())
        return getTask(createIndex(i, 0));
    else
        return nullptr;
}

int TaskModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return tasks.size();
}

int TaskModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 9;
}

QVariant TaskModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.row() >= tasks.size() || index.row() < 0)
        return QVariant();

    BranchItem *bi = tasks.at(index.row())->getBranch();
    Task *t = tasks.at(index.row());

    if (role == Qt::DisplayRole) {
        if (index.column() == 0)
            return t->getPriority();
        else if (index.column() == 1)
            return t->getPriorityDelta();
        else if (index.column() == 2)
            return QString(); // return t->getStatusString() + " - "
                              // +t->getAwakeString();
        else if (index.column() == 3)
            return t->getAgeCreation();
        else if (index.column() == 4)
            return t->getAgeModification();
        else if (index.column() == 5) {
            if (t->getDaysSleep() > 0)
                return t->getDaysSleep();
            else
                return "-";
        }
        else if (index.column() == 6) {
            QString s = bi->getModel()->getMapName();
            if (s.isEmpty())
                return "-";
            else
                return bi->getModel()->getMapName();
        }
        else if (index.column() == 8) {
            BranchItem *bi = tasks.at(index.row())->getBranch();
            return bi->getHeadingPlainWithParents(showParentsLevel);
        }
    }
    else if (role == Qt::DecorationRole && index.column() == 2) {
        QString s = t->getIconString();
        if (s == "task-new")
            return task_new_icon;
        else if (s == "task-new-morning")
            return task_new_morning_icon;
        else if (s == "task-new-sleeping")
            return task_new_sleeping_icon;
        else if (s == "task-wip")
            return task_wip_icon;
        else if (s == "task-wip-sleeping")
            return task_wip_sleeping_icon;
        else if (s == "task-wip-morning")
            return task_wip_morning_icon;
        else if (s == "task-finished")
            return task_finished_icon;
        else {
            qWarning() << "Unknown task type in TaskModel::data: " << s;
            return QVariant();
        }
    }
    else if (role == Qt::DecorationRole && index.column() == 7) {
        BranchItem *bi = t->getBranch();
        if (bi->hasActiveFlag("stopsign")) {
            if (bi->hasActiveFlag("2arrow-up"))
                return taskfilter_stopsign_arrow_2up_icon;
            else if (bi->hasActiveFlag("arrow-up"))
                return taskfilter_stopsign_arrow_up_icon;
            else
                return taskfilter_stopsign_icon;
        }
        else {
            if (bi->hasActiveFlag("2arrow-up"))
                return arrow_2up_icon;
            else if (bi->hasActiveFlag("arrow-up"))
                return arrow_up_icon;
        }
        return QIcon();
    }
    else // role != Qt::DisplayRole
    {
        if (role == Qt::EditRole && index.column() == 1) // DeltaPrio
            return t->getPriorityDelta();
        if (role == Qt::ForegroundRole && bi)
            return bi->getHeadingColor();
        if (role == Qt::BackgroundRole && bi) {
            return bi->getBackgroundColor(bi);
        }
    }

    return QVariant();
}

QVariant TaskModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal) {
        switch (section) {
        case 0:
            return tr("Prio", "TaskEditor");
        case 1:
            return tr("Delta", "TaskEditor");
        case 2:
            return tr("Status", "TaskEditor");
        case 3:
            return tr("Age total", "TaskEditor");
        case 4:
            return tr("Age mod.", "TaskEditor");
        case 5:
            return tr("Sleep", "TaskEditor");
        case 6:
            return tr("Map", "TaskEditor");
        case 7:
            return tr("Flags", "TaskEditor");
        case 8:
            return tr("Task", "TaskEditor");
        default:
            return QVariant();
        }
    }
    return QVariant();
}

bool TaskModel::insertRows(int position, int rows, const QModelIndex &index,
                           Task *t)
{
    Q_UNUSED(index);
    beginInsertRows(QModelIndex(), position, position + rows - 1);

    for (int row = 0; row < rows; row++)
        tasks.insert(position, t);

    endInsertRows();
    return true;
}

bool TaskModel::removeRows(int position, int rows, const QModelIndex &index)
{
    Q_UNUSED(index);
    beginRemoveRows(QModelIndex(), position, position + rows - 1);

    for (int row = 0; row < rows; ++row)
        delete (tasks.takeAt(position));

    endRemoveRows();
    return true;
}

bool TaskModel::setData(const QModelIndex &index, const QVariant &value,
                        int role)
{
    if (index.isValid() && role == Qt::EditRole) {
        Task *t = tasks.at(index.row());
        if (!t) {
            qWarning() << "TaskModel::setData  no task found";
            return false;
        }

        if (index.column() == 1) // set Delta Priority
        {
            BranchItem *bi = t->getBranch();
            VymModel *m = bi->getModel();
            m->setTaskPriorityDelta(value.toInt(), bi);
            recalcPriorities();
            emit(dataChanged(index, index));
            return true;
        }
        if (index.column() == 8) // set Heading
        {
            BranchItem *bi = t->getBranch();
            VymModel *m = bi->getModel();
            m->setHeadingPlainText(value.toString(), bi);
            emit(dataChanged(index, index));
            return true;
        }
    }

    return false;
}

void TaskModel::emitDataChanged(Task *t)
{
    QModelIndex ix = index(t);
    if (ix.isValid()) {
        int row = ix.row();
        int col = 0;
        while (col < columnCount(QModelIndex())) {
            ix = createIndex(row, col, t);
            if (ix.isValid())
                emit(dataChanged(ix, ix));
            col++;
        }
    }
}

Qt::ItemFlags TaskModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    return QAbstractTableModel::flags(index) | Qt::ItemIsDragEnabled |
           Qt::ItemIsDropEnabled | Qt::ItemIsEditable;
}

int TaskModel::count(VymModel *model) const
{
    if (!model)
        return tasks.size();
    int n = 0;
    foreach (Task *t, tasks)
        if (t->getBranch()->getModel() == model)
            n++;
    return n;
}

Task *TaskModel::createTask(BranchItem *bi)
{
    if (bi) {
        foreach (Task *t, tasks) {
            if (t->getBranch() == bi) {
                qWarning() << "TaskModel::createTask Branch exists already!";
                return nullptr;
            }
        }
        Task *task = new Task(this);
        task->setBranch(bi);
        task->setAwake(Task::Morning);
        insertRows(tasks.count(), 1, QModelIndex(), task);

        bi->setTask(task);

        return task;
    }
    qWarning() << "TaskEditor::addItem - item exists";
    return nullptr;
}

void TaskModel::deleteTask(Task *t)
{
    int pos = tasks.indexOf(t);
    if (pos >= 0)
        removeRows(pos, 1, QModelIndex());
}

bool TaskModel::updateAwake(bool force)
{
    bool awake_changed = false;
    foreach (Task *t, tasks) {
        if (t->updateAwake() || force) {
            t->getBranch()->updateTaskFlag();
            awake_changed = true;
        }
    }
    return awake_changed;
}

void TaskModel::recalcPriorities()
{
    emit(layoutAboutToBeChanged());
    int minPrio = 1000000;
    foreach (Task *t, tasks) {
        int p = 0;
        BranchItem *bi = t->getBranch();

        // Status
        switch (t->getStatus()) {
        case Task::NotStarted:
            break;
        case Task::WIP:
            p += 10;
            break;
        case Task::Finished:
            p += 2000;
            break;
        }

        // Awake and sleeping
        switch (t->getAwake()) {
        case Task::Morning:
            p -= 1000;
            break;
        case Task::WideAwake:
            break;
        case Task::Sleeping:
            //p += 1000 + t->getDaysSleep();
            break;
        }

        // Color (importance)
        QColor c = bi->getHeadingColor();

        // light blueish green
        if (c == QColor("#00aa7f"))
            p -= 20;

        // green (e.g. from vym < 2.6.3 with #005500)
        if (c.red() == 0 && c.blue() == 0 && c.green() < 160)
            p -= 40;

        // orange
        if (c == QColor("#d95100"))
            p -= 60;

        // red
        if (c == QColor("#ff0000"))
            p -= 80;

        // Flags
        if (bi->hasActiveFlag("stopsign"))
            p -= 450;
        if (bi->hasActiveFlag("2arrow-up"))
            p -= 1000;
        if (bi->hasActiveFlag("arrow-up"))
            p -= 500;

        // Age
        p -= t->getAgeModification();
        p -= t->getAgeCreation() * 1.0 / 365 *
             80; // After a year, this is as important as "red"

        // Position in subtree
        p += bi->num();

        // Priority delta (set menually)
        p -= t->getPriorityDelta();

        // Set priority finally
        t->setPriority(p);
        if (p < minPrio)
            minPrio = p;
    }
    // Normalize, so that most important task has prio 1
    foreach (Task *t, tasks) {
        t->setPriority(1 - minPrio + t->getPriority());
    }

    emit(layoutChanged());
}

void TaskModel::setShowParentsLevel(uint i)
{
    showParentsLevel = i;
    recalcPriorities(); // Triggers update of view
}

uint TaskModel::getShowParentsLevel() { return showParentsLevel; }

Qt::DropActions TaskModel::supportedDropActions() const
{
    return Qt::MoveAction;
}

QStringList TaskModel::mimeTypes() const
{
    QStringList types;
    types << "application/vnd.text.list";
    return types;
}

QMimeData *TaskModel::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *mimeData = new QMimeData();
    QByteArray encodedData;

    QDataStream stream(&encodedData, QIODevice::WriteOnly);

    if (indexes.count() > 0 && indexes.first().isValid()) {
        Task *task = getTask(indexes.first());

        // Field 0: Heading
        QString text = task->getBranch()->getHeadingPlain();
        stream << text;

        // Field 1: task row
        stream << QString::number(index(task).row());
    }

    mimeData->setData("application/vnd.text.list", encodedData);
    return mimeData;
}

bool TaskModel::dropMimeData(const QMimeData *data, Qt::DropAction action,
                             int row, int column, const QModelIndex &parent)
{
    Q_UNUSED(row);

    if (action == Qt::IgnoreAction)
        return true;

    if (!data->hasFormat("application/vnd.text.list"))
        return false;

    if (column > 0)
        return false;

    QByteArray encodedData = data->data("application/vnd.text.list");
    QDataStream stream(&encodedData, QIODevice::ReadOnly);
    QStringList newItems;
    int rows = 0;

    while (!stream.atEnd()) {
        QString text;
        stream >> text;
        newItems << text;
        ++rows;
    }

    Task *dst = getTask(parent);
    Task *src = getTask(newItems[1].toInt());

    // qDebug() << "Dropping: " <<  src->getBranch()->getHeadingPlain() << " on
    // " << dst->getBranch()->getHeadingPlain();

    int delta_p = dst->getPriority() - src->getPriority();

    src->setPriorityDelta(src->getPriorityDelta() - delta_p + 1);
    BranchItem *bi = src->getBranch();
    bi->getModel()->emitDataChanged(bi);
    bi->getModel()->setChanged();
    return true;
}
