#ifndef TASKMODEL_H
#define TASKMODEL_H

#include <QAbstractTableModel>
#include <QIcon>
#include <QList>

#include "task.h"

class BranchItem;
class VymModel;

class TaskModel : public QAbstractTableModel {
    Q_OBJECT

  public:
    TaskModel(QObject *parent = 0);
    QModelIndex index(Task *t) const;
    QModelIndex indexRowEnd(Task *t);
    Task *getTask(const QModelIndex &ix) const;
    Task *getTask(const int i) const;
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole);
    //    bool setData(const QModelIndex &index, Task *t, int
    //    role=Qt::EditRole);
    void emitDataChanged(Task *t);
    bool insertRows(int position, int rows,
                    const QModelIndex &index = QModelIndex(), Task *t = nullptr);
    bool removeRows(int position, int rows,
                    const QModelIndex &index = QModelIndex());

    int count(VymModel *model = nullptr) const;
    Task *createTask(BranchItem *bi);
    void deleteTask(Task *t);
    bool updateAwake(bool force = false);
    void recalcPriorities();

    void setShowParentsLevel(uint i);
    uint getShowParentsLevel();

    // Drag and drop support
    Qt::DropActions supportedDropActions() const;
    QStringList mimeTypes() const;
    QMimeData *mimeData(const QModelIndexList &indexes) const;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row,
                      int column, const QModelIndex &parent);

  private:
    QList<Task *> tasks;
    uint showParentsLevel;

    QIcon arrow_up_icon;
    QIcon arrow_2up_icon;
    QIcon task_new_icon;
    QIcon task_new_morning_icon;
    QIcon task_new_sleeping_icon;
    QIcon task_wip_icon;

    QIcon task_wip_morning_icon;
    QIcon task_wip_sleeping_icon;
    QIcon task_finished_icon;

    QIcon taskfilter_stopsign_icon;
    QIcon taskfilter_stopsign_arrow_up_icon;
    QIcon taskfilter_stopsign_arrow_2up_icon;
};

#endif
