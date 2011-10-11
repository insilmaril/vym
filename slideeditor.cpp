#include <QVBoxLayout>
#include <QTreeView>

#include "slidecontrolwidget.h"
#include "slidemodel.h"
#include "slideitem.h"
#include "vymmodel.h"

#include "slideeditor.h"

extern QString iconPath;

SlideEditor::SlideEditor(VymModel *m)
{
    model=m;

    // Create slides model
    slideModel=new SlideModel;

    // Create TreeView
    view = new QTreeView (this);
    view->setModel (slideModel);

    // Create ControlWidget
    slideControl= new SlideControlWidget (this);
    connect (
	slideControl, SIGNAL (takeSnapshot() ), 
	this, SLOT (takeSnapshot () ) );
    connect (
	slideControl, SIGNAL (previousButtonPressed() ), 
	this, SLOT (previousSlide() ) );
    connect (
	slideControl, SIGNAL (nextButtonPressed() ), 
	this, SLOT (nextSlide() ) );

    QVBoxLayout* mainLayout = new QVBoxLayout;

    /* FIXME-3 testing QMenuBar *mb=new QMenuBar;
    QAction *a=new  QAction ("Foo action",NULL);
    mb->addAction (a);
    mb->insertSeparator();
    mainLayout->addWidget(mb);
    */
    mainLayout->addWidget(view);
    mainLayout->addWidget(slideControl);

    setLayout (mainLayout);

    // Selection
    connect (view->selectionModel(),SIGNAL (selectionChanged (QItemSelection,QItemSelection)),
	this, SLOT (updateSelection (QItemSelection,QItemSelection)));

//    connect (resultsModel, SIGNAL(layoutChanged() ), view, SLOT (expandAll() ));    
}

QModelIndex SlideEditor::getSelectedIndex()
{
    QModelIndexList list=view->selectionModel()->selectedIndexes();
    if (list.isEmpty() )
	return QModelIndex();
    else
	return list.first();
}

void SlideEditor::addItem (const QString &s)
{
    if (!s.isEmpty())
    {
	QModelIndex index = view->selectionModel()->currentIndex();
	
	if (!slideModel->insertRow(index.row()+1, index.parent()))
	    return;

	for (int column = 0; column < slideModel->columnCount(index.parent()); ++column) {
	    QModelIndex child = slideModel->index(index.row()+1, column, index.parent());
	    slideModel->setData(child, QVariant(s), Qt::EditRole);
	}
    }
}

/*
void SlideEditor::popup()
{
    show();
    parentWidget()->show();
 //   findWidget->setFocus();
}
*/

void SlideEditor::previousSlide()
{
    QModelIndex ix=getSelectedIndex();
    ix=view->indexAbove (ix);
    if (ix.isValid())
    view->selectionModel()->select (ix,QItemSelectionModel::ClearAndSelect );
}

void SlideEditor::nextSlide()
{
    QModelIndex ix=getSelectedIndex();
    ix=view->indexBelow (ix);
    if (ix.isValid())
    view->selectionModel()->select (ix,QItemSelectionModel::ClearAndSelect );
}

void SlideEditor::takeSnapshot()
{
    SlideItem *si=slideModel->addItem (model->getHeading() );
    si->setTreeItem (model->getSelectedItem() );
    si->setZoomFactor   (model->getMapEditor()->getZoomFactorTarget() );
    si->setRotationAngle (model->getMapEditor()->getAngleTarget() );
}

void SlideEditor::updateSelection(QItemSelection newsel,QItemSelection)
{
    QModelIndex ix;
    foreach (ix,newsel.indexes() )
    {
	SlideItem *fri= static_cast<SlideItem*>(ix.internalPointer());
	int id=fri->getTreeItemID();
	if (id>0)
	{
	    TreeItem *ti=model->findID(id);
	    if (ti)
	    {
		model->select (ti);
		MapEditor *me=model->getMapEditor();
		if (me)
		{
		    qreal q=fri->getZoomFactor();
		    if (q>0) me->setZoomFactorTarget (q); 
		    q=fri->getRotationAngle();
		    me->setAngleTarget (q);
		}
	    }	
	}
    }
}

