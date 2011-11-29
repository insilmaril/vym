#include "taskmodel.h"

#include <QDebug>

#include "branchitem.h"
#include "task.h"
#include "vymmodel.h"

TaskModel::TaskModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

QModelIndex TaskModel::index (Task* t)
{
    int n=tasks.indexOf (t);
    if (n<0)
	return QModelIndex();
    else    
	return createIndex (n,0,t);
}

Task* TaskModel::getTask (const QModelIndex &ix) const
{
    if (ix.isValid() )
	return tasks.at (ix.row() );
    else
	return NULL;
}

int TaskModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return tasks.size();
}

int TaskModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 4;
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
        else if (index.column() == 3)
	{
	    BranchItem *bi=tasks.at(index.row())->getBranch();
	    if (bi) return bi->getModel()->getMapName();
            return "?";	// Should never happen
	}
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
            case 3:
                return tr("Map","TaskEditor");

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

    for (int row=0; row < rows; ++row) 
        delete (tasks.takeAt(position)) ;

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

void TaskModel::emitDataHasChanged (Task* t)
{
    QModelIndex ix=index (t);
    if (ix.isValid() )
    {
	int row=ix.row();
	int col=0;
	while (col<columnCount(QModelIndex() ) )
	{
	    ix=createIndex (row, col, t);
	    if (ix.isValid() ) emit(dataChanged(ix,ix) );
	    col++;  
	}    
    }
}

Qt::ItemFlags TaskModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
}

Task* TaskModel::createTask (BranchItem *bi)
{
    if (bi)// FIXME-0 Check for already existing item
    {
	foreach (Task* t, tasks)
	{
	    if (t->getBranch()==bi)
	    {
		qWarning()<<"TaskModel::createTask Branch exists already!";
		return NULL;
	    }
	}
	Task* task=new Task(this);
	task->setBranch (bi);
        insertRows(tasks.count(), 1, QModelIndex(),task);

	bi->setTask (task);

	recalcPriorities();
	return task;
    }
    qWarning()<<"TaskEditor::addItem - item exists";
    return NULL;
}

void TaskModel::deleteTask (Task* t)
{
    int pos=tasks.indexOf(t);
    if (t>=0)
	removeRows(pos, 1,QModelIndex() );
}

void TaskModel::recalcPriorities()  //FIXME-1 not implemented yet
{
    foreach (Task *t,tasks)
    {   
	int p=0;
	p+=(t->getStatus())*100;
	p+=tasks.indexOf(t);
	t->setPriority (p);
	emitDataHasChanged (t);
    }
}

