#include "vymview.h"

#include "branchitem.h"
#include "dockeditor.h"
#include "mainwindow.h"
#include "mapeditor.h"
#include "slideeditor.h"
#include "treedelegate.h"
#include "treeeditor.h"

extern Main *mainWindow;
extern Settings settings;

VymView::VymView(VymModel *m)
{
    model = m;
    model->setView(this);

    // Create TreeView
    treeEditor = new TreeEditor(model);

    selModel = new QItemSelectionModel(model);
    model->setSelectionModel(selModel);

    treeEditor->setSelectionModel(selModel);
    treeEditor->setColumnWidth(0, 150);
    treeEditor->setAnimated(true);
    treeEditor->resize(20, 200);

    TreeDelegate *delegate = new TreeDelegate(this);
    treeEditor->setItemDelegate(delegate);

    DockEditor *de;
    de = new DockEditor(tr("Tree Editor", "Title of dockable editor widget"),
                        this, model);
    de->setWidget(treeEditor);
    de->setAllowedAreas(Qt::AllDockWidgetAreas);
    addDockWidget(Qt::LeftDockWidgetArea, de);
    treeEditorDE = de;

    connect(treeEditorDE, SIGNAL(visibilityChanged(bool)), mainWindow,
            SLOT(updateActions()));

    // Create good old MapEditor
    mapEditor = model->getMapEditor();
    if (!mapEditor)
        mapEditor = new MapEditor(model);
    setCentralWidget(mapEditor);

    // Create SlideEditor
    slideEditor = new SlideEditor(model);

    de = new DockEditor(tr("Slide Editor", "Title of dockable editor widget"),
                        this, model);
    de->setWidget(slideEditor);
    de->setAllowedAreas(Qt::AllDockWidgetAreas);
    addDockWidget(Qt::RightDockWidgetArea, de);
    slideEditorDE = de;
    slideEditorDE->hide();
    connect(slideEditorDE, SIGNAL(visibilityChanged(bool)), mainWindow,
            SLOT(updateActions()));

    // Connect selections

    // Selection in Model changed
    connect(
        selModel,
        SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
        this,
        SLOT(changeSelection(const QItemSelection &, const QItemSelection &)));

    // Needed to update selbox during animation
    connect(
        model,
        SIGNAL(
            selectionChanged(const QItemSelection &, const QItemSelection &)),
        mapEditor,
        SLOT(updateSelection(const QItemSelection &, const QItemSelection &)));

    // Connect data changed signals
    connect(model,
            SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
            mapEditor, SLOT(updateData(const QModelIndex &)));

    connect(model,
            SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)), this,
            SLOT(updateDockWidgetTitles())); // FIXME-3 connect directly to
                                             // MainWindow and rename method
                                             // (also updates selection in BPE)

    connect(model, SIGNAL(updateQueries(VymModel *)), mainWindow,
            SLOT(updateQueries(VymModel *)));

    connect(model, SIGNAL(expandAll()), this, SLOT(expandAll()));

    connect(model, SIGNAL(expandOneLevel()), this, SLOT(expandOneLevel()));

    connect(model, SIGNAL(collapseOneLevel()), this, SLOT(collapseOneLevel()));

    connect(model, SIGNAL(collapseUnselected()), this,
            SLOT(collapseUnselected()));

    connect(model, SIGNAL(showSelection()), this, SLOT(showSelection()));

    connect(model, SIGNAL(updateLayout()), mapEditor, SLOT(autoLayout()));

    mapEditor->setAntiAlias(mainWindow->isAliased());
    mapEditor->setSmoothPixmap(mainWindow->hasSmoothPixmapTransform());

    readSettings();
}

VymView::~VymView()
{
    settings.setLocalValue(model->getFilePath(), "/treeEditor/visible",
                               treeEditorIsVisible());
    settings.setLocalValue(model->getFilePath(), "/slideEditor/visible",
                               slideEditorIsVisible());
}

void VymView::readSettings()
{
    if (settings
            .localValue(model->getFilePath(), "/slideEditor/visible", "false")
            .toBool())
        slideEditorDE->show();
    else
        slideEditorDE->hide();

    if (settings.localValue(model->getFilePath(), "/treeEditor/visible", "true")
            .toBool())
        treeEditorDE->show();
    else
        treeEditorDE->hide();
}

VymModel *VymView::getModel() { return model; }

MapEditor *VymView::getMapEditor() { return mapEditor; }

bool VymView::treeEditorIsVisible() { return treeEditorDE->isVisible(); }

bool VymView::slideEditorIsVisible() { return slideEditorDE->isVisible(); }

void VymView::initFocus() { mapEditor->setFocus(); }

void VymView::nextSlide() { slideEditor->nextSlide(); }

void VymView::previousSlide() { slideEditor->previousSlide(); }

void VymView::setSelectionColor(const QColor &col)
{
    mapEditor->setSelectionColor(col);
    treeEditor->setStyleSheet(
        "selection-background-color: " + col.name() + ";" +
        "background-color: " + mapEditor->getScene()->backgroundBrush().color().name());
}

void VymView::setBackgroundColor(const QColor &col)
{
    mapEditor->getScene()->setBackgroundBrush(col);
    treeEditor->setStyleSheet(
        "selection-background-color: " + mapEditor->getSelectionColor().name() + ";" +
        "background-color: " + col.name());
}

void VymView::changeSelection(const QItemSelection &newsel,
                              const QItemSelection &desel)
{
    // Update note editor and heading editor // FIXME-3 improve this, evtl. move
    // from mainwindow to here
    model->updateSelection(newsel, desel);
    mainWindow->changeSelection(model, newsel, desel);
    mainWindow->updateDockWidgetTitles(model);
    mapEditor->updateSelection(newsel, desel);

    showSelection();
}

void VymView::updateDockWidgetTitles()
{
    mainWindow->updateDockWidgetTitles(model);
}

void VymView::expandAll() { treeEditor->expandAll(); }

void VymView::expandOneLevel()
{
    int level = 999999;
    int d;
    BranchItem *cur = nullptr;
    BranchItem *prev = nullptr;
    QModelIndex pix;

    // Find level to expand
    model->nextBranch(cur, prev);
    while (cur) {
        pix = model->index(cur);
        d = cur->depth();
        if (!treeEditor->isExpanded(pix) && d < level)
            level = d;
        model->nextBranch(cur, prev);
    }

    // Expand all to level
    cur = nullptr;
    prev = nullptr;
    model->nextBranch(cur, prev);
    while (cur) {
        pix = model->index(cur);
        d = cur->depth();
        if (!treeEditor->isExpanded(pix) && d <= level &&
            cur->branchCount() > 0)
            treeEditor->setExpanded(pix, true);
        model->nextBranch(cur, prev);
    }
}

void VymView::collapseOneLevel()
{
    int level = -1;
    int d;
    BranchItem *cur = nullptr;
    BranchItem *prev = nullptr;
    QModelIndex pix;

    // Find level to collapse
    model->nextBranch(cur, prev);
    while (cur) {
        pix = model->index(cur);
        d = cur->depth();
        if (treeEditor->isExpanded(pix) && d > level)
            level = d;
        model->nextBranch(cur, prev);
    }

    // collapse all to level
    cur = nullptr;
    prev = nullptr;
    model->nextBranch(cur, prev);
    while (cur) {
        pix = model->index(cur);
        d = cur->depth();
        if (treeEditor->isExpanded(pix) && d >= level)
            treeEditor->setExpanded(pix, false);
        model->nextBranch(cur, prev);
    }
}

void VymView::collapseUnselected()
{
    QModelIndex pix;

    // Find level to collapse
    BranchItem *selbi = model->getSelectedBranch();
    if (!selbi)
        return;

    QList<BranchItem *> itemPath;

    // Do not include selected branch,
    // this one also should be collapsed later
    BranchItem *cur = selbi->parentBranch();
    BranchItem *prev = nullptr;

    while (cur->parentBranch()) {
        itemPath << cur;
        cur = cur->parentBranch();
    }

    cur = nullptr;

    // collapse all to level
    model->nextBranch(cur, prev);
    while (cur) {
        pix = model->index(cur);
        if (treeEditor->isExpanded(pix) && itemPath.indexOf(cur) < 0) {
            treeEditor->setExpanded(pix, false);
        }
        model->nextBranch(cur, prev);
    }
}

void VymView::showSelection()
{
    QModelIndex ix = model->getSelectedIndex();
    treeEditor->scrollTo(ix, QAbstractItemView::EnsureVisible);
    mapEditor->scrollTo(ix);
}

void VymView::toggleTreeEditor()
{
    if (treeEditorDE->isVisible()) {
        treeEditorDE->hide();
        settings.setLocalValue(model->getFilePath(), "/treeEditor/visible",
                               "false");
    }
    else {
        treeEditorDE->show();
        settings.setLocalValue(model->getFilePath(), "/treeEditor/visible",
                               "true");
    }
    model->setChanged();
}

void VymView::toggleSlideEditor()
{
    if (slideEditorDE->isVisible()) {
        slideEditorDE->hide();
        settings.setLocalValue(model->getFilePath(), "/slideEditor/visible",
                               "false");
    }
    else {
        slideEditorDE->show();
        settings.setLocalValue(model->getFilePath(), "/slideEditor/visible",
                               "true");
    }
}

void VymView::setFocusMapEditor() { mapEditor->setFocus(); }
