#include "taskeditor.h"

#include <QDebug>
#include <QTableView>
#include <QVBoxLayout>

#include "branchitem.h"
#include "task.h"
#include "taskmodel.h"

extern TaskModel* taskModel;

//extern QString iconPath;

TaskEditor::TaskEditor(QWidget *)
{
    // Create Table view
    view = new QTableView (this);
    view->setModel (taskModel);

    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->addWidget (view);
    setLayout (mainLayout);
}

