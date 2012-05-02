#ifndef TASKEDITOR_H 
#define TASKEDITOR_H

#include <QWidget>
#include <QTableView>

#include "activetasksfiltermodel.h"

class BranchItem;
class QTableView;
class Task;
class TaskModel;
class QSortFilterProxyModel;

class TaskEditor: public QWidget
{
    Q_OBJECT

public:
    TaskEditor (QWidget *parent=NULL);
    ~TaskEditor ();
    void setMapName (const QString &);
    bool isUsedFilterMap ();
    void setFilterMap  ();
    bool isUsedFilterActive ();
    void setFilterActive ();
    bool select (Task *task);
    void showSelection();
    void contextMenuEvent ( QContextMenuEvent * e );

public slots:
    void sort();

private slots:
    void selectionChanged (const QItemSelection & selected, const QItemSelection & );
    void toggleFilterMap ();
    void toggleFilterActive ();

private:
    QTableView *view;
    QSortFilterProxyModel  *filterMapModel;
    ActiveTasksFilterModel *filterActiveModel;
    QString mapName;
    QAction *actionToggleFilterMap;
    QAction *actionToggleFilterActive;
    bool blockExternalSelect;
};

#endif

