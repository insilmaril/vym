#ifndef TASKEDITOR_H
#define TASKEDITOR_H

#include <QTableView>
#include <QWidget>

#include "taskfiltermodel.h"

class BranchItem;
class QTableView;
class Task;
class TaskModel;
class QSortFilterProxyModel;

class TaskEditor : public QWidget {
    Q_OBJECT

  public:
    TaskEditor(QWidget *parent = NULL);
    ~TaskEditor();
    void setMapName(const QString &);
    bool isUsedFilterMap();
    void setFilterMap();
    bool isUsedFilterActive();
    void setFilterActive();
    void setFilterNew();
    void setFilterFlags1();
    void setFilterFlags2();
    void setFilterFlags3();
    void updateFilters();
    bool select(Task *task);
    void clearSelection();
    void showSelection();
    void contextMenuEvent(QContextMenuEvent *e);

  private slots:
    void headerContextMenu();
    void updateColumnLayout();
    void selectionChanged(const QItemSelection &selected,
                          const QItemSelection &);
    void toggleFilterMap();
    void toggleFilterActive();
    void toggleFilterNew();
    void toggleFilterFlags1();
    void toggleFilterFlags2();
    void toggleFilterFlags3();

  private:
    QTableView *view;
    TaskFilterModel *filterActiveModel;
    QString currentMapName;
    QAction *actionToggleFilterMap;
    QAction *actionToggleFilterActive;
    QAction *actionToggleFilterNew;
    QAction *actionToggleFilterFlags1;
    QAction *actionToggleFilterFlags2;
    QAction *actionToggleFilterFlags3;
    bool blockExternalSelect;
};

#endif
