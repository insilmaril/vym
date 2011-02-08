#include "treeeditor.h"

#include <QAction>
#include <QRegExp>

#include "mainwindow.h"
#include "vymmodel.h"

extern Main *mainWindow;

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
TreeEditor::TreeEditor(VymModel *m)
{
    model=m;

    setModel(m);

    QAction *a;
    // Shortcuts for navigating with cursor:
    a = new QAction(tr( "Select upper object","Tree Editor" ), this);
    a->setShortcut (Qt::Key_Up );
    a->setShortcutContext (Qt::WidgetShortcut);
    addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( cursorUp() ) );

    a = new QAction( tr( "Select lower object","Tree Editor" ),this);
    a->setShortcut ( Qt::Key_Down );
    a->setShortcutContext (Qt::WidgetShortcut);
    addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( cursorDown() ) );

    a = new QAction( this);
    a->setShortcut ( Qt::Key_PageUp);
    a->setShortcutContext (Qt::WidgetShortcut);
    addAction (a);
    connect( a, SIGNAL( triggered() ), mainWindow, SLOT( editMoveUp() ) );

    a = new QAction( this);
    a->setShortcut ( Qt::Key_PageDown );
    a->setShortcutContext (Qt::WidgetShortcut);
    addAction (a);
    connect( a, SIGNAL( triggered() ), mainWindow, SLOT( editMoveDown() ) );
}

TreeEditor::~TreeEditor()
{
    //cout <<"Destructor TreeEditor for "<<model->getMapName().toStdString()<<endl;
}

QModelIndex TreeEditor::getSelectedIndex()
{
    QModelIndexList list=selectionModel()->selectedIndexes();
    if (list.isEmpty() )
	return QModelIndex();
    else
	return list.first();
}

void TreeEditor::cursorUp()
{
    QModelIndex ix=getSelectedIndex();
    ix=indexAbove (ix);
    if (ix.isValid())
	model->select (ix );
}

void TreeEditor::cursorDown()
{
    QModelIndex ix=getSelectedIndex();
    ix=indexBelow (ix);
    if (ix.isValid())
	model->select (ix );
}

