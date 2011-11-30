#ifndef TASKEDITOR_H 
#define TASKEDITOR_H

#include <QTableView>
#include <QWidget>


class BranchItem;
class QSortFilterProxyModel;
class QTableView;
class TaskModel;

class TaskEditor: public QWidget
{
    Q_OBJECT

public:
    TaskEditor (QWidget *parent=NULL);

private slots:
    void selectionChanged (const QItemSelection & selected, const QItemSelection & );
private:
    QTableView *view;
    QSortFilterProxyModel *proxyModel;
};

#endif

