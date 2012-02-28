#ifndef TASKEDITOR_H 
#define TASKEDITOR_H

#include <QWidget>
#include <QTableView>


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
    void sort();
    void setMapName (const QString &);
    bool isUsedFilterMap ();
    void setFilterMap  ();
    bool isUsedFilterSleeping ();
    void setFilterSleeping (bool);
    bool select (Task *task);
    void showSelection();
    void contextMenuEvent ( QContextMenuEvent * e );

private slots:
    void selectionChanged (const QItemSelection & selected, const QItemSelection & );
    void toggleFilterMap ();
    void toggleFilterSleeping ();

private:
    QTableView *view;
    QSortFilterProxyModel *filterMapModel;
    QSortFilterProxyModel *filterSleepingModel;
    QString mapName;
    QAction *actionToggleFilterMap;
    QAction *actionToggleFilterSleeping;
    bool blockExternalSelect;
};

#endif

