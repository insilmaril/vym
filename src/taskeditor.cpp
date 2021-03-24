#include "taskeditor.h"

#include <QAbstractTableModel>
#include <QAction>
#include <QDebug>
#include <QHeaderView>
#include <QMenu>
#include <QSortFilterProxyModel>
#include <QTextEdit>
#include <QToolBar>
#include <QVBoxLayout>

#include "branchitem.h"
#include "mainwindow.h"
#include "task.h"
#include "taskmodel.h"
#include "vymmodel.h"

extern Main *mainWindow;
extern Settings settings;
extern QMenu *taskContextMenu;
extern TaskModel *taskModel;

extern QString editorFocusStyle;

TaskEditor::TaskEditor(QWidget *)
{
    // Creat Table view
    view = new QTableView;

    QVBoxLayout *mainLayout = new QVBoxLayout;

    QToolBar *tb = new QToolBar("TaskEditor filters");
    tb->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    mainLayout->addWidget(tb);

    // Original icon from KDE:
    // /usr/share/icons/oxygen/16x16/actions/view-filter.png

    QIcon icon = QIcon(":/taskfilter-currentmap.png");
    QAction *a =
        new QAction(icon, "", this); // tr( "Current map","TaskEditor" ),this );
    a->setToolTip(
        tr("Show only tasks from current map", "Filters in task Editor"));
    a->setCheckable(true);
    a->setChecked(settings.value("/taskeditor/filterMap", false).toBool());
    tb->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(toggleFilterMap()));
    actionToggleFilterMap = a;

    icon = QIcon(":/taskfilter-activetask.png");
    a = new QAction(icon, "", this); // tr( "Active tasks","TaskEditor" ),this
                                     // );
    a->setToolTip(tr("Show only active tasks", "Filters in task Editor"));
    a->setCheckable(true);
    a->setChecked(settings.value("/taskeditor/filterActive", false).toBool());
    tb->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(toggleFilterActive()));
    actionToggleFilterActive = a;

    icon = QIcon(":/taskfilter-newtask.png");
    a = new QAction(icon, "", this); // tr( "New tasks","TaskEditor" ),this );
    a->setToolTip(tr("Show only new tasks", "Filters in task Editor"));
    a->setCheckable(true);
    a->setChecked(settings.value("/taskeditor/filterNew", false).toBool());
    tb->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(toggleFilterNew()));
    actionToggleFilterNew = a;

    icon = QIcon(":/flag-arrow-up.svg");
    a = new QAction(icon, "", this);
    a->setToolTip(tr("Show only tasks marked with this arrow-up flag",
                     "Filters in task Editor"));
    a->setCheckable(true);
    a->setChecked(settings.value("/taskeditor/filterFlags1", false).toBool());
    if (settings.value("/mainwindow/showTestMenu", false).toBool())
        tb->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(toggleFilterFlags1()));
    actionToggleFilterFlags1 = a;

    icon = QIcon(":/flag-arrow-2up.svg");
    a = new QAction(icon, "", this);
    a->setToolTip(tr("Show only tasks marked with this arrow-up flag",
                     "Filters in task Editor"));
    a->setCheckable(true);
    a->setChecked(settings.value("/taskeditor/filterFlags2", false).toBool());
    if (settings.value("/mainwindow/showTestMenu", false).toBool())
        tb->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(toggleFilterFlags2()));
    actionToggleFilterFlags2 = a;

    icon = QIcon(":/flag-no-arrow-up.png");
    a = new QAction(icon, "", this);
    a->setToolTip(tr("Show only tasks marked without any arrow-up flag",
                     "Filters in task Editor"));
    a->setCheckable(true);
    a->setChecked(settings.value("/taskeditor/filterFlags3", false).toBool());
    if (settings.value("/mainwindow/showTestMenu", false).toBool())
        tb->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(toggleFilterFlags3()));
    actionToggleFilterFlags3 = a;

    // Clone actions defined in MainWindow
    foreach (QAction *qa, mainWindow->taskEditorActions) {
        a = new QAction(this);
        a->setShortcut(qa->shortcut());
        a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(a, SIGNAL(triggered()), qa, SLOT(trigger()));
        addAction(a);
    }

    mainLayout->addWidget(view);
    setLayout(mainLayout);

    view->setSelectionBehavior(QAbstractItemView::SelectRows);
    view->horizontalHeader()->setStretchLastSection(true);
    view->verticalHeader()->hide();
    // view->setEditTriggers(QAbstractItemView::NoEditTriggers);

    filterActiveModel = new TaskFilterModel;
    filterActiveModel->setSourceModel(taskModel);
    filterActiveModel->setDynamicSortFilter(true);

    view->setModel(filterActiveModel);
    view->setSortingEnabled(true);
    view->setIconSize(QSize(64, 64));

    QHeaderView *hv = view->horizontalHeader();
    hv->setSortIndicator(0, Qt::AscendingOrder);
    view->sortByColumn(hv->sortIndicatorSection(), hv->sortIndicatorOrder());

    view->setSelectionMode(QAbstractItemView::ExtendedSelection);
    view->setDragEnabled(true);
    view->setAcceptDrops(true);
    view->setDropIndicatorShown(true);
    view->setAutoScroll(false);

    blockExternalSelect = false;

    connect(view->selectionModel(),
            SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this,
            SLOT(selectionChanged(QItemSelection, QItemSelection)));

    // Enable wordwrap when data changes
    if (settings.value("/taskeditor/wordWrap", true)
            .toBool()) // FIXME-3 not working or only sometimes?
    {
        connect(taskModel, SIGNAL(dataChanged(QModelIndex, QModelIndex)), view,
                SLOT(resizeRowsToContents()));
        connect(view->horizontalHeader(), SIGNAL(sectionResized(int, int, int)),
                view, SLOT(resizeRowsToContents()));
    }

    // Initialize view filters according to previous settings
    setFilterMap();
    setFilterActive();
    setFilterNew();
    setFilterFlags1();
    setFilterFlags2();
    setFilterFlags3();

    // Initialize display of parents of a task
    bool ok;
    int i = settings.value("/taskeditor/showParentsLevel", 0).toInt(&ok);
    if (ok)
        taskModel->setShowParentsLevel(i);

    view->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(view->horizontalHeader(),
            SIGNAL(customContextMenuRequested(QPoint)),
            SLOT(headerContextMenu()));

    view->setStyleSheet("QTableView:focus {" + editorFocusStyle + "}");

    updateColumnLayout();
}

TaskEditor::~TaskEditor()
{
    settings.setValue("/taskeditor/filterMap",
                      actionToggleFilterMap->isChecked());
    settings.setValue("/taskeditor/filterActive",
                      actionToggleFilterActive->isChecked());
    settings.setValue("/taskeditor/filterNew",
                      actionToggleFilterNew->isChecked());
    settings.setValue("/taskeditor/filterFlags1",
                      actionToggleFilterFlags1->isChecked());
    settings.setValue("/taskeditor/filterFlags2",
                      actionToggleFilterFlags2->isChecked());
    settings.setValue("/taskeditor/filterFlags3",
                      actionToggleFilterFlags3->isChecked());
    settings.setValue("/taskeditor/showParentsLevel",
                      taskModel->getShowParentsLevel());

    for (int i = 0; i <= 8; i++) {
        settings.setValue(QString("/taskeditor/column/%1/width").arg(i),
                          view->columnWidth(i));
        settings.setValue(QString("/taskeditor/column/%1/hidden").arg(i),
                          view->isColumnHidden(i));
    }
}

void TaskEditor::setMapName(const QString &n)
{
    currentMapName = n;

    setFilterMap();
}

bool TaskEditor::isUsedFilterMap()
{
    return actionToggleFilterMap->isChecked();
}

void TaskEditor::setFilterMap()
{
    if (isUsedFilterMap())
        filterActiveModel->setMapFilter(currentMapName);
    else
        filterActiveModel->setMapFilter(QString());
    updateFilters();
}

bool TaskEditor::isUsedFilterActive()
{
    return actionToggleFilterActive->isChecked();
}

void TaskEditor::setFilterActive()
{
    filterActiveModel->setFilter(actionToggleFilterActive->isChecked());
    updateFilters();
}

void TaskEditor::setFilterNew()
{
    filterActiveModel->setFilterNew(actionToggleFilterNew->isChecked());
    updateFilters();
}

void TaskEditor::setFilterFlags1()
{
    filterActiveModel->setFilterFlags1(actionToggleFilterFlags1->isChecked());
    updateFilters();
}

void TaskEditor::setFilterFlags2()
{
    filterActiveModel->setFilterFlags2(actionToggleFilterFlags2->isChecked());
    updateFilters();
}

void TaskEditor::setFilterFlags3()
{
    filterActiveModel->setFilterFlags3(actionToggleFilterFlags3->isChecked());
    updateFilters();
}

void TaskEditor::updateFilters()
{
    filterActiveModel->invalidate();
    filterActiveModel
        ->invalidate(); // ugly, but calling twice updates rows as expected
}

void TaskEditor::showSelection()
{
    QModelIndexList list = view->selectionModel()->selectedIndexes();
    if (list.count() > 0)
        // Usually whole row is selected, so just go for first cell
        view->scrollTo(taskModel->index(taskModel->getTask(list.first())),
                       QAbstractItemView::EnsureVisible);
}

bool TaskEditor::select(Task *task)
{
    if (task) {
        blockExternalSelect = true;
        QModelIndex i0b = taskModel->index(task);
        QModelIndex i0e = taskModel->indexRowEnd(task);

        QModelIndex i1b = filterActiveModel->mapFromSource(i0b);
        QModelIndex i1e = filterActiveModel->mapFromSource(i0e);

        QItemSelection sel(i1b, i1e);

        view->selectionModel()->select(sel,
                                       QItemSelectionModel::ClearAndSelect);
        blockExternalSelect = false;
        return true;
    }
    return false;
}

void TaskEditor::clearSelection() { view->selectionModel()->clearSelection(); }

void TaskEditor::headerContextMenu()
{
    // qDebug() << "TE::headerContextMenu()";
    // Trying to workaround https://bugreports.qt.io/browse/QTBUG-52307
    // view->horizontalHeader()->setStretchLastSection(true);
    // view->resizeColumnsToContents();
    // updateGeometry();
    // show();
}

void TaskEditor::updateColumnLayout()
{
    // Update column widths and visibility

    QString s = "/taskeditor/column/%1/";

    // Priority
    int i = 0;
    view->setColumnWidth(i, settings.value(s.arg(i) + "width", 80).toInt());
    view->setColumnHidden(i,
                          settings.value(s.arg(i) + "hidden", false).toBool());

    // Delta
    i = 1;
    view->setColumnWidth(i, settings.value(s.arg(i) + "width", 50).toInt());
    view->setColumnHidden(i,
                          settings.value(s.arg(i) + "hidden", false).toBool());

    // Status
    i = 2;
    view->setColumnWidth(i, settings.value(s.arg(i) + "width", 50).toInt());
    view->setColumnHidden(i,
                          settings.value(s.arg(i) + "hidden", false).toBool());

    // Age total
    i = 3;
    view->setColumnWidth(i, settings.value(s.arg(i) + "width", 50).toInt());
    view->setColumnHidden(i,
                          settings.value(s.arg(i) + "hidden", true).toBool());

    // Age mod
    i = 4;
    view->setColumnWidth(i, settings.value(s.arg(i) + "width", 50).toInt());
    view->setColumnHidden(i,
                          settings.value(s.arg(i) + "hidden", true).toBool());

    // Sleep
    i = 5;
    view->setColumnWidth(i, settings.value(s.arg(i) + "width", 80).toInt());
    view->setColumnHidden(i,
                          settings.value(s.arg(i) + "hidden", false).toBool());

    // Map
    i = 6;
    view->setColumnWidth(i, settings.value(s.arg(i) + "width", 100).toInt());
    view->setColumnHidden(i,
                          settings.value(s.arg(i) + "hidden", false).toBool());

    // Flags
    i = 7;
    view->setColumnWidth(i, settings.value(s.arg(i) + "width", 80).toInt());
    view->setColumnHidden(i,
                          settings.value(s.arg(i) + "hidden", false).toBool());

    // Taskname
    i = 8;
    view->setColumnWidth(i, settings.value(s.arg(i) + "width", 80).toInt());
    view->setColumnHidden(i,
                          settings.value(s.arg(i) + "hidden", false).toBool());
}

void TaskEditor::selectionChanged(const QItemSelection &selected,
                                  const QItemSelection &)
{
    // FIXME-3 what, if multiple selection in MapEditor?

    if (selected.indexes().isEmpty())
        // Avoid segfault on quit, when selected is empty
        return;

    QItemSelection sel0 = filterActiveModel->mapSelectionToSource(selected);
    QModelIndex ix = sel0.indexes().first();
    Task *t = taskModel->getTask(ix);
    if (t) {
        BranchItem *bi = t->getBranch();
        if (bi) {
            VymModel *m = bi->getModel();
            if (!blockExternalSelect)
                m->select(bi);
            if (m != mainWindow->currentModel())
                mainWindow->gotoModel(m);
            view->setStyleSheet(
                "QTableView {selection-background-color: " +
                m->getSelectionColor().name() +
                "; selection-color:" + bi->getHeadingColor().name() + "}" +
                "QTableView:focus {" + editorFocusStyle + "}");
            view->scrollTo(selected.indexes().first());
        }
    }
}

void TaskEditor::contextMenuEvent(QContextMenuEvent *e)
{
    taskContextMenu->popup(e->globalPos());
}

void TaskEditor::toggleFilterMap() { setFilterMap(); }

void TaskEditor::toggleFilterActive() { setFilterActive(); }
void TaskEditor::toggleFilterNew() { setFilterNew(); }

void TaskEditor::toggleFilterFlags1() { setFilterFlags1(); }
void TaskEditor::toggleFilterFlags2() { setFilterFlags2(); }
void TaskEditor::toggleFilterFlags3() { setFilterFlags3(); }
