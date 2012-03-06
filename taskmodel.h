#ifndef TASKMODEL_H
#define TASKMODEL_H

#include <QAbstractTableModel>
#include <QList>

#include "task.h"

class BranchItem;
class VymModel;

class TaskModel : public QAbstractTableModel
{
    Q_OBJECT
    
public:
    TaskModel(QObject *parent=0);
    QModelIndex index (Task* t);
    QModelIndex indexRowEnd (Task* t);
    Task* getTask (const QModelIndex &ix) const;
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
//    bool setData(const QModelIndex &index, const QVariant &value, int role=Qt::EditRole);
    bool setData(const QModelIndex &index, Task *t, int role=Qt::EditRole);
    void emitDataChanged (Task *t);
    bool insertRows(int position, int rows, const QModelIndex &index=QModelIndex(),Task *t=NULL);
    bool removeRows(int position, int rows, const QModelIndex &index=QModelIndex());

    int count (VymModel *model=NULL);
    Task* createTask (BranchItem *bi);
    void deleteTask (Task* t);
    void recalcPriorities();

private:
    QList <Task*> tasks;
 };

#endif
