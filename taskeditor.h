#ifndef TASKEDITOR_H 
#define TASKEDITOR_H

#include <QWidget>
#include <QTableView>

#include "taskfiltermodel.h"

class BranchItem;
class QTableView;
class Task;
class TaskModel;
class QSortFilterProxyModel;

class TaskEditor : public QWidget
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
    void setFilterNew ();
    bool select (Task *task);
    void clearSelection ();
    void showSelection ();
    void contextMenuEvent ( QContextMenuEvent * e );

public slots:
    void sort();

private slots:
    void selectionChanged (const QItemSelection & selected, const QItemSelection & );
    void toggleFilterMap ();
    void toggleFilterActive ();
    void toggleFilterNew ();

private:
    QTableView *view;
    TaskFilterModel *filterActiveModel;
    QString currentMapName;
    QAction *actionToggleFilterMap;
    QAction *actionToggleFilterActive;
    QAction *actionToggleFilterNew;
    bool blockExternalSelect;
};

#endif

