#include "treeeditor.h"

#include <QAction>
#include <QMenu>
#include <QRegularExpression>

#include "mainwindow.h"
#include "vymmodel.h"

extern Main *mainWindow;
extern QString editorFocusStyle;

extern QMenu *branchContextMenu;
extern QMenu *canvasContextMenu;
extern QMenu *floatimageContextMenu;

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
TreeEditor::TreeEditor(VymModel *m)
{
    model = m;
    if (model)
        setModel(model);
    init();
}

void TreeEditor::init()
{
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    header()->hide();

    QAction *a;
    // Shortcuts for navigating with cursor:
    a = new QAction(tr("Select upper object", "Tree Editor"), this);
    a->setShortcut(Qt::Key_Up);
    a->setShortcutContext(Qt::WidgetShortcut);
    addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(cursorUp()));

    a = new QAction(tr("Select lower object", "Tree Editor"), this);
    a->setShortcut(Qt::Key_Down);
    a->setShortcutContext(Qt::WidgetShortcut);
    addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(cursorDown()));

    a = new QAction(this);
    a->setShortcut(Qt::Key_PageUp);
    a->setShortcutContext(Qt::WidgetShortcut);
    addAction(a);
    connect(a, SIGNAL(triggered()), mainWindow, SLOT(editMoveUp()));

    a = new QAction(this);
    a->setShortcut(Qt::Key_PageDown);
    a->setShortcutContext(Qt::WidgetShortcut);
    addAction(a);
    connect(a, SIGNAL(triggered()), mainWindow, SLOT(editMoveDown()));

    a = new QAction(this);
    a->setShortcut(Qt::Key_Return);
    a->setShortcutContext(Qt::WidgetShortcut);
    addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(startEdit()));

    // Clone actions defined in MainWindow  // FIXME-2 PageUp/Down not working in TreeEditor
    foreach (QAction *qa, mainWindow->mapEditorActions) {
        a = new QAction(this);
        a->setShortcut(qa->shortcut());
        a->setShortcutContext(qa->shortcutContext());
        connect(a, SIGNAL(triggered()), qa, SLOT(trigger()));
        addAction(a);
    }

    setStyleSheet("QTreeView:focus {" + editorFocusStyle + "}");
}

TreeEditor::~TreeEditor()
{
    // qDebug()<<"Destructor TreeEditor for "<<model->getMapName();
}

QModelIndex TreeEditor::getSelectedIndex()
{
    QModelIndexList list = selectionModel()->selectedIndexes();
    if (list.isEmpty())
        return QModelIndex();
    else
        return list.first();
}

void TreeEditor::contextMenuEvent(QContextMenuEvent *e) {
    if (model->getSelectedBranch())
        branchContextMenu->popup(e->globalPos());
    else if (model->getSelectedImage())
        floatimageContextMenu->popup(e->globalPos());
    else if (model->getSelectedXLink())
        model->editXLink();
    else
        canvasContextMenu->exec(e->globalPos());

    e->accept();
}

void TreeEditor::cursorUp()
{
    QModelIndex ix = getSelectedIndex();
    ix = indexAbove(ix);
    if (ix.isValid())
        model->select(ix);
}

void TreeEditor::cursorDown()
{
    QModelIndex ix = getSelectedIndex();
    ix = indexBelow(ix);
    if (ix.isValid())
        model->select(ix);
}

void TreeEditor::startEdit()
{

    QModelIndex ix = getSelectedIndex();
    if (ix.isValid())
        edit(ix);
}
