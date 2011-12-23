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
    void sort();
    bool select (Task *task);
    void contextMenuEvent ( QContextMenuEvent * e );

private slots:
    void selectionChanged (const QItemSelection & selected, const QItemSelection & );

private:
    QTableView *view;
    QSortFilterProxyModel *proxyModel;
};

#endif

