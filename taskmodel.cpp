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

QModelIndex TaskModel::indexRowEnd (Task* t)
{
    int n=tasks.indexOf (t);
    if (n<0)
	return QModelIndex();
    else    
	return createIndex (n,6,t);
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
    return 7;
}

QVariant TaskModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    
    if (index.row() >= tasks.size() || index.row() < 0)
        return QVariant();

    BranchItem *bi=tasks.at(index.row())->getBranch();

    if (role == Qt::DisplayRole) 
    {
        if (index.column() == 0)
            return tasks.at(index.row())->getPriority();
        else if (index.column() == 1)
            return tasks.at(index.row())->getStatusString();
        else if (index.column() == 2)
	    return tasks.at(index.row())->getAgeCreation();
        else if (index.column() == 3)
	    return tasks.at(index.row())->getAgeModified();
        else if (index.column() == 4)
	    return tasks.at(index.row())->getDaysSleep();
        else if (index.column() == 5)
	{
	    if (bi) return bi->getModel()->getMapName();
            return "?";	// Should never happen
	}
        else if (index.column() == 6)
            return tasks.at(index.row())->getName();
    }
    else // role != Qt::DisplayRole
    {
	if (role == Qt::ForegroundRole && bi ) 
	    return bi->getHeadingColor();
	if (role == Qt::BackgroundRole && bi ) 
	    return bi->getModel()->getMapBackgroundColor();
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
                return tr("Age total","TaskEditor");
            case 3:
                return tr("Age mod.","TaskEditor");
            case 4:
                return tr("Sleep","TaskEditor");
            case 5:
                return tr("Map","TaskEditor");
            case 6:
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
    if (bi)
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

void TaskModel::recalcPriorities() 
{
    int minPrio=1000000;
    foreach (Task *t,tasks)
    {   
	int p=0;
	BranchItem *bi=t->getBranch();

	// Status
	switch (t->getStatus() )
	{
	    case Task::Finished: p+=2000; break;
	    case Task::WIP: p+=100; break;
	    case Task::NotStarted: break;
	}

	// Color (importance)
	QColor c=bi->getHeadingColor();
	if (c==QColor ("#ff0000") ) p-=40;
	if (c==QColor ("#d95100") ) p-=30;
	if (c==QColor ("#005500") ) p-=20;
	if (c==QColor ("#00aa7f") ) p-=10;

	// Stopsign
	if (bi->hasActiveStandardFlag ("stopsign") ) p-=100;

	// Age
	p-=t->getAgeModified();

	// Sleeping?
	if (t->getDaysSleep() >0)
	    p+=1000 + 5*t->getDaysSleep();

	t->setPriority (p);
	if (p<minPrio) minPrio=p;
    }
    // Normalize, so that most important task has prio 1
    foreach (Task *t,tasks)
    {   
	t->setPriority (1-  minPrio + t->getPriority() );
	emitDataHasChanged (t);
    }
}

