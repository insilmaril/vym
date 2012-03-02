#include "vymview.h"

#include "branchitem.h"
#include "dockeditor.h"
#include "mainwindow.h"
#include "mapeditor.h"
#include "treedelegate.h"
#include "slideeditor.h"
#include "treeeditor.h"

extern Main *mainWindow;


VymView::VymView(VymModel *m)
{
    model=m;

    // Create TreeView
    treeEditor=new TreeEditor (model);

    selModel=new QItemSelectionModel (model);
    model->setSelectionModel (selModel);

    treeEditor->setSelectionModel (selModel);
    treeEditor->setColumnWidth (0,150);
    treeEditor->setAnimated (true);
    treeEditor->resize ( 20,200);

    TreeDelegate *delegate=new TreeDelegate (this);
    treeEditor->setItemDelegate (delegate);

    DockEditor *de;
    de = new DockEditor (tr("TreeEditor","Title of dockable editor widget"), this, model);
    de->setWidget (treeEditor);
    de->setAllowedAreas (Qt::AllDockWidgetAreas);
    addDockWidget(Qt::LeftDockWidgetArea, de);
    treeEditorDE=de;

    connect (
	treeEditorDE, SIGNAL (visibilityChanged(bool) ), 
	mainWindow,SLOT (updateActions() ) );

    // Create good old MapEditor
    mapEditor=model->getMapEditor();
    if (!mapEditor) mapEditor=new MapEditor (model);
    setCentralWidget (mapEditor);

    // Create SlideEditor
    slideEditor=new SlideEditor (model);

    de = new DockEditor (tr("SlideEditor","Title of dockable editor widget"), this, model);
    de->setWidget (slideEditor);
    de->setAllowedAreas (Qt::AllDockWidgetAreas);
    addDockWidget(Qt::RightDockWidgetArea, de);
    slideEditorDE=de;
    slideEditorDE->hide();
    connect (
	slideEditorDE, SIGNAL (visibilityChanged(bool) ), 
	mainWindow,SLOT (updateActions() ) );

    // Create Layout 
    /*
    QVBoxLayout* mainLayout = new QVBoxLayout (this); 
    QSplitter *splitter= new QSplitter (this);

    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    splitter->setSizePolicy(sizePolicy);
    mainLayout->addWidget (splitter);
    */

    // Connect selections

	// Selection in Model changed	
	connect (
	    selModel, SIGNAL (selectionChanged(const QItemSelection &, const QItemSelection &)), 
	    this,SLOT (changeSelection(const QItemSelection &,const QItemSelection &)));

	// Needed to update selbox during animation 
	connect (
	    model, SIGNAL (selectionChanged(const QItemSelection &, const QItemSelection &)), 
	    mapEditor,SLOT (updateSelection(const QItemSelection &,const QItemSelection &)));

    // Connect data changed signals 
    connect (
	model, SIGNAL (dataChanged(const QModelIndex &, const QModelIndex &)), 
	mapEditor,SLOT (updateData(const QModelIndex &) ) );

    connect (
	model, SIGNAL (updateQueries (VymModel*)), 
	mainWindow,SLOT (updateQueries(VymModel*) ) );

    connect (
	model, SIGNAL (noteHasChanged(QModelIndex) ),
	mainWindow, SLOT (updateNoteEditor (QModelIndex) ) );
	
    connect (
	model, SIGNAL (expandAll() ),
	this, SLOT (expandAll () ) );
	
    connect (
	model, SIGNAL (expandOneLevel() ),
	this, SLOT (expandOneLevel() ) );
	
    connect (
	model, SIGNAL (collapseOneLevel() ),
	this, SLOT (collapseOneLevel() ) );
	
    connect (
	model, SIGNAL (collapseUnselected() ),
	this, SLOT (collapseUnselected() ) );
	
    connect (
	model, SIGNAL (showSelection() ),
	this, SLOT (showSelection() ) );
	
    connect (
	model, SIGNAL (updateLayout() ),
	mapEditor, SLOT (autoLayout() ) );
	
    mapEditor->setAntiAlias (mainWindow->isAliased());
    mapEditor->setSmoothPixmap(mainWindow->hasSmoothPixmapTransform());
}

VymView::~VymView() {}

VymModel* VymView::getModel()
{
    return model;
}

MapEditor* VymView::getMapEditor()
{
    return mapEditor;
}

bool VymView::treeEditorIsVisible()
{
    return treeEditorDE->isVisible();
}

bool VymView::slideEditorIsVisible()
{
    return slideEditorDE->isVisible();
}

void VymView::initFocus()
{
    mapEditor->setFocus();
}

void VymView::changeSelection (const QItemSelection &newsel, const QItemSelection &desel)  
{
    // Update note editor and heading editor // FIXME-3 improve this, evtl. move from mainwindow to here
    model->updateSelection (newsel,desel);
    mainWindow->changeSelection (model,newsel,desel);	
    mapEditor->updateSelection (newsel,desel);
    showSelection();
}

void VymView::expandAll()
{
    treeEditor->expandAll();
}

void VymView::expandOneLevel()
{
    int level=999999;
    int d;
    BranchItem *cur=NULL;
    BranchItem *prev=NULL;
    QModelIndex pix;

    // Find level to expand
    model->nextBranch(cur,prev);
    while (cur) 
    {
	pix=model->index (cur);
	d=cur->depth();
	if (!treeEditor->isExpanded(pix) && d < level)
	    level=d;
	model->nextBranch(cur,prev);	
    }

    // Expand all to level
    cur=NULL;
    prev=NULL;
    model->nextBranch(cur,prev);
    while (cur) 
    {
	pix=model->index (cur);
	d=cur->depth();
	if (!treeEditor->isExpanded(pix) && d <= level && cur->branchCount()>0)
	    treeEditor->setExpanded(pix,true);
	model->nextBranch(cur,prev);	
    }
}

void VymView::collapseOneLevel()
{
    int level=-1;
    int d;
    BranchItem *cur=NULL;
    BranchItem *prev=NULL;
    QModelIndex pix;

    // Find level to collapse
    model->nextBranch(cur,prev);
    while (cur) 
    {
	pix=model->index (cur);
	d=cur->depth();
	if (treeEditor->isExpanded(pix) && d > level)
	    level=d;
	model->nextBranch(cur,prev);	
    }

    // collapse all to level
    cur=NULL;
    prev=NULL;
    model->nextBranch(cur,prev);
    while (cur) 
    {
	pix=model->index (cur);
	d=cur->depth();
	if (treeEditor->isExpanded(pix) && d >= level)
	    treeEditor->setExpanded(pix,false);
	model->nextBranch(cur,prev);	
    }
}

void VymView::collapseUnselected()
{
    BranchItem *cur=NULL;
    BranchItem *prev=NULL;
    QModelIndex pix;

    // Find level to collapse
    TreeItem *selti=model->getSelectedItem();
    if (!selti) return;

    int level=selti->depth();

    // collapse all to level
    model->nextBranch(cur,prev);
    bool b=false;
    while (cur) 
    {
	pix=model->index (cur);
	if (treeEditor->isExpanded(pix) &&  level <= cur->depth())
	{
	    treeEditor->setExpanded(pix,false);
	    b=true;
	}
	model->nextBranch(cur,prev);	
    }

/* FIXME-2 "collapse more" unimplemented yet
    if (b) return;

    // If we didn't collapse anything so far collapse more
    qDebug()<<"VM::collapse more";
    cur=NULL;
    prev=NULL;

    // Find level to collapse
    model->nextBranch(cur,prev);
    while (cur) 
    {
	pix=model->index (cur);
	d=cur->depth();
	if (treeEditor->isExpanded(pix) && d > level && )
	    level=d;
	model->nextBranch(cur,prev);	
    }

    // collapse all to level
    model->nextBranch(cur,prev);
    bool b=false;
    while (cur) 
    {
	pix=model->index (cur);
	if (treeEditor->isExpanded(pix) &&  level <= cur->depth())
	{
	    treeEditor->setExpanded(pix,false);
	    b=true;
	}
	model->nextBranch(cur,prev);	
    }
*/
}

void VymView::showSelection()
{
    QModelIndex ix=model->getSelectedIndex();
    treeEditor->scrollTo( ix, QAbstractItemView::EnsureVisible);
    mapEditor->scrollTo ( ix);	
}

void VymView::toggleTreeEditor()
{
    if (treeEditorDE->isVisible() )
	treeEditorDE->hide();
    else
	treeEditorDE->show();
}

void VymView::toggleSlideEditor()
{
    if (slideEditorDE->isVisible() )
	slideEditorDE->hide();
    else
	slideEditorDE->show();
}


