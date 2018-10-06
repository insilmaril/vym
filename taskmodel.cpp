#include "taskmodel.h"

#include <QDebug>

#include "branchitem.h"
#include "branchobj.h"
#include "task.h"
#include "vymmodel.h"

TaskModel::TaskModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    showParentsLevel = 0;
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
    Task *t=tasks.at(index.row() );

    if (role == Qt::DisplayRole) 
    {
        if (index.column() == 0)
            return t->getPriority();
        else if (index.column() == 1)
            return t->getStatusString() + " - " +t->getAwakeString();
        else if (index.column() == 2)
            return t->getAgeCreation();
        else if (index.column() == 3)
            return t->getAgeModified();
        else if (index.column() == 4)
            return t->getDaysSleep();
        else if (index.column() == 5)
        {
            QString s = bi->getModel()->getMapName();
            if (s.isEmpty() )
                return "-";    
            else
                return bi->getModel()->getMapName();
        }
        else if (index.column() == 6)
        {
            BranchItem *bi = tasks.at(index.row())->getBranch();
            return bi->getHeadingPlainWithParents( showParentsLevel );
        }
    } else if (role == Qt::DecorationRole && index.column() == 1)
    {
        return QIcon(":/flag-" + t->getIconString() + ".png");
    }
    else // role != Qt::DisplayRole
    {
	if (role == Qt::ForegroundRole && bi ) 
	    return bi->getHeadingColor();
	if (role == Qt::BackgroundRole && bi ) 
        {
            BranchItem *frameBI=bi->getFramedParentBranch(bi);
            if (frameBI && index.column() != 5)
            {
                BranchObj *bo=frameBI->getBranchObj();
                if (bo) 
                    // Return frame background
                    return bo->getFrameBrushColor();
            }
            else
            {
                // Return map background
                return bi->getModel()->getMapBackgroundColor();
            }
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

void TaskModel::emitDataChanged (Task* t)
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

int TaskModel::count (VymModel *model)
{
    if (!model) return tasks.size();
    int n=0;
    foreach (Task *t,tasks) if (t->getBranch()->getModel()==model) n++;
    return n;
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
        task->setAwake(Task::Morning);
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
    if (pos>=0)
	removeRows(pos, 1,QModelIndex() );
}

bool TaskModel::updateAwake(bool force)
{
    bool awake_changed = false;
    foreach (Task *t,tasks)
    {
        if (t->updateAwake() || force)
        {
            t->getBranch()->updateTaskFlag(); 
            awake_changed = true;
        }
    }
    return awake_changed;
}

void TaskModel::recalcPriorities() 
{
    emit (layoutAboutToBeChanged() );
    int minPrio=1000000;
    foreach (Task *t,tasks)
    {   
	int p=0;
	BranchItem *bi=t->getBranch();

	// Status
	switch (t->getStatus() )
	{
	    case Task::NotStarted: break;
	    case Task::WIP: p+=10; break;
	    case Task::Finished: p+=2000; break;
	}

	// Awake and sleeping
	switch (t->getAwake() )
	{
	    case Task::Morning: p-=1000; break;
	    case Task::WideAwake: break;
	    case Task::Sleeping: p+=1000 + t->getDaysSleep(); break;
	}

	// Color (importance)
	QColor c = bi->getHeadingColor();

        // light blueish green
	if (c == QColor ("#00aa7f") ) p -= 20;

        // green (e.g. from vym < 2.6.3 with #005500)
	if (c.red() == 0 && c.blue() == 0 && c.green() < 160) p -= 40;

        // orange
	if (c == QColor ("#d95100") ) p -= 60;

        // red
	if (c == QColor ("#ff0000") ) p -= 80;

	// Flags
	if (bi->hasActiveStandardFlag ("stopsign") ) p-=800;

	// Age
	p-=t->getAgeModified();
	p-=t->getAgeCreation() * 1.0 / 365 * 80; // After a year, this is as important as "red"

        // Position in subtree
        p += bi->num();

	t->setPriority (p);
	if (p<minPrio) minPrio=p;
    }
    // Normalize, so that most important task has prio 1
    foreach (Task *t,tasks)
    {   
	t->setPriority (1-  minPrio + t->getPriority() );
    }

    emit (layoutChanged() );
}

void TaskModel::setShowParentsLevel(uint i)
{
    showParentsLevel = i;
    recalcPriorities(); // Triggers update of view
}

uint TaskModel::getShowParentsLevel()
{
    return showParentsLevel;
}

