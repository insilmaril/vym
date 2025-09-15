#include "findresulttreeview.h"

#include <QAction>
#include <QContextMenuEvent>
#include <QHeaderView>
#include <QMenu>

extern QString editorFocusInStyle;
extern QString editorFocusOutStyle;

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
FindResultTreeView::FindResultTreeView()
{
    init();
}

void FindResultTreeView::init()
{
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    header()->hide();

    QAction *a;

    a = new QAction(this);
    a->setShortcut(Qt::Key_Return);
    a->setShortcutContext(Qt::WidgetShortcut);
    addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(startEdit()));


    // FIXME-2 Set borders when in focus to editorFocusStyle
}

FindResultTreeView::~FindResultTreeView()
{
    // qDebug()<<"Destructor FindResultTreeView for "<<model->getMapName();
}

QModelIndex FindResultTreeView::getSelectedIndex()
{
    QModelIndexList list = selectionModel()->selectedIndexes();
    if (list.isEmpty())
        return QModelIndex();
    else
        return list.first();
}

void FindResultTreeView::contextMenuEvent(QContextMenuEvent *e) {
    /*
    if (model->getSelectedBranch())
        branchContextMenu->popup(e->globalPos());
    else if (model->getSelectedImage())
        floatimageContextMenu->popup(e->globalPos());
    else if (model->getSelectedXLink())
        model->editXLink();
    else
        canvasContextMenu->exec(e->globalPos());

    e->accept();
    */
}

void FindResultTreeView::closeEvent(QCloseEvent *event)
{
    qDebug() << "FRTV::close";
    //mainWindow->windowSetFindResultTreeViewsVisibility(false);
}

void FindResultTreeView::startEdit()
{
    QModelIndex ix = getSelectedIndex();
    emit searchFinished();
}
