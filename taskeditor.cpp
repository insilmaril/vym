#include "taskeditor.h"

#include <QAbstractTableModel>
#include <QDebug>
#include <QHeaderView>
#include <QSortFilterProxyModel>
#include <QVBoxLayout>

#include "branchitem.h"
#include "task.h"
#include "taskmodel.h"

extern TaskModel* taskModel;

//extern QString iconPath;

TaskEditor::TaskEditor(QWidget *)
{
    // Create Table view
    view = new QTableView; 

    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->addWidget (view);
    setLayout (mainLayout);

    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(taskModel);
    proxyModel->setSortCaseSensitivity (Qt::CaseInsensitive);
    proxyModel->setDynamicSortFilter (true);
    proxyModel->sort(0, Qt::AscendingOrder);

    view->setModel (proxyModel);
    view->setSelectionBehavior(QAbstractItemView::SelectRows);
    view->horizontalHeader()->setStretchLastSection(true);
    view->verticalHeader()->hide();
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    view->setSelectionMode(QAbstractItemView::SingleSelection);
    view->setSortingEnabled(true);

}

