#include "taskmodel.h"

#include <QDebug>

#include "branchitem.h"
#include "task.h"

TaskModel::TaskModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int TaskModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return tasks.size();
}

int TaskModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 3;
}

QVariant TaskModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.row() >= tasks.size() || index.row() < 0)
        return QVariant();

    if (role == Qt::DisplayRole) 
    {
        if (index.column() == 0)
            return tasks.at(index.row())->getPriority();
        else if (index.column() == 1)
            return tasks.at(index.row())->getStatusString();
        else if (index.column() == 2)
            return tasks.at(index.row())->getName();
    }
    return QVariant();
}

QVariant TaskModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal) {
        switch (section) {
            case 0:
                return tr("Prio","TaskEditor");
            case 1:
                return tr("Status","TaskEditor");
            case 2:
                return tr("Task","TaskEditor");

            default:
                return QVariant();
        }
    }
    return QVariant();
}

bool TaskModel::insertRows(int position, int rows, const QModelIndex &index, Task* t)
{
    Q_UNUSED(index);
    beginInsertRows(QModelIndex(), position, position+rows-1);

    for (int row=0; row < rows; row++) 
        tasks.insert(position, t);

    endInsertRows();
    return true;
}

bool TaskModel::removeRows(int position, int rows, const QModelIndex &index)
{
    Q_UNUSED(index);
    beginRemoveRows(QModelIndex(), position, position+rows-1);

    for (int row=0; row < rows; ++row) {
        tasks.removeAt(position);
    }

    endRemoveRows();
    return true;
}

bool TaskModel::setData(const QModelIndex &index, Task* t, int role)
{
    if (index.isValid() && role == Qt::EditRole) 
    {
        int row = index.row();

        tasks.replace(row, t);
        emit(dataChanged(index, index));

        return true;
    }

    return false;
}

Qt::ItemFlags TaskModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}

Task* TaskModel::createTask (BranchItem *bi)
{
    qDebug()<<"TM::createTask for "<<bi->getHeading();

    if (bi)// FIXME-0 Check for already existing item
    {
	Task *task=new Task();
	task->setBranch (bi);
        insertRows(tasks.count(), 1, QModelIndex(),task);

	recalcPriorities();
	return task;
    }
    qWarning()<<"TaskEditor::addItem - item exists";
    return NULL;
}

void TaskModel::deleteTask (Task* t)
{
    
}

void TaskModel::recalcPriorities()
{
//        QModelIndex index = taskModel->index(0, 0, QModelIndex());
//        table->setData(index, name, Qt::EditRole);

}

