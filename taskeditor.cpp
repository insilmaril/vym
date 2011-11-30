#include "taskeditor.h"

#include <QAbstractTableModel>
#include <QDebug>
#include <QHeaderView>
#include <QSortFilterProxyModel>
#include <QVBoxLayout>

#include "branchitem.h"
#include "mainwindow.h"
#include "task.h"
#include "taskmodel.h"
#include "vymmodel.h"

extern TaskModel* taskModel;
extern Main *mainWindow;

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
    proxyModel->sort(2, Qt::DescendingOrder);

    view->setModel (proxyModel);
    view->setSelectionBehavior(QAbstractItemView::SelectRows);
    view->setSelectionMode(QAbstractItemView::SingleSelection);
    view->horizontalHeader()->setStretchLastSection(true);
    view->verticalHeader()->hide();
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    view->setSortingEnabled(true);

    connect (
	view->selectionModel(),SIGNAL (selectionChanged (QItemSelection,QItemSelection)),
	this, SLOT (selectionChanged (QItemSelection,QItemSelection)));


}


void TaskEditor::selectionChanged ( const QItemSelection & selected, const QItemSelection & )
{
    QModelIndex ix;
    QModelIndex ix_org;
    foreach (ix,selected.indexes() )
    {
	ix_org= proxyModel->mapToSource(ix);
	Task *t=taskModel->getTask (ix_org);
	if (t) 
	{
	    BranchItem *bi=t->getBranch();
	    if (bi) 
	    {
		VymModel *m=bi->getModel();
		m->select (bi);
		if (m!=mainWindow->currentModel() )
		    mainWindow->gotoModel (m);
	    }
	}
    }
}



