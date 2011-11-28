#ifndef TASKEDITOR_H 
#define TASKEDITOR_H

#include <QWidget>

class BranchItem;
class QTableView;
class TaskModel;

class TaskEditor: public QWidget
{
    Q_OBJECT

public:
    TaskEditor (QWidget *parent=NULL);
private:
    QTableView *view;
};

#endif

