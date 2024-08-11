#include "mapeditor.h"

#include <QGraphicsProxyWidget>
#include <QMenuBar>
#include <QObject>
#include <QPrintDialog>
#include <QPrinter>
#include <QScrollBar>

#include "animpoint.h"
#include "branchitem.h"
#include "geometry.h"
#include "heading-container.h"
#include "image-container.h"
#include "link-container.h"
#include "mainwindow.h"
#include "misc.h"
#include "shortcuts.h"
#include "warningdialog.h"
#include "winter.h"
#include "xlink.h"
#include "xlinkitem.h"
#include "xlinkobj.h"

extern Main *mainWindow;
extern QString clipboardDir;
extern QString clipboardFile;
extern bool debug;
extern QPrinter *printer;

extern QMenu *branchContextMenu;
extern QMenu *canvasContextMenu;
extern QMenu *floatimageContextMenu;
extern QMenu *taskContextMenu;

extern Switchboard switchboard;
extern Settings settings;

extern QTextStream vout;

extern QString editorFocusStyle;

extern FlagRowMaster *systemFlagsMaster;

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
MapEditor::MapEditor(VymModel *vm)
{
    // qDebug() << "Constructor ME " << this;

    QString shortcutScope = tr("Map Editor", "Shortcut scope");
    mapScene = new QGraphicsScene(nullptr);
    mapScene->setBackgroundBrush(QBrush(Qt::white, Qt::SolidPattern));
    mapScene->setItemIndexMethod(QGraphicsScene::NoIndex);  // FIXME-3 Avoiding crashes...
                                                            // Alternatively call removeFromIndex() in destructor
                                                            // or maybe also prepareGeometryChange()

    // Origin for view transformations (rotation, scaling)
    setTransformationAnchor(QGraphicsView::AnchorViewCenter);
    transformationOrigin = QPointF(0, 0);
    useTransformationOrigin = false;
    zoomDelta = 0.20;

    if (debug) {
        // Add cross in origin for debugging
        QPointF p;
        qreal size = 100;
        QGraphicsRectItem *x_axis = new QGraphicsRectItem(p.x() - size, p.y(), size * 2, 1 );
        QGraphicsRectItem *y_axis = new QGraphicsRectItem(p.x(), p.y() - size, 1, size * 2);
        x_axis->setBrush(Qt::NoBrush);
        y_axis->setBrush(Qt::NoBrush);
        x_axis->setPen(QColor(Qt::blue));
        y_axis->setPen(QColor(Qt::blue));

        mapScene->addItem(x_axis);
        mapScene->addItem(y_axis);

        // Add another cross
        p = QPointF(200,0);
        size = 20;
        QGraphicsRectItem *x_axis2 = new QGraphicsRectItem(p.x() - size, p.y(), size * 2, 1 );
        QGraphicsRectItem *y_axis2 = new QGraphicsRectItem(p.x(), p.y() - size, 1, size * 2);
        x_axis2->setBrush(Qt::NoBrush);
        y_axis2->setBrush(Qt::NoBrush);
        x_axis2->setPen(QColor(Qt::gray));
        y_axis2->setPen(QColor(Qt::gray));

        mapScene->addItem(x_axis2);
        mapScene->addItem(y_axis2);
    }

    zoomFactorInt = zoomFactorTargetInt = 1;
    rotationInt = rotationTargetInt = 0;

    model = vm;
    model->registerMapEditor(this);

    setScene(mapScene);

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    selectionMode = AutoSelection;

    setStyleSheet("QGraphicsView:focus {" + editorFocusStyle + "}");

    // Create bitmap cursors, platform dependant
    HandOpenCursor = QCursor(QPixmap(":/mode-move-view.png"), 1, 1);
    PickColorCursor = QCursor(QPixmap(":/cursorcolorpicker.png"), 5, 27);
    XLinkCursor = QCursor(QPixmap(":/cursorxlink.png"), 1, 7);

    editingBO = nullptr;

    printFrame = true;
    printFooter = true;

    setAcceptDrops(true);

    // Container used for temporary moving and relinking branches
    tmpParentContainer = new TmpParentContainer ();
    mapScene->addItem(tmpParentContainer);
    tmpParentContainer->setName("tmpParentContainer");

    // When moving objects, draw then on top of everything else
    tmpParentContainer->setZValue(10000);

    // Shortcuts and actions
    QAction *a;

    a = new QAction("Select upper branch", this);
    a->setShortcut(Qt::Key_Up);
    a->setShortcutContext(Qt::WidgetShortcut);
    connect(a, SIGNAL(triggered()), this, SLOT(cursorUp()));
    addAction(a);

    a = new QAction("Add upper branch to selection", this);
    a->setShortcut(Qt::Key_Up | Qt::SHIFT);
    a->setShortcutContext(Qt::WidgetShortcut);
    addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(cursorUpToggleSelection()));

    a = new QAction("Select lower branch", this);
    a->setShortcut(Qt::Key_Down);
    a->setShortcutContext(Qt::WidgetShortcut);
    addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(cursorDown()));

    a = new QAction("Add lower branch to selection", this);
    a->setShortcut(Qt::Key_Down | Qt::SHIFT);
    a->setShortcutContext(Qt::WidgetShortcut);
    addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(cursorDownToggleSelection()));

    a = new QAction("Select left branch", this);
    a->setShortcut(Qt::Key_Left);
    //  a->setShortcutContext (Qt::WidgetWithChildrenShortcut);
    addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(cursorLeft()));

    a = new QAction("Select child branch", this);
    a->setShortcut(Qt::Key_Right);
    //  a->setShortcutContext (Qt::WidgetWithChildrenShortcut);
    addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(cursorRight()));

    a = new QAction("Select first branch", this);
    a->setShortcut(Qt::Key_Home);
    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(cursorFirst()));

    a = new QAction("Select last branch", this);
    a->setShortcut(Qt::Key_End);
    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(cursorLast()));

    // Action to embed LineEdit for heading in Scene
    lineEdit = nullptr;

    a = new QAction(tr("Edit heading", "MapEditor"), this);
    a->setShortcut(Qt::Key_Return); // Edit heading
    a->setShortcutContext(Qt::WidgetShortcut);
    addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(editHeading()));
    a = new QAction(tr("Edit heading", "MapEditor"), this);
    a->setShortcut(Qt::Key_Enter); // Edit heading
    a->setShortcutContext(Qt::WidgetShortcut);
    addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(editHeading()));

    // Panning
    panningTimer = new QTimer(this);
    vPan = QPointF();
    connect(panningTimer, SIGNAL(timeout()), this, SLOT(panView()));

    // Clone actions defined in MainWindow
    foreach (QAction *qa, mainWindow->mapEditorActions) {
        a = new QAction(this);
        a->setShortcut(qa->shortcut());
        a->setShortcutContext(qa->shortcutContext());
        connect(a, SIGNAL(triggered()), qa, SLOT(trigger()));
        addAction(a);
    }

    setState(Neutral);

    winter = nullptr;

    // animations
    animationUse = settings.value("/animation/use", true) .toBool();
    animationTicks = settings.value("/animation/snapback/ticks", 50).toInt();
    animationInterval = settings.value("/animation/snapback/interval", 15).toInt();
    animatedContainers.clear();
    animationTimer = new QTimer(this);
    connect(animationTimer, SIGNAL(timeout()), this, SLOT(animate()));

}

MapEditor::~MapEditor()
{
    // qDebug ()<<"Destr MapEditor this="<<this;

    if (winter) {
        delete winter;
        winter = nullptr;
    }

    stopAllAnimation();
}

VymModel *MapEditor::getModel() { return model; }

QGraphicsScene *MapEditor::getScene() { return mapScene; }

void MapEditor::panView()
{
    if (!vPan.isNull()) {
        // Scroll if needed
        // To avoid jumping of the sceneView, only
        // show selection, if not tmp linked
        qreal px = 0;
        qreal py = 0;
        if (vPan.x() < 0)
            px = vPan.x();
        else if (vPan.x() > 0)
            px = width() + vPan.x();
        if (vPan.y() < 0)
            py = vPan.y();
        else if (vPan.y() > 0)
            py = height() + vPan.y();

        QPointF q = mapToScene(QPoint(px, py));
        QRectF r = QRectF(q, QPointF(q.x() + 1, q.y() + 1));

        // Expand view if necessary
        setScrollBarPosTarget(r);

        // Stop possible other animations
        if (scrollBarPosAnimation.state() == QAbstractAnimation::Running)
            scrollBarPosAnimation.stop();

        // Do linear animation
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() +
                                        vPan.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() + vPan.y());
    }
}

void MapEditor::ensureAreaVisibleAnimated(
        const QRectF &area, 
        bool scaled,
        bool rotated,
        qreal new_rotation)
{
    // Changes viewCenter to make sure that 
    // r is  within the margins of the viewport
    //
    // Only zooms, if r NOT fit into viewport 
    // view is centered then on bounding box.
    //
    // Similar to QGraphicsItem::ensureVisible, 
    // but with animation and (if necessary)
    // zooming

    int xmargin = settings.value("/mapeditor/scrollToMarginX/", 50).toInt();
    int ymargin = settings.value("/mapeditor/scrollToMarginY/", 50).toInt();

    // Do we need to zoom out to show area?
    QRect areaViewCoord = mapFromScene(area).boundingRect();

    // Visible area within margins
    QRect visibleViewCoord = rect();
    visibleViewCoord -= QMargins(xmargin, ymargin, xmargin, ymargin);

    if (!rotated)
        // Use current view rotation, if we do not plan to rotate
        new_rotation = rotationInt;

    // Calculate required width and height considering rotation of view
    qreal a = new_rotation / 180 * M_PI;
    qreal area_w_viewCoord = abs(sin(a) * area.height()) + abs(cos(a) * area.width());
    qreal area_h_viewCoord = abs(sin(a) * area.width()) + abs(cos(a) * area.height());
    qreal z_x = 1.0 * visibleViewCoord.width() / area_w_viewCoord;
    qreal z_y = 1.0 * visibleViewCoord.height() / area_h_viewCoord;

    qreal zf = min (z_x, z_y);

    bool zoomOutRequired = 
        (visibleViewCoord.width() < areaViewCoord.width() ||
         visibleViewCoord.height() < areaViewCoord.height());

    int animDuration = 2000;
    QEasingCurve easingCurve = QEasingCurve::OutQuint;
    
    //qDebug() << " zoom out: " << zoomOutRequired;
    //qDebug() << " zoom  in: " << zoomInRequired << " zoomFactor=" << zoomFactorInt << " zf=" << zf;
    if (zoomOutRequired || scaled) {
        setViewCenterTarget(
                area.center(), 
                zf, 
                new_rotation,
                animDuration,
                easingCurve);
        return;
    }

    // After zooming bbox would fit into margins of viewport
    long view_dx = 0;
    long view_dy = 0;
    if (areaViewCoord.left() < xmargin)
        // move left
        view_dx = areaViewCoord.left() - xmargin;
    else if (areaViewCoord.right() > viewport()->width())
        // move right
        view_dx = areaViewCoord.x() + areaViewCoord.width() - viewport()->width() + xmargin;

    if (areaViewCoord.top() < ymargin)
        // move up
        view_dy = areaViewCoord.top() - ymargin;
    else if (areaViewCoord.bottom() > viewport()->height() - ymargin)
        // move down
        view_dy = areaViewCoord.y() + areaViewCoord.height() - viewport()->height() + ymargin;

    if (abs(view_dx) > 5 || abs(view_dy) > 5 || rotated)
        setViewCenterTarget(
                mapToScene(viewport()->geometry().center() + QPoint (view_dx, view_dy)),
                zoomFactorInt,
                new_rotation,
                animDuration,
                easingCurve);
}

void MapEditor::ensureSelectionVisibleAnimated(bool scaled, bool rotated)
{
    // Changes viewCenter to make sure that bounding box of all currently
    // selected items is  within the margins of the viewport
    //
    // Only zooms, if bounding box of items does NOT fit into viewport 
    // view is centered then on bounding box. (Useful also for big images)
    //
    // Similar to QGraphicsItem::ensureVisible, but with animation and (if necessary)
    // zooming

    QList <TreeItem*> selis = model->getSelectedItems();

    // Nothing to do, if nothing is selected
    if (selis.isEmpty()) return;

    // Calculate total bounding box
    QRectF bbox;
    bool firstIteration = true;

    foreach (TreeItem *ti, selis) {
        Container *c = nullptr;
        QRectF c_bbox;
        if (ti->hasTypeAttribute())
            ti = ti->parent();
        if (ti->hasTypeBranch()) {
            c = ((BranchItem*)ti)->getBranchContainer()->getHeadingContainer();
            c_bbox = c->mapToScene(c->rect()).boundingRect();
        } else if (ti->getType() == TreeItem::Image) {
            c = ((ImageItem*)ti)->getImageContainer();
            c_bbox = c->mapToScene(c->rect()).boundingRect();
        } else if (ti->hasTypeXLink()) {
            XLinkObj *xlo = ((XLinkItem*)ti)->getXLinkObj();
            if (xlo)
                c_bbox = xlo->boundingRect();
        }

        if (firstIteration) {
            bbox = c_bbox;
            firstIteration = false;
        } else
            bbox = bbox.united(c_bbox);
    }
    int new_rotation = round_int(rotationInt) % 360;

    if (rotated && selis.count() == 1) {
        if (selis.first()->hasTypeBranch()) {
            BranchContainer *bc = ((BranchItem*)selis.first())->getBranchContainer();

            // Avoid rotations > 360
            setRotation(new_rotation);

            qreal rotScene = bc->rotationHeadingInScene();
            int d_rotation = new_rotation + round_int(rotScene) % 360;
            if (d_rotation > 180)
                d_rotation = d_rotation - 360;
            new_rotation = new_rotation - d_rotation;
        }
    }

    ensureAreaVisibleAnimated(bbox, scaled, rotated, new_rotation);
}

void MapEditor::setScrollBarPosTarget(QRectF rect)
{
    // Expand viewport, if rect is not contained
    if (!sceneRect().contains(rect))
        setSceneRect(sceneRect().united(rect));

    int xmargin = settings.value("/mapeditor/scrollToMarginX/", 80).toInt();
    int ymargin = settings.value("/mapeditor/scrollToMarginX/", 80).toInt();

    // Prepare scrolling
    qreal width = viewport()->width();
    qreal height = viewport()->height();
    QRectF viewRect = transform().
        scale(zoomFactorTargetInt, zoomFactorTargetInt).
        mapRect(rect);

    qreal left = horizontalScrollBar()->value();
    qreal right = left + width;
    qreal top = verticalScrollBar()->value();
    qreal bottom = top + height;

    scrollBarPosTarget = getScrollBarPos();

    if (viewRect.left() <= left + xmargin) {
        // need to scroll from the left
        scrollBarPosTarget.setX(int(viewRect.left() - xmargin - 0.5));
    }
    if (viewRect.right() >= right - xmargin) {
        // need to scroll from the right
        scrollBarPosTarget.setX(int(viewRect.right() - width + xmargin + 0.5));
    }
    if (viewRect.top() <= top + ymargin) {
        // need to scroll from the top
        scrollBarPosTarget.setY(int(viewRect.top() - ymargin - 0.5));
    }
    if (viewRect.bottom() >= bottom - ymargin) {
        // need to scroll from the bottom
        scrollBarPosTarget.setY(
            int(viewRect.bottom() - height + ymargin + 0.5));
    }
}

QPointF MapEditor::getScrollBarPosTarget() { return scrollBarPosTarget; }

void MapEditor::setScrollBarPos(const QPointF &p)
{
    scrollBarPos = p;
    horizontalScrollBar()->setValue(int(p.x()));
    verticalScrollBar()->setValue(int(p.y()));
}

QPointF MapEditor::getScrollBarPos()
{
    return QPointF(horizontalScrollBar()->value(),
                   verticalScrollBar()->value());
    // return scrollBarPos;
}

void MapEditor::animateScrollBars()
{
    qDebug() << "ME::animateScrollBars";
    if (scrollBarPosAnimation.state() == QAbstractAnimation::Running)
        scrollBarPosAnimation.stop();

    if (settings.value("/animation/use/", true).toBool()) {
        scrollBarPosAnimation.setTargetObject(this);
        scrollBarPosAnimation.setPropertyName("scrollBarPos");
        scrollBarPosAnimation.setDuration(
            settings.value("/animation/duration/scrollbar", 2000).toInt());
        scrollBarPosAnimation.setEasingCurve(QEasingCurve::OutQuint);
        scrollBarPosAnimation.setStartValue(QPointF(
            horizontalScrollBar()->value(), verticalScrollBar()->value()));
        scrollBarPosAnimation.setEndValue(scrollBarPosTarget);
        scrollBarPosAnimation.start();
    }
    else
        setScrollBarPos(scrollBarPosTarget);
}

void MapEditor::animate()
{
    animationTimer->stop();
    foreach (Container *c, animatedContainers) {
        c->animate();

        if (c->getContainerType() == Container::Branch)
            ((BranchContainer*)c)->updateUpLink();

        if (!c->isAnimated())
            animatedContainers.removeAll(c);
    }

    if (!animatedContainers.isEmpty())
        animationTimer->start(animationInterval);

    model->repositionXLinks();
}

void MapEditor::startAnimation(Container *c, const QPointF &v)  // FIXME-3 only used in ME::autoLayout
{
    if (!c) return;

    startAnimation(c, c->pos(), c->pos() + v);
}

void MapEditor::startAnimation(Container *c, const QPointF &start,
                              const QPointF &dest)
{
    if (start == dest) return;

    if (c) {
        c->setPos(start);
        AnimPoint ap;
        ap.setStart(start);
        ap.setDest(dest);
        ap.setTicks(animationTicks);
        ap.setAnimated(true);
        c->setAnimation(ap);
        if (!animatedContainers.contains(c))
            animatedContainers.append(c);
        animationTimer->setSingleShot(true);
        animationTimer->start(animationInterval);
    }
}

void MapEditor::stopAnimation(Container *c)
{
    int i = animatedContainers.indexOf(c);
    if (i >= 0)
        animatedContainers.removeAt(i);
}

void MapEditor::stopAllAnimation()
{
    animationTimer->stop();

    Container *c;
    while (!animatedContainers.isEmpty()) {
        c = animatedContainers.takeFirst();
        c->stopAnimation();
    }
}

void MapEditor::zoomIn()
{
    qreal f_zf = 1 + zoomDelta;   // view transformation grows

    useTransformationOrigin = false;
    setZoomFactorTarget(zoomFactorTargetInt * f_zf);
}

void MapEditor::zoomOut()
{
    qreal f_zf = 1 - zoomDelta;   // view transformation grows

    useTransformationOrigin = false;
    setZoomFactorTarget(zoomFactorTargetInt * f_zf);
}

void MapEditor::setZoomFactorTarget(const qreal &zft)
{
    zoomFactorTargetInt = zft;
    if (zoomAnimation.state() == QAbstractAnimation::Running)
        zoomAnimation.stop();

    if (settings.value("/animation/use/", true).toBool()) {
        zoomAnimation.setTargetObject(this);
        zoomAnimation.setPropertyName("zoomFactorInt");
        zoomAnimation.setDuration(
            settings.value("/animation/duration/zoom", 2000).toInt());
        zoomAnimation.setEasingCurve(QEasingCurve::OutQuint);
        zoomAnimation.setStartValue(zoomFactorInt);
        zoomAnimation.setEndValue(zft);
        zoomAnimation.start();
    }
    else
        setZoomFactor(zft);
}

qreal MapEditor::zoomFactorTarget() { return zoomFactorTargetInt; }

void MapEditor::setZoomFactor(const qreal &zf)
{
    zoomFactorInt = zf;
    updateMatrix();
}

qreal MapEditor::zoomFactor() { return zoomFactorInt; }

void MapEditor::setRotationTarget(const qreal &at)
{
    rotationTargetInt = at;
    if (rotationAnimation.state() == QAbstractAnimation::Running)
        rotationAnimation.stop();
    if (settings.value("/animation/use/", true).toBool()) {
        rotationAnimation.setTargetObject(this);
        rotationAnimation.setPropertyName("rotationInt");
        rotationAnimation.setDuration(
            settings.value("/animation/duration/rotation", 2000).toInt());
        rotationAnimation.setEasingCurve(QEasingCurve::OutQuint);
        rotationAnimation.setStartValue(rotationInt);
        rotationAnimation.setEndValue(at);
        rotationAnimation.start();
    }
    else
        setRotation(rotationTargetInt);
}

qreal MapEditor::rotationTarget() { return rotationTargetInt; }

void MapEditor::setRotation(const qreal &a)
{
    rotationInt = a;
    updateMatrix();
    if (winter)
        winter->updateView();
}

qreal MapEditor::rotation() { return rotationInt; }

void MapEditor::setViewCenterTarget(const QPointF &p, const qreal &zft,
                                    const qreal &at, const int duration,
                                    const QEasingCurve &easingCurve)
{
    viewCenterTarget = p;
    zoomFactorTargetInt = zft;
    rotationTargetInt = at;

    viewCenter = mapToScene(viewport()->geometry()).boundingRect().center();

    if (viewCenterAnimation.state() == QAbstractAnimation::Running)
        viewCenterAnimation.stop();
    if (rotationAnimation.state() == QAbstractAnimation::Running)
        rotationAnimation.stop();
    if (zoomAnimation.state() == QAbstractAnimation::Running)
        zoomAnimation.stop();

    if (settings.value("/animation/use/", true).toBool()) {
        viewCenterAnimation.setTargetObject(this);
        viewCenterAnimation.setPropertyName("viewCenter");
        viewCenterAnimation.setDuration(
            settings.value("/animation/duration/scrollbar", duration).toInt());
        viewCenterAnimation.setEasingCurve(easingCurve);
        viewCenterAnimation.setStartValue(viewCenter);
        viewCenterAnimation.setEndValue(viewCenterTarget);
        viewCenterAnimation.start();

        rotationAnimation.setTargetObject(this);
        rotationAnimation.setPropertyName("rotationInt");
        rotationAnimation.setDuration(
            settings.value("/animation/duration/rotation", duration).toInt());
        rotationAnimation.setEasingCurve(easingCurve);
        rotationAnimation.setStartValue(rotationInt);
        rotationAnimation.setEndValue(rotationTargetInt);
        rotationAnimation.start();

        zoomAnimation.setTargetObject(this);
        zoomAnimation.setPropertyName("zoomFactorInt");
        zoomAnimation.setDuration(
            settings.value("/animation/duration/zoom", duration).toInt());
        zoomAnimation.setEasingCurve(easingCurve);
        zoomAnimation.setStartValue(zoomFactorInt);
        zoomAnimation.setEndValue(zoomFactorTargetInt);
        zoomAnimation.start();
    } else {
        setRotation(rotationTargetInt);
        setZoomFactor(zft);
        setViewCenter(viewCenterTarget);
    }
}

void MapEditor::setViewCenterTarget()
{
    MapItem *selti = (MapItem *)(model->getSelectedItem());
    if (selti) {
        Container *c = nullptr;
        if (selti->hasTypeBranch()) 
            c = ((BranchItem*)selti)->getBranchContainer()->getHeadingContainer();
        else if (selti->hasTypeImage())
            c = ((ImageItem*)selti)->getImageContainer();
        else
            return;
        setViewCenterTarget(c->mapToScene(c->rect().center()), 1, 0);
    }
}

QPointF MapEditor::getViewCenterTarget() { return viewCenterTarget; }

void MapEditor::setViewCenter(const QPointF &vc) {
    // For wheel events // useTransFormationOrigin == true
    // and thus we will need vp_center in updateMatrix() later
    vp_center = vc; 
    centerOn(vc);
}

QPointF MapEditor::getViewCenter() { return viewCenter; }

void MapEditor::updateMatrix()
{
    if (useTransformationOrigin) {
        //qDebug() << " vp_center=" << toS(vp_center);
        centerOn(transformationOrigin);
    }

    QTransform t;
    t.rotate(rotationInt);
    t.scale(zoomFactorInt, zoomFactorInt);
    setTransform(t);

    if (useTransformationOrigin)
        centerOn(vp_center);
}

void MapEditor::minimizeView() {
    // If we only would set scene rectangle to existing items, then
    // view would "jump", when Qt automatically tries to center.
    // Better consider the currently visible viewport (with slight offset)
    QRectF r = mapToScene(viewport()->geometry()).boundingRect();
    r.translate(-2,-3);
    setSceneRect(scene()->itemsBoundingRect().united(r));
    //qDebug() << "ME::minimizeView";   // FIXME-2 check when and how often minimizeView is called
    // Used to be called also from VymModel::reposition()
}

void MapEditor::print()
{
    QRectF totalBBox = getTotalBBox();

    if (!printer)
        printer = mainWindow->setupPrinter();

    // Try to set orientation automagically
    // Note: Interpretation of generated postscript is amibiguous, if
    // there are problems with landscape mode, see
    // http://sdb.suse.de/de/sdb/html/jsmeix_print-cups-landscape-81.html

    if (totalBBox.width() > totalBBox.height())
        // recommend landscape
        printer->setPageOrientation(QPageLayout::Landscape);
    else
        // recommend portrait
        printer->setPageOrientation(QPageLayout::Portrait);

    QPrintDialog dialog(printer, this);
    dialog.setWindowTitle(tr("Print vym map", "MapEditor"));
    if (dialog.exec() == QDialog::Accepted) {
        QPainter pp(printer);

        pp.setRenderHint(QPainter::Antialiasing, true);

        // Don't print the visualisation of selection
        model->unselectAll();

        QRectF mapRect = totalBBox;
        QGraphicsRectItem *frame = nullptr;

        if (printFrame) {
            // Print frame around map
            mapRect.setRect(totalBBox.x() - 10, totalBBox.y() - 10,
                            totalBBox.width() + 20, totalBBox.height() + 20);
            frame = mapScene->addRect(mapRect, QPen(Qt::black),
                                      QBrush(Qt::NoBrush));
            frame->show();
        }

        double paperAspect =
            (double)printer->width() / (double)printer->height();
        double mapAspect = (double)mapRect.width() / (double)mapRect.height();
        int viewBottom;
        if (mapAspect >= paperAspect) {
            // Fit horizontally to paper width
            // pp.setViewport(0,0,
            // printer->width(),(int)(printer->width()/mapAspect) );
            viewBottom = (int)(printer->width() / mapAspect);
        }
        else {
            // Fit vertically to paper height
            // pp.setViewport(0,0,(int)(printer->height()*mapAspect),printer->height());
            viewBottom = printer->height();
        }

        if (printFooter) {
            // Print footer below map
            QFont font;
            font.setPointSize(10);
            pp.setFont(font);
            QRectF footerBox(0, viewBottom, printer->width(), 15);
            pp.drawText(footerBox, Qt::AlignLeft,
                        "VYM - " + model->getFileName());
            pp.drawText(footerBox, Qt::AlignRight,
                        QDate::currentDate().toString(Qt::TextDate));
        }
        mapScene->render(&pp,
                         QRectF(0, 0, printer->width(), printer->height() - 15),
                         QRectF(mapRect.x(), mapRect.y(), mapRect.width(),
                                mapRect.height()));

        // Viewport has paper dimension
        if (frame)
            delete (frame);

        // Restore selection
        model->reselect();
    }
}

QRectF MapEditor::getTotalBBox()    // FIXME-2 really needed? Overlaps with scene and VM... (compare getImage)
{
    minimizeView();
    return sceneRect();
}

QImage MapEditor::getImage(QPointF &offset)
{
    QRectF sceneRect = scene()->itemsBoundingRect();

    int d = 0; // border around sceneRect

    QRect imageRect;
    imageRect.setWidth(sceneRect.width() + 2 * d - 2);
    imageRect.setHeight(sceneRect.height() + 2 * d);

    offset = QPointF(sceneRect.left() - d, sceneRect.top() - d);
    QImage pix(imageRect.width(), imageRect.height(), QImage::Format_RGB32);

    //qDebug() << "ME::getImage   offset="<< offset << " imageRect=" << toS(imageRect,0) << " sceneRect=" << toS(sceneRect,0);
    QPainter pp(&pix);
    pp.setRenderHints(renderHints());
    mapScene->render(&pp,
                     // Destination:
                     QRectF(0, 0, imageRect.width(), imageRect.height()),
                     // Source in scene:
                     QRectF(sceneRect.x() - d, sceneRect.y() - d,
                            sceneRect.width() + 2 * d, sceneRect.height() + 2 * d ));
    return pix;
}

void MapEditor::setAntiAlias(bool b)
{
    setRenderHint(QPainter::Antialiasing, b);
}

void MapEditor::setSmoothPixmap(bool b)
{
    setRenderHint(QPainter::SmoothPixmapTransform, b);
}

void MapEditor::autoLayout()    // FIXME-3 not ported yet to containers. Review use case ("brainstorming")
{
    /*
    // Create list with all bounding polygons
    QList<LinkableMapObj *> mapobjects;
    QList<ConvexPolygon> polys;
    ConvexPolygon p;
    QList<Vector> vectors;
    QList<Vector> orgpos;
    QStringList headings; // FIXME-4 testing only
    Vector v;
    BranchItem *bi;
    BranchItem *bi2;
    BranchObj *bo;

    // Outer loop: Iterate until we have no more changes in orientation
    bool orientationChanged = true;
    while (orientationChanged) {
        BranchItem *ri = model->getRootItem();
        for (int i = 0; i < ri->branchCount(); ++i) {
            bi = ri->getBranchNum(i);
            bo = (BranchObj *)bi->getLMO();
            if (bo) {
                mapobjects.append(bo);
                p = bo->getBoundingPolygon();
                p.calcCentroid();
                polys.append(p);
                vectors.append(QPointF(0, 0));
                orgpos.append(p.at(0));
                headings.append(bi->headingPlain());
            }
            for (int j = 0; j < bi->branchCount(); ++j) {
                bi2 = bi->getBranchNum(j);
                bo = (BranchObj *)bi2->getLMO();
                if (bo) {
                    mapobjects.append(bo);
                    p = bo->getBoundingPolygon();
                    p.calcCentroid();
                    polys.append(p);
                    vectors.append(QPointF(0, 0));
                    orgpos.append(p.at(0));
                    headings.append(bi2->headingPlain());
                }
            }
        }

        // Iterate moving bounding polygons until we have no more collisions
        int collisions = 1;
        while (collisions > 0) {
            collisions = 0;
            for (int i = 0; i < polys.size() - 1; ++i) {
                for (int j = i + 1; j < polys.size(); ++j) {
                    if (polygonCollision(polys.at(i), polys.at(j),
                                         QPointF(0, 0))
                            .intersect) {
                        collisions++;
                        if (debug)
                            qDebug() << "Collision: " << headings[i] << " - "
                                     << headings[j];
                        v = polys.at(j).centroid() - polys.at(i).centroid();
                        v.normalize();
                        // Add random direction, if only two polygons with
                        // identical y or x
                        if (v.x() == 0 || v.y() == 0) {
                            Vector w(cos(double((int)rand() % 1000)),
                                     sin(double((int)rand() % 1000)));
                            w.normalize();
                            v = v + w;
                        }

                        // Scale translation vector by area of polygons
                        vectors[j] = v * 10000 / polys.at(j).weight();
                        vectors[i] = v * 10000 / polys.at(i).weight();
                        vectors[i].invert();
                        // FIXME-4 outer loop, "i" get's changed several
                        // times...
                        // Better not move away from centroid of 2 colliding
                        // polys, but from centroid of _all_
                    }
                }
            }
            for (int i = 0; i < vectors.size(); i++) {
                // qDebug() << " v="<<vectors[i]<<" "<<headings[i];
                if (!vectors[i].isNull())
                    polys[i].translate(vectors[i]);
            }
            // if (debug) qDebug()<< "Collisions total: "<<collisions;
            // collisions=0;
        }

        // Finally move the real objects and update
        QList<LinkableMapObj::Orientation> orients;
        for (int i = 0; i < polys.size(); i++) {
            Vector v = polys[i].at(0) - orgpos[i];
            orients.append(mapobjects[i]->getOrientation());
            if (!v.isNull()) {
                if (debug)
                    qDebug() << " Moving " << polys.at(i).weight() << " "
                             << mapobjects[i]->getAbsPos() << " -> "
                             << mapobjects[i]->getAbsPos() + v << "  "
                             << headings[i];
                // mapobjects[i]->moveBy(v.x(),v.y() );
                // mapobjects[i]->setRelPos();
                startAnimation((BranchObj *)mapobjects[i], v);
                if (debug)
                    qDebug() << i << " Weight: " << polys.at(i).weight() << " "
                             << v << " " << headings.at(i);
            }
        }
        / *
        model->reposition();
        orientationChanged=false;
        for (int i=0;i<polys.size();i++)
            if (orients[i]!=mapobjects[i]->getOrientation())
            {
            orientationChanged=true;
            break;
            }
        * /

        break;

        // orientationChanged=false;
    } // loop if orientation has changed
    */
}

TreeItem *MapEditor::findMapItem(
        QPointF p,
        const QList <TreeItem*> &excludedItems,
        bool findNearCenter)
{
    // Search XLinks
    XLink *xlink;
    for (int i = 0; i < model->xlinkCount(); i++) {
        xlink = model->getXLinkNum(i);
        if (xlink) {
            XLinkObj *xlo = xlink->getXLinkObj();
            if (xlo) {
                XLinkObj::SelectionType xlinkSelection = xlo->couldSelect(p);
                if (xlinkSelection == XLinkObj::Path) {
                    // Found path of XLink, now return the nearest XLinkItem of p
                    qreal d0 = Geometry::distance(p, xlo->getBeginPos());
                    qreal d1 = Geometry::distance(p, xlo->getEndPos());
                    if (d0 < d1)
                        return xlink->beginXLinkItem();
                    else
                        return xlink->endXLinkItem();
                }
                if (xlinkSelection == XLinkObj::C0)
                    return xlink->beginXLinkItem();
                if (xlinkSelection == XLinkObj::C1)
                    return xlink->endXLinkItem();
            }
        }
    }

    // Search branches (and their childs, e.g. images
    // Start with mapcenter, no images allowed at rootItem
    BranchItem *nearestFloatingCenter = nullptr;
    qreal d = 0;
    int i = 0;
    BranchItem *bi = model->getRootItem()->getFirstBranch();
    TreeItem *found = nullptr;
    while (bi) {
        found = bi->findMapItem(p, excludedItems);
        if (found)
            return found;

        if (findNearCenter) {
            // Try to find nearest MapCenter   // FIXME-3 or branch with floating layout.
                                               // Currently only MapCenters are searched
            Container *hc = bi->getBranchContainer()->getHeadingContainer();
            QPointF q = hc->mapToScene(hc->center());
            if (!nearestFloatingCenter) {
                nearestFloatingCenter = bi;
                d = Geometry::distance(p, q);
            } else {
                qreal d2 = Geometry::distance(p, q);
                if (d2 < d) {
                    d = d2;
                    nearestFloatingCenter = bi;
                }
            }
        }

        i++;
        bi = model->getRootItem()->getBranchNum(i);
    }

    if (nearestFloatingCenter && d < 80 && !excludedItems.contains(nearestFloatingCenter))
        return nearestFloatingCenter;

    return nullptr;
}

BranchItem *MapEditor::findMapBranchItem(
        QPointF p,
        const QList <TreeItem*> &excludedItems,
        bool findNearCenter)
{
    TreeItem *ti = findMapItem(p, excludedItems, findNearCenter);
    if (ti && ti->hasTypeBranch())
        return (BranchItem*)ti;
    else
        return nullptr;
}

void MapEditor::testFunction1()
{
    //autoLayout();
    qDebug() << "ME::test";
    qDebug() << "  hor_scrollbar=" << horizontalScrollBar()->value();
    qDebug() << "  ver_scrollbar=" << verticalScrollBar()->value();
    qDebug() << "           rect=" << toS(rect());
    qDebug() << "      sceneRect=" << toS(sceneRect());
    qDebug() << "             tf=" << zoomFactorInt;
    qDebug() << "       pageStep=" << horizontalScrollBar()->pageStep();
}

void MapEditor::testFunction2()
{
    TreeItem *selti = model->getSelectedItem();
    if (selti)
    {
        if (selti->hasTypeBranch()) {
            BranchContainer *bc = ((BranchItem*)selti)->getBranchContainer();
            bc->setScrollOpacity(bc->getScrollOpacity() * 0.9);   // FIXME-3 animation test
        } else if (selti->hasTypeImage()) {
            ImageContainer *ic = ((ImageItem*)selti)->getImageContainer();
            qDebug() << ic->info() << ic;
        } else
            qDebug() << "Unknown type";
    } else
        qWarning() << "Nothing selected";
}

void MapEditor::toggleWinter()
{
    if (winter) {
        delete winter;
        winter = nullptr;
    }
    else {
        winter = new Winter(this);
        QList<QRectF> obstacles;
        BranchContainer *bc;
        BranchItem *cur = nullptr;
        BranchItem *prev = nullptr;
        model->nextBranch(cur, prev);
        while (cur) {
            if (!cur->hasHiddenExportParent()) {
                // Branches
                bc = cur->getBranchContainer();
                if (bc->isVisible()) {
                    HeadingContainer *hc = bc->getHeadingContainer();
                    obstacles.append(hc->mapRectToScene(hc->boundingRect()));
                }
            }
            model->nextBranch(cur, prev);
        }
        winter->setObstacles(obstacles);
    }
}

bool MapEditor::isContainerCloserInDirection(Container *c1, Container *c2, const qreal &d_min, const QPoint &v, RadarDirection radarDir)
{
    qreal d = c1->distance(c2);

    switch(radarDir) {
        case UpDirection:
            if (v.y() < 0 &&  (d_min < 0 || d < d_min))
                return true;
            break;

        case DownDirection:
            if (v.y() > 0 &&  (d_min < 0 || d < d_min))
                return true;
            break;

        case LeftDirection:
            if (v.x() < 0 &&  (d_min < 0 || d < d_min))
                return true;
            break;

        case RightDirection:
            if (v.x() > 0 &&  (d_min < 0 || d < d_min))
                return true;
            break;
        default:
            qWarning() << "MapEditor::isContainerCloserInDirection undefined radar";
    }
    return false;
}

TreeItem* MapEditor::getItemInDirection(TreeItem *ti, RadarDirection radarDir)
{
    SelectionMode selMode = currentSelectionMode(ti);

    if (selMode == GeometricSelection)
        return getItemFromGeometry(ti, radarDir);

    if (selMode == OrgChartSelection)
        return getItemFromOrgChart(ti, radarDir);

    return getItemFromClassicMap(ti, radarDir);
}

TreeItem* MapEditor::getItemFromGeometry(TreeItem *ti, RadarDirection radarDir)
{
    TreeItem *nearestItem = nullptr;
    if (ti) {
        // Calculate reference points: Nearest corners of selected towards radarDir
        QPoint rp_view;
        Container *c = nullptr;
        if (ti->hasTypeBranch())
            c = ((BranchItem*)ti)->getBranchContainer()->getHeadingContainer();
        else if (ti->hasTypeImage())
            c = ((ImageItem*)ti)->getImageContainer();

        if (!c) {
            qWarning() << __func__ << "No container found";
            return nullptr;
        }

        rp_view = mapFromScene(c->mapToScene(c->center()));

        qreal d_min = -1;
        qreal d;
        BranchItem *cur = nullptr;
        BranchItem *prev = nullptr;
        model->nextBranch(cur, prev);
        while (cur) {
            // Interate over all branches in map
            BranchContainer *bc;
            bc = cur->getBranchContainer();
            if (bc && bc->isVisible()) {
                HeadingContainer *hc = bc->getHeadingContainer();

                QPointF p_scene = hc->mapToScene(hc->center());
                QPoint v = mapFromScene(p_scene) - rp_view; // Direction between centers
                d = c->distance(hc);
                if (cur != ti && isContainerCloserInDirection(c, hc, d_min, v, radarDir)) {
                    d_min = d;
                    nearestItem = cur;
                }

                // Iterate over images
                for (int i = 0; i < cur->imageCount(); i++) {
                    ImageItem *ii = cur->getImageNum(i);
                    ImageContainer *ic = ii->getImageContainer();
                    p_scene = ic->mapToScene(ic->center());
                    v = mapFromScene(p_scene) - rp_view;
                    d = c->distance(ic);
                    if (ii != ti && isContainerCloserInDirection(c, ic, d_min, v, radarDir)) {
                        d_min = d;
                        nearestItem = ii;
                    }
                }
            }
            model->nextBranch(cur, prev);
        }
    }

    return nearestItem;
}

TreeItem* MapEditor::getItemFromOrgChart(TreeItem *ti, RadarDirection radarDir)
{
    BranchItem *bi = nullptr;
    if (ti->hasTypeBranch())
        bi = (BranchItem*)ti;

    if (!bi)
        return nullptr;

    if (radarDir == UpDirection)
        return bi->parentBranch();
    else if (radarDir == DownDirection)
        return bi->getLastSelectedBranch();
    else if (radarDir == LeftDirection)
        return getItemFromClassicMap(ti, UpDirection);
    else if (radarDir == RightDirection)
        return getItemFromClassicMap(ti, DownDirection);

    return nullptr;
}

TreeItem* MapEditor::getItemFromClassicMap(TreeItem *selti, RadarDirection radarDir)
{
    if (!selti)
        return nullptr;

    BranchItem * bi = nullptr;
    if (selti->hasTypeBranch())
        bi = (BranchItem*)selti;

    int d = selti->depth(); // original depth

    if (radarDir == UpDirection) {
        TreeItem *ti = getItemDirectAbove(selti);

        if (ti)
            // direct predecessor
            return ti;

        // Go towards center and look for predecessor
        while (selti->depth() > 0) {
            selti = selti->parent();
            ti = getItemDirectAbove(selti);

            if (ti) {
                // turn
                selti = ti;
                while (selti->depth() < d) {
                    // try to get back to original depth dz
                    ti = selti->getLastItem();
                    if (!ti)
                        return selti;

                    selti = ti;
                }
                return selti;
            }
        }

    } else if (radarDir == DownDirection) {
        TreeItem *ti = getItemDirectBelow(selti);

        if (ti) return ti;

        // Go towards center and look for siblings
        while (selti->depth() > 0) {
            selti = selti->parent();
            ti = getItemDirectBelow(selti);

            if (ti) {
                // turn
                selti = ti;
                while (selti->depth() < d) {
                    // try to get back to original depth d
                    ti = selti->getFirstItem();
                    if (!ti)
                        return selti;

                    selti = ti;
                }
                return selti;
            }
        }

    } else if (radarDir == LeftDirection) {
        if (bi) {
            // Branch selected
            if (d == 0) {
                // Special case: use alternative selection index
                BranchItem *newbi = bi->getLastSelectedBranchAlt();
                if (!newbi) {
                    BranchContainer *bc;
                    // Try to find a mainbranch left of center
                    for (int i = 0; i < bi->branchCount(); i++) {
                        newbi = bi->getBranchNum(i);
                        bc = newbi->getBranchContainer();
                        if (bc && bc->getOrientation() == BranchContainer::LeftOfParent)
                            break;
                    }
                }
                return newbi;
            }
            if (bi->getBranchContainer()->getOrientation() ==
                BranchContainer::RightOfParent)
                // right of center
                return bi->parentBranch();
            else {
                // left of center
                TreeItem *ri = bi->getLastSelectedBranch();
                if (ri)
                    // Return last selected branch
                    return ri;
                else
                    // Look for image
                    return getItemFromGeometry(bi, LeftDirection);
            }
        }
    } else if (radarDir == RightDirection) {
        if (bi) {
            // Branch selected
            if (d == 0) {
                // Special case: use alternative selection index
                BranchItem *newbi = bi->getLastSelectedBranch();
                if (!newbi) {
                    BranchContainer *bc;
                    // Try to find a mainbranch right of center
                    for (int i = 0; i < bi->branchCount(); i++) {
                        newbi = bi->getBranchNum(i);
                        bc = newbi->getBranchContainer();
                        if (bc && bc->getOrientation() == BranchContainer::RightOfParent)
                            break;
                    }
                }
                return newbi;
            }
            if (bi->getBranchContainer()->getOrientation() == BranchContainer::LeftOfParent)
                // left of center
                return bi->parentBranch();
            else {
                // right of center
                TreeItem *ri = bi->getLastSelectedBranch();
                if (ri)
                    // Return last selected branch
                    return ri;
                else
                    // Look for image
                    return getItemFromGeometry(bi, RightDirection);
            }
        }
    }
    return nullptr;
}

TreeItem *MapEditor::getItemDirectAbove(TreeItem *ti)
{
    if (ti) {
        if (ti->hasTypeBranch()) {
            BranchItem *bi = (BranchItem*)ti;
          
            int i = bi->num();
            if (i > 0)
                return bi->parent()->getBranchNum(i - 1);
        } else if (ti->hasTypeImage()) {
            ImageItem *ii = (ImageItem*)ti;
          
            int i = ii->num();
            if (i > 0)
                return ii->parent()->getImageNum(i - 1);
        }
    }
    return nullptr;
}

TreeItem *MapEditor::getItemDirectBelow(TreeItem *ti)
{
    if (ti) {
        if (ti->hasTypeBranch()) {
            BranchItem *bi = (BranchItem*)ti;
            int i = bi->num();
            if (i + 1 < bi->parent()->branchCount())
                return bi->parent()->getBranchNum(i + 1);
        } else if (ti->hasTypeImage()) {
            ImageItem *ii = (ImageItem*)ti;
            int i = ii->num();
            if (i + 1 < ii->parent()->imageCount())
                return ii->parent()->getImageNum(i + 1);
        }
    }
    return nullptr;
}

void MapEditor::cursorUp()
{
    if (editorState == MapEditor::EditingHeading)
        return;

    TreeItem *selti = model->getSelectedItem();
    if (selti) {
        selti = getItemInDirection(selti, UpDirection);
        if (selti)
            model->select(selti);
    }
}

void MapEditor::cursorUpToggleSelection()
{
    if (editorState == MapEditor::EditingHeading)
        return;

    QList <TreeItem*> seltis = model->getSelectedItems();
    TreeItem *selti = model->getSelectedItem();
    TreeItem *ti;

    if (seltis.size() == 0)
        return;
    else if (seltis.size() == 1) {
        ti = getItemInDirection(seltis.first(), UpDirection);
        if (ti) model->selectToggle(ti);
    } else {
        // Nothing selected or already multiple selections
        TreeItem *last_ti = model->lastToggledItem();
        if (last_ti && (last_ti->hasTypeBranch() || last_ti->hasTypeImage())) {
            if (lastToggleDirection == toggleUp)
                ti = getItemInDirection(last_ti, UpDirection);
            else
                ti = last_ti;

            if (ti)
                model->selectToggle(ti);
        }
    }
    lastToggleDirection = toggleUp;
}

void MapEditor::cursorDown()
{
    if (editorState == MapEditor::EditingHeading)
        return;

    TreeItem *selti = model->getSelectedItem();
    if (selti) {
        selti = getItemInDirection(selti, DownDirection);
        if (selti)
            model->select(selti);
    }
}

void MapEditor::cursorDownToggleSelection()
{
    if (editorState == MapEditor::EditingHeading)
        return;

    TreeItem *selti = model->getSelectedItem();
    TreeItem *ti;
    if (selti) {
        ti = getItemInDirection(selti, DownDirection);
        if (ti) {
            model->selectToggle(ti);
        }
    } else {
        // Nothing selected or already multiple selections
        TreeItem *last_ti = model->lastToggledItem();
        if (last_ti && (last_ti->hasTypeBranch() || last_ti->hasTypeImage())) {
            if (lastToggleDirection == toggleDown)
                ti = getItemInDirection(last_ti, DownDirection);
            else
                ti = last_ti;

            if (ti)
                model->selectToggle(ti);
        }
    }
    lastToggleDirection = toggleDown;
}

void MapEditor::cursorLeft()
{
    TreeItem *selti = model->getSelectedItem();
    if (!selti) {
        // If multiple items are selected, select the next to last toggled one
        selti = model->lastToggledItem();
        if (!selti) return;
    }

    TreeItem *ti = getItemInDirection(selti, LeftDirection);
    if (ti)
        model->select(ti);
}

void MapEditor::cursorRight()
{
    TreeItem *selti = model->getSelectedItem();

    if (!selti) {
        // If multiple items are selected, select the next to last toggled one
        selti = model->lastToggledItem();
        if (!selti) return;
    }

    TreeItem *ti = getItemInDirection(selti, RightDirection);
    if (ti)
        model->select(ti);
    else {
        ImageItem *ii = selti->getFirstImage();
        if (ii)
            model->select(ii);
    }
}

void MapEditor::cursorFirst() { model->selectFirstBranch(); }  // FIXME-3 adapt for images and container layouts

void MapEditor::cursorLast() { model->selectLastBranch(); }  // FIXME-3 adapt for images and container layouts

void MapEditor::editHeading()
{
    if (editorState == EditingHeading) {
        editHeadingFinished();
        return;
    }

    BranchItem *bi = model->getSelectedBranch();
    if (bi) {
        VymText heading = bi->heading();
        if (heading.isRichText() || bi->headingPlain().contains("\n")) {
            mainWindow->windowShowHeadingEditor();
            ensureSelectionVisibleAnimated();
            return;
        }
        model->setSelectionBlocked(true);

        lineEdit = new QLineEdit;
        QGraphicsProxyWidget *proxyWidget = mapScene->addWidget(lineEdit);
        // FIXME-3-FT get total rotation XXX for BC in scene and do "proxyWidget->setRotation(XXX);
        lineEdit->setCursor(Qt::IBeamCursor);
        lineEdit->setCursorPosition(1);

#if defined(Q_OS_WINDOWS)
        QFont font = lineEdit->font();
        font.setPointSize(font.pointSize() + 4);
        lineEdit->setFont(font);
#endif

        QPointF tl;
        QPointF br;
        qreal w = 230;
        qreal h = 30;

        BranchContainer *bc = bi->getBranchContainer();
        if (bc->getOrientation() == BranchContainer::RightOfParent) {
            tl = bc->headingRect().topLeft();
            br = tl + QPointF(w, h);
        }
        else {
            br = bc->headingRect().bottomRight();
            tl = br - QPointF(w, h);
        }
        // Qt bug when using QProxyWdiget in scaled QGraphicsView
        // https://bugreports.qt.io/browse/QTBUG-48681
        // Still present in Qt 6.5
        QRectF r(tl, br);
        lineEdit->setGeometry(r.toRect());
        proxyWidget->setGeometry(r.toRect());

        minimizeView();

        // Set focus to MapEditor first
        // To avoid problems with Cursor up/down
        setFocus();

        ensureAreaVisibleAnimated(r);

        if (heading.getTextASCII() == " ")
            heading.setPlainText("");
        lineEdit->setText(heading.getTextASCII());
        lineEdit->setFocus();
        lineEdit->selectAll(); // Hack to enable cursor in lineEdit
        lineEdit->deselect();  // probably a Qt bug...

        setState(EditingHeading);
    }
}

void MapEditor::editHeadingFinished()
{
    if (editorState != EditingHeading || !lineEdit ) {
        qWarning() << "ME::editHeadingFinished not editing heading!";
    } else {
        lineEdit->clearFocus();
        QString s = lineEdit->text();
        s.replace(QRegularExpression("\\n"), " "); // Don't paste newline chars
        if (s.length() == 0)
            s = " "; // Don't allow empty lines, which would screw up drawing
        model->setHeadingPlainText(s);
        delete (lineEdit);
        lineEdit = nullptr;

        // Maybe reselect previous branch
        mainWindow->editHeadingFinished(model);

        // Autolayout to avoid overlapping branches with longer headings
        if (settings.value("/mainwindow/autoLayout/use", "true") == "true")
            autoLayout();
    }

    model->setSelectionBlocked(false);
    setState(Neutral);
}

void MapEditor::contextMenuEvent(QContextMenuEvent *e)
{
    // Lineedits are already closed by preceding
    // mouseEvent, we don't need to close here.

    QPointF p = mapToScene(e->pos());
    TreeItem *ti = findMapItem(p);

    if (ti) {
        model->select(ti);

        BranchItem *selbi = model->getSelectedBranch();

        // Context Menu
        if (selbi) {
            QString sysFlagName;
            QUuid uid = selbi->getBranchContainer()->findFlagByPos(p);
            if (!uid.isNull()) {
                Flag *flag = systemFlagsMaster->findFlagByUid(uid);
                if (flag)
                    sysFlagName = flag->getName();
            }

            if (sysFlagName.startsWith("system-task"))
                taskContextMenu->popup(e->globalPos());
            else
                // Context Menu on branch or mapcenter
                branchContextMenu->popup(e->globalPos());
        }
        else {
            if (model->getSelectedImage()) {
                // Context Menu on floatimage
                floatimageContextMenu->popup(e->globalPos());
            }
            else {
                if (model->getSelectedXLink())
                    // Context Menu on XLink
                    model->editXLink();
            }
        }
    }
    else { // No object or container found, we are on the Canvas itself
        // Context Menu on scene

        // Open context menu synchronously to position new mapcenter
        model->setContextPos(p);
        canvasContextMenu->exec(e->globalPos());
        model->unsetContextPos();
    }
    e->accept();
}

void MapEditor::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_PageUp || e->key() == Qt::Key_PageDown)
        // Ignore PageUP/Down to avoid scrolling with keys
        return;

    if (e->modifiers() & Qt::ShiftModifier)
        updateCursor();
    QGraphicsView::keyPressEvent(e);
}

void MapEditor::keyReleaseEvent(QKeyEvent *e)
{
    if (!(e->modifiers() & Qt::ShiftModifier))
        setCursor(Qt::ArrowCursor);
}

void MapEditor::startPanningView(QMouseEvent *e)
{
    setState(PanningView);
    panning_initialPointerPos = e->globalPosition().toPoint();
    panning_initialScrollBarValues =                  // Used for scrollbars when moving view
        QPoint(horizontalScrollBar()->value(), verticalScrollBar()->value());
    setCursor(Qt::ClosedHandCursor);
}

void MapEditor::mousePressEvent(QMouseEvent *e) // FIXME-3  Drop down dialog, if multiple tree items are found to select the "right" one
{
    // Ignore right clicks
    if (e->button() == Qt::RightButton) {
        e->ignore();
        QGraphicsView::mousePressEvent(e);
        return;
    }

    // Check if we need to reset zoomFactor for middle button + Ctrl
    if (e->button() == Qt::MiddleButton && e->modifiers() & Qt::ControlModifier) {
        setZoomFactorTarget(1);
        setRotationTarget(0);
        return;
    }

    // Initial position of pointer in scene coordinates. See also e->globalPos (!)
    movingObj_initialScenePos = mapToScene(e->pos());

    TreeItem *ti_found = findMapItem(movingObj_initialScenePos);

    // Allow selecting text in QLineEdit if necessary
    if (model->isSelectionBlocked()) {
        e->ignore();
        QGraphicsView::mousePressEvent(e);
        return;
    }

    // Stop editing in LineEdit
    if (editorState == EditingHeading) editHeadingFinished();

    /*
    qDebug() << "ME::mouse pressed\n";
    qDebug() << "   ti_found=" << ti_found;
    */
    //if (ti_found) qDebug() << "   ti_found="<<ti_found->heading();

    // If Modifier mode "view" is set, all other clicks can be ignored,
    // nothing will be selected
    if ((e->modifiers() & Qt::ShiftModifier) &&
            mainWindow->getModMode() == Main::ModModeMoveView) {
        startPanningView(e);
        return;
    }

    BranchItem *selbi = model->getSelectedBranch();
    BranchContainer *selbc = nullptr;
    if (selbi) {
        selbc = selbi->getBranchContainer();

        // XLink modifier, create new XLink
        if (mainWindow->getModMode() == Main::ModModeXLink &&
            (e->modifiers() & Qt::ShiftModifier)) {
            setState(CreatingXLink);
            tmpXLink = new XLink(model);
            tmpXLink->setBeginBranch(selbi);
            tmpXLink->createXLinkObj();
            tmpXLink->setStyleBegin("None");
            tmpXLink->setStyleEnd("None");
            tmpXLink->setEndPoint(movingObj_initialScenePos);
            tmpXLink->updateXLink();
            return;
        }
    }

    QString sysFlagName;

    if (ti_found) {
        // Check modifier key (before selecting object!)
        if (e->modifiers() & Qt::ShiftModifier) {
            if (mainWindow->getModMode() == Main::ModModeColor) {
                setState(PickingColor);
                mainWindow->setCurrentColor(ti_found->headingColor());
                if (e->modifiers() & Qt::ControlModifier)
                    model->colorBranch(ti_found->headingColor());
                else
                    model->colorSubtree(ti_found->headingColor());
                return;
            }

        }

        // Check for flags on MousePress
        if (selbc) {
            QUuid uid = selbc->findFlagByPos(movingObj_initialScenePos);
            if (!uid.isNull()) {
                Flag *flag = systemFlagsMaster->findFlagByUid(uid);
                if (flag)
                    sysFlagName = flag->getName();
            }
        }

        // Check vymlink  modifier (before selecting object!)
        if (sysFlagName == "system-vymLink") {
            model->select(ti_found);
            if (e->modifiers() & Qt::ControlModifier) {
                if (e->modifiers() & Qt::ShiftModifier)
                    model->deleteVymLink();
                else
                    mainWindow->editOpenVymLink(true);
            } else
                mainWindow->editOpenVymLink(false);
            return;
        }

        // Select the clicked object, if not moving without linking
        if (e->modifiers() & Qt::ShiftModifier) {
            if (mainWindow->getModMode() == Main::ModModePoint) {
                lastToggleDirection = toggleUndefined;

                model->selectToggle(ti_found);
            }
        } else {
            if (model->getSelectedItems().count() < 2 || !model->getSelectedItems().contains(ti_found))
                // Only add ti_found, if we don't have multi-selection yet, which we
                // want to move around. In that case we would ignore the "pressed" event
                model->select(ti_found);
        }
        movingItems = model->getSelectedItemsReduced();

        // Make sure currently clicked item is first in list
        int i = movingItems.indexOf(ti_found);
        if (i > 0)
            movingItems.move(i, 0);

        // Left Button	    Move Branches
        if (e->button() == Qt::LeftButton) {
            // No system flag clicked, take care of moving modes or simply
            // start moving
            if (ti_found->hasTypeBranch())
            {
                BranchContainer *bc = ((BranchItem*)ti_found)->getBranchContainer();
                movingObj_initialContainerOffset = movingObj_initialScenePos - bc->getHeadingContainer()->mapToScene(QPointF(0,0));
            }

            if (mainWindow->getModMode() == Main::ModModeMoveObject &&
                    e->modifiers() & Qt::ShiftModifier) {
                setState(MovingObjectWithoutLinking);
            }
            else
                setState(MovingObject);

            // Set initial position and size of tmpParentContainer
            // Required when ONLY moving images.
            tmpParentContainer->setPos(movingObj_initialScenePos - movingObj_initialContainerOffset);
            if (movingItems.count() > 0) {
                qreal w = 0;
                qreal h = 0;
                BranchContainer *bc_first = nullptr;
                foreach (TreeItem *ti, movingItems) {
                    if (ti->hasTypeBranch()) {
                        BranchContainer* bc = ((BranchItem*)ti)->getBranchContainer();
                        if (!bc_first)
                            bc_first = bc;
                        w = max(w, bc->rect().width());
                        h += bc->rect().height();
                    }
                }
                if (bc_first) tmpParentContainer->setRect(bc_first->rect().left(), bc_first->rect().top(), w, h);
            }
        }
        else
            // Middle Button - Toggle Scroll
            //
            // (On Mac OS X this won't work, but we still have
            // a button in the toolbar)
            if (e->button() == Qt::MiddleButton)
                model->toggleScroll();
    } else {
        // No ti_found, we are on the scene itself
        // Left Button	    move Pos of sceneView
        if (e->button() == Qt::LeftButton ||
            e->button() == Qt::MiddleButton) {
            startPanningView(e);
            return;
        }
    }

    e->accept();

    // Take care of  remaining system flags _or_ modifier modes
    if (selbc) {
        if (!sysFlagName.isEmpty()) {
            // systemFlag clicked
            if (sysFlagName.contains("system-url")) {
                if (e->modifiers() & Qt::ControlModifier)
                    mainWindow->editOpenURLTab();
                else
                    mainWindow->editOpenURL();
            } else if (sysFlagName == "system-note")
                mainWindow->windowToggleNoteEditor();
            else if (sysFlagName == "hideInExport")
                model->toggleHideExport();
            else if (sysFlagName.startsWith("system-task-"))
                model->cycleTaskStatus();
            return;
        }
    }   // system flags or modModes
    else { // No selbc found, check XLinks
        if (ti_found) {
            if (ti_found->getType() == TreeItem::XLinkType) {
                XLinkObj *xlo = ((XLinkItem *)ti_found)->getXLink()->getXLinkObj();
                if (xlo)
                    setState(EditingXLink);
            }
        }
    }
}

void MapEditor::mouseMoveEvent(QMouseEvent *e)
{
    QPointF p_event = mapToScene(e->pos());

    // Show mouse position for debugging in statusBar
    if (debug && e->modifiers() & Qt::ControlModifier)
        mainWindow->statusMessage(
            QString("ME::mouseMoveEvent  Scene: %1 - Viewport: %2")
                .arg(toS(p_event, 0),  toS(e->pos())));

    // Allow selecting text in QLineEdit if necessary
    if (model->isSelectionBlocked()) {
        e->ignore();
        QGraphicsView::mouseMoveEvent(e);
        return;
    }

    // Pan view
    if (editorState == PanningView) {
        QPointF pg = e->globalPosition();
        QPoint v_pan;
        v_pan.setX(-pg.x() + panning_initialPointerPos.x());
        v_pan.setY(-pg.y() + panning_initialPointerPos.y());
        horizontalScrollBar()->setSliderPosition(
            (int)(panning_initialScrollBarValues.x() + v_pan.x()));
        verticalScrollBar()->setSliderPosition(
            (int)(panning_initialScrollBarValues.y() + v_pan.y()));

        // Avoid flickering
        scrollBarPosAnimation.stop();
        viewCenterAnimation.stop();
        // rotationAnimation.stop();
        // zoomAnimation.stop();

        return;
    }

    // After clicking object shift might have been pressed, adjust state then
    if (mainWindow->getModMode() == Main::ModModeMoveObject &&
            e->modifiers() & Qt::ShiftModifier && editorState == MovingObject) {
        setState(MovingObjectWithoutLinking);
    }

    // Move the selected items
    if (movingItems.count() > 0  &&
        (editorState == MovingObject ||
         editorState == MovingObjectTmpLinked ||
         editorState == MovingObjectWithoutLinking ||
         editorState == EditingXLink)) {

        if (!(e->buttons() & Qt::LeftButton)) {
            // Sometimes at least within a VM there might be a
            // release event lost, while still the mousePress event is processed
            //
            // So moving without a pressed left button is considered a "release"
            mouseReleaseEvent(e);
            return;
        }

        int margin = 50;

        // Check if we have to scroll
        QPointF p = e->position();
        vPan.setX(0);
        vPan.setY(0);
        if (p.y() >= 0 && p.y() <= margin)
            vPan.setY(p.y() - margin);
        else if (p.y() <= height() && p.y() > height() - margin)
            vPan.setY(p.y() - height() + margin);
        if (p.x() >= 0 && p.x() <= margin)
            vPan.setX(p.x() - margin);
        else if (p.x() <= width() && p.x() > width() - margin)
            vPan.setX(p.x() - width() + margin);

        moveObject(e, p_event);
    } // selection && moving_obj

    // Draw a link from one branch to another
    if (editorState == CreatingXLink) {
        tmpXLink->setEndPoint(p_event);
        tmpXLink->updateXLink();
    }
}

void MapEditor::moveObject(QMouseEvent *e, const QPointF &p_event)
{
    bool repositionRequired = false;
    bool updateUpLinksRequired = false;

    // If necessary pan the view using animation
    if (!panningTimer->isActive())
        panningTimer->start(50);

    // reset cursor if we are moving and don't copy

    // Check if we could link (temporary). Consider also "near" mapCenters.
    TreeItem *targetItem = findMapItem(p_event, movingItems, true);

    // Check, if targetItem is a child of one of the moving items
    if (targetItem) {
        foreach (TreeItem *ti, movingItems) {
            if (targetItem->isChildOf(ti)) {
                // qWarning() << "ME::moveObject " << targetItem->headingPlain() << "is child of " << ti->headingPlain();
                targetItem = nullptr;
                break;
            }
        }
    }

    // Add selected branches and images temporary to tmpParentContainer,
    // if they are not there yet:
    BranchContainer *bc_first = nullptr;
    BranchContainer *bc_prev  = nullptr;
    if (movingItems.count() > 0 && (tmpParentContainer->childrenCount() == 0)) {
        BranchContainer *bc;
        foreach (TreeItem *ti, movingItems)
        {
            // The item structure in VymModel remaines untouched so far,
            // only containers will be reparented temporarily!
            if (ti->hasTypeBranch()) {
                BranchItem *bi = (BranchItem*)ti;
                bc = bi->getBranchContainer();

                if (!bc_first) {
                    bc_first = bc;

                    // Initially set orientation of tmpParentContainer to first BranchContainer
                    if (bc->getOrientation() == BranchContainerBase::UndefinedOrientation) {
                        // Orientation undefined for MapCenters, assume RightOfParent
                        tmpParentContainer->setOrientation(BranchContainerBase::RightOfParent);
                    } else
                        tmpParentContainer->setOrientation(bc->getOrientation());
                }

                if (tmpParentContainer->branchCount() == 0 || 
                    bc->parentItem() != tmpParentContainer->getBranchesContainer()) {

                    // Save position of children branches in case we only want to
                    // move this branch and keep children unchanged using CTRL modifier
                    if (bc->hasFloatingBranchesLayout())
                        foreach(BranchContainer *bc2, bc->childBranches())
                            bc2->setOriginalScenePos();
                                                                            //
                    bc->setOriginalPos();
                    bc->setOriginalOrientation();   // Also sets originalParentBranchContainer
                    tmpParentContainer->addToBranchesContainer(bc);

                }

                if (bc_first && bc_first != bc) {
                    QPointF p;
                    // Animate other items to position horizontally centered below first one
                    if (bc_first->getOriginalOrientation() == BranchContainer::RightOfParent) {
                        p = tmpParentContainer->mapFromItem(bc,
                                bc->alignTo(Container::TopLeft, bc_prev, Container::BottomLeft));
                    } else if (bc_first->getOriginalOrientation() == BranchContainer::LeftOfParent)
                        p = tmpParentContainer->mapFromItem(bc,
                                bc->alignTo(Container::TopRight, bc_prev, Container::BottomRight));
                    else
                        p = tmpParentContainer->mapFromItem(bc,
                                bc->alignTo(Container::TopCenter, bc_prev, Container::BottomCenter));

                    startAnimation ( bc, bc->pos(), p);
                }
                bc_prev = bc;
            } else if (ti->hasTypeImage()) {
                ImageContainer *ic = ((ImageItem*)ti)->getImageContainer();
                if (ic->parentItem() != tmpParentContainer->getImagesContainer()) {
                    ic->setOriginalPos();
                    tmpParentContainer->addToImagesContainer(ic);
                }
        }
            else if (ti->getType() == TreeItem::XLinkType) {
                // Move XLink control point
                XLinkObj *xlo = ((XLinkItem *)ti)->getXLinkObj();
                if (xlo) {
                    xlo->setSelectedCtrlPoint(p_event); // FIXME-3 Missing savestate
                    model->setChanged();
                }
            }
            else
                qWarning("ME::moveObject  Huh? I'm confused. No BC, IC or XLink moved");
        }
    } // add to tmpParentContainer

    if (tmpParentContainer->childBranches().count() > 0)
        // If ME::moveObject is called AFTER tPC has been filled previously, 
        // bc_first still might be unset here
        bc_first = tmpParentContainer->childBranches().first();

    BranchContainer *targetBranchContainer = nullptr;

    // Check if we are moving a branch and could relink. Position tmpParentContainer
    if (targetItem && targetItem->hasTypeBranch() && bc_first &&
                !(mainWindow->getModMode() == Main::ModModeMoveObject &&
                    (e->modifiers() & Qt::ShiftModifier))) {
        setState(MovingObjectTmpLinked);

        targetBranchContainer = ((BranchItem*)targetItem)->getBranchContainer();

        Container *targetRefContainer = targetBranchContainer->getHeadingContainer();
        Container *movingRefContainer = bc_first->getHeadingContainer();
        Container::PointName targetRefPointName;
        Container::PointName movingRefPointName;
        QPointF linkOffset;                     // Distance for temporary link

        BranchContainer *tbc = targetBranchContainer->parentBranchContainer();
        if (e->modifiers() & Qt::ShiftModifier && tbc) {
            qreal dy = targetBranchContainer->rect().height() / 2;
            targetBranchContainer = targetBranchContainer->parentBranchContainer();

            if (targetBranchContainer->getOrientation() == BranchContainer::RightOfParent) {
                // Shift modifier: Link right above
                targetRefPointName = Container::BottomRight;
                movingRefPointName = Container::BottomLeft;
                linkOffset = QPointF(model->mapDesign()->linkWidth(), - dy);
            } else if (targetBranchContainer->getOrientation() == BranchContainer::LeftOfParent) {
                    // Shift modifier: Link left above
                    targetRefPointName = Container::TopLeft;
                    movingRefPointName = Container::BottomRight;
                    linkOffset = QPointF(- model->mapDesign()->linkWidth(), dy);
            }   // else:  Undefined orientation is handled with hasFloatingLayout() below!
        } else if (e->modifiers() & Qt::ControlModifier && tbc) {
            qreal dy = targetBranchContainer->rect().height() / 2;
            targetBranchContainer = targetBranchContainer->parentBranchContainer();
            if (targetBranchContainer->getOrientation() == BranchContainer::RightOfParent) {
                // Control modifier: Link right below
                targetRefPointName = Container::TopRight;
                movingRefPointName = Container::TopLeft;
                linkOffset = QPointF(model->mapDesign()->linkWidth(), dy);
            } else if (targetBranchContainer->getOrientation() == BranchContainer::LeftOfParent) {
                    // Control modifier: Link left below
                    targetRefPointName = Container::TopLeft;
                    movingRefPointName = Container::TopRight;
                    linkOffset = QPointF(- model->mapDesign()->linkWidth(), dy);
            }   // else:  Undefined orientation is handled with hasFloatingLayout() below!
        } else {
            // No modifier used, temporary link to target itself
            targetRefContainer = targetBranchContainer->getBranchesContainer();
            movingRefContainer = tmpParentContainer;
            if (targetBranchContainer->getOrientation() == BranchContainer::RightOfParent) {
                if (targetBranchContainer->branchCount() == 0) {
                    // vertically centered besides target
                    targetRefPointName = Container::RightCenter;
                    movingRefPointName = Container::LeftCenter;
                    linkOffset = QPointF(model->mapDesign()->linkWidth(), 0);
                } else {
                    // Below target
                    targetRefPointName = Container::BottomLeft;
                    movingRefPointName = Container::TopLeft;
                }
            } else if (targetBranchContainer->getOrientation() == BranchContainer::LeftOfParent) {
                if (targetBranchContainer->branchCount() == 0) {
                    // vertically centered besides target
                    targetRefPointName = Container::LeftCenter;
                    movingRefPointName = Container::RightCenter;
                    linkOffset = QPointF(- model->mapDesign()->linkWidth(), 0);
                } else {
                    // Below target
                    targetRefPointName = Container::BottomRight;
                    movingRefPointName = Container::TopRight;
                }
            }   // else:  Undefined orientation is handled with hasFloatingLayout() below!
        }

        if (!targetRefContainer)
            targetRefContainer = ((BranchItem*)targetItem)->getBranchContainer();

        // Align tmpParentContainer
        if (targetBranchContainer->hasFloatingBranchesLayout()) {
            // When temporary linking e.g. to MapCenter, position on a circle around MC

            qreal radius = 80;

            QPointF center_sp = targetBranchContainer->getHeadingContainer()->mapToScene(QPointF(0,0));
            qreal a = getAngle(p_event - center_sp);
            QPointF p_hint = center_sp + QPointF (radius * cos(a), - radius * sin(a));

            tmpParentContainer->setPos(p_hint);
        } else
            // Temporary link to branchContainers of targetRefContainer. Use position calculated above
            tmpParentContainer->setPos(
                                        linkOffset + movingRefContainer->mapToScene(
                                                        movingRefContainer->alignTo(
                                                            movingRefPointName, targetRefContainer, targetRefPointName)));

        // Set states of MapEditor and tPC
        if (tmpParentContainer->movingState() != BranchContainerBase::TemporaryLinked) {
            // Link tmpParentContainer temporarily to targetBranchContainer

            tmpParentContainer->setMovingState(BranchContainerBase::TemporaryLinked, targetBranchContainer);
            setState(MovingObjectTmpLinked);
        }

        repositionRequired = true;

    } // tmp linking to target
    else {
        // Move without temporary relinking to a target
        //
        // Update state of MapEditor
        if (mainWindow->getModMode() == Main::ModModeMoveObject &&
                e->modifiers() & Qt::ShiftModifier)
            setState(MovingObjectWithoutLinking);
        else
            setState(MovingObject);

        if (tmpParentContainer->movingState() == BranchContainerBase::TemporaryLinked)
            tmpParentContainer->setMovingState(BranchContainerBase::Moving);

        updateUpLinksRequired = true;
    }

    // Update states of children branch containers (for updating links later)
    foreach (BranchContainer *bc, tmpParentContainer->childBranches()) {
        if (tmpParentContainer->movingState() == BranchContainerBase::TemporaryLinked)
            bc->setMovingState(BranchContainerBase::TemporaryLinked, targetBranchContainer);
        else
            bc->setMovingState(BranchContainerBase::Moving);
    }

    // Set orientation
    BranchContainer::Orientation newOrientation;
    Container *tpc_bc = tmpParentContainer->getBranchesContainer();
    if (targetBranchContainer && tpc_bc && !tpc_bc->childItems().contains(targetBranchContainer)) {
        // tmpParentContainer has children and these do NOT contain targetBranchContainer

        if (targetBranchContainer->hasFloatingBranchesLayout()) {
            if (p_event.x() > targetBranchContainer->getHeadingContainer()->mapToScene(QPointF(0,0)).x())
                newOrientation = BranchContainer::RightOfParent;
            else
                newOrientation = BranchContainer::LeftOfParent;
        } else {
            // Relinking to other branch
            newOrientation = targetBranchContainer->getOrientation();
        }
    } else {
        // No target branch

        if (bc_first) {
            // Set new orientation for branches (not mapCenters): Consider pointer pos relative to first moving branch
            if (p_event.x() > bc_first->getOriginalParentPos().x() && !(e->modifiers() & Qt::ControlModifier))
                newOrientation = BranchContainer::RightOfParent;
            else
                newOrientation = BranchContainer::LeftOfParent;
        } else
            // No target and no branch moving. No orientation change.
            newOrientation = tmpParentContainer->getOrientation();
    }

    // Reposition if required
    if (newOrientation != tmpParentContainer->getOrientation()) {
        // tPC has BoundingFloats layout, still children need orientation
        tmpParentContainer->setOrientation(newOrientation);
        repositionRequired = true;
    }

    if (repositionRequired) {
        if (bc_first && targetBranchContainer && targetBranchContainer->hasFloatingBranchesLayout()) {
            foreach(BranchContainer *bc, tmpParentContainer->childBranches())
                bc->setOrientation(newOrientation);
        }
        tmpParentContainer->reposition();
    }

    if (!targetBranchContainer) {
        // Above tPC was positioned only if there is a target, so now tPC->setPos() is required if there is no target
        // Since orientation might have changed and position depends on orientation, only do this now
        if (bc_first) {
            QPointF hc_center = tmpParentContainer->mapFromItem(bc_first->getHeadingContainer(), bc_first->getHeadingContainer()->pos());
            tmpParentContainer->setPos(p_event - hc_center - movingObj_initialContainerOffset);

            // When moving with Ctrl  modifier, don't children branches (in scene)
            if (e->modifiers() & Qt::ControlModifier) {
                foreach(BranchContainer *bc, tmpParentContainer->childBranches()) {
                    if (bc->hasFloatingBranchesLayout()) {
                        foreach(BranchContainer *bc2, bc->childBranches()) {
                            QPointF q = bc->getHeadingContainer()->sceneTransform().inverted().map(bc2->getOriginalPos());
                            bc2->setPos(q);
                            bc2->updateUpLink();
                        }
                    }
                }
            }

        } else
            // No branches, only image
            tmpParentContainer->setPos(p_event - movingObj_initialContainerOffset);

    }

    if (updateUpLinksRequired) {
        foreach(BranchContainer *bc, tmpParentContainer->childBranches())   
            bc->updateUpLink();
    }

    model->repositionXLinks();
}

void MapEditor::mouseReleaseEvent(QMouseEvent *e)
{
    // Allow selecting text in QLineEdit if necessary
    if (model->isSelectionBlocked()) {
        e->ignore();
        QGraphicsView::mouseReleaseEvent(e);
        return;
    }

    QPointF p = mapToScene(e->pos());

    BranchItem *destinationBranch;

    destinationBranch = findMapBranchItem(p, movingItems, true);

    bool repositionNeeded = false;

    // Have we been picking color?
    if (editorState == PickingColor) {
        setCursor(Qt::ArrowCursor);
        // Check if we are over another branch
        if (destinationBranch) {
            if (e->modifiers() & Qt::ShiftModifier)
                model->colorBranch(mainWindow->getCurrentColor());
            else
                model->colorSubtree(mainWindow->getCurrentColor());
        }
        setState(Neutral);
        return;
    }

    // Have we been drawing a link?
    if (editorState == CreatingXLink) {
        setState(Neutral);

        TreeItem *seli;
        if (movingItems.count() > 0)
            seli = movingItems.first();
        else
            seli = nullptr;

        // Check if we are over another branch
        if (destinationBranch) {
            tmpXLink->setEndBranch(destinationBranch);
            tmpXLink->activate();
            tmpXLink->updateXLink();
            if (model->createXLink(tmpXLink)) return;
        }
        delete (tmpXLink);
        tmpXLink = nullptr;
        return;
    }

    // Have we been moving something?
    if (editorState == MovingObject || editorState == MovingObjectTmpLinked) {
        panningTimer->stop();

        // Check if we have a destination and should relink
        if (destinationBranch && editorState != MovingObjectWithoutLinking) {
            // Restore list of selected items later

            // Prepare relinking
            BranchItem *dst_branch = destinationBranch;
            int dst_num = -1;

            if (e->modifiers() & Qt::ShiftModifier && destinationBranch->parent()) {
                // Link above dst
                dst_branch = destinationBranch->parentBranch();
                dst_num = destinationBranch->num();
            } else if (e->modifiers() & Qt::ControlModifier && destinationBranch->parent()) {
                // Link below dst
                dst_branch = destinationBranch->parentBranch();
                dst_num = destinationBranch->num() +  1;
            }

            // Tell VymModel to relink
            QList <BranchItem*> movingBranches;
            foreach(BranchContainer *bc, tmpParentContainer->childBranches()) {
                bc->setMovingState(BranchContainerBase::NotMoving);
                movingBranches << bc->getBranchItem();
            }


            if (!movingBranches.isEmpty())
                model->relinkBranches(
                        movingBranches,
                        dst_branch,
                        dst_num);
            // If dst is scrolled, select it
            if (dst_branch->isScrolled())
                model->select(dst_branch);

            // Loop over images // FIXME-3 refactor in VM similar to relinkBranches
            foreach(ImageContainer *ic, tmpParentContainer->childImages()) {
                ImageItem *ii = ic->getImageItem();
                model->relinkImage(ii, destinationBranch);
            }

            if (!tmpParentContainer->childImages().isEmpty()) {
                foreach(ImageContainer *ic, tmpParentContainer->childImages()) {
                    ImageItem *ii = ic->getImageItem();
                    model->selectToggle(ii);
                }
            }
        } else {
            // Branches moved, but not relinked

            QList <BranchContainer*> childBranches = tmpParentContainer->childBranches();
            QList <QPointF> animationCurrentPositions;   // After reposition start animations
            QList <BranchContainer*> animationContainers;

            if (!childBranches.isEmpty()) {
                repositionNeeded = true;

                // We begin a saveStateBlock. If nothing is really moved, this
                // block will be discarded later
                model->saveStateBeginBlock(
                    QString("Move %1 branch(es)").arg(childBranches.count())
                );

                // Empty the tmpParentContainer, which is used for moving
                // Updating the stacking order also resets the original parents
                foreach(BranchContainer *bc, childBranches) {
                    BranchItem *bi = bc->getBranchItem();

                    bc->setMovingState(BranchContainerBase::NotMoving);

                    if (bc->isAnimated()) 
                        bc->stopAnimation();

                    // Relink container to original parent container
                    // and keep (!) current absolute position
                    bi->updateContainerStackingOrder();

                    // Floating layout e.g. MapCenter
                    if (bc->isFloating())
                    {
                        if (bi->depth() == 0)
                            // MapCenter
                            bc->setPos(bc->getHeadingContainer()->mapToScene(QPointF(0, 0)));
                        // Save position change
                        QString uc, rc;
                        uc = QString("setPos%1;").arg(toS(bc->getOriginalPos(), 5));
                        rc = QString("setPos%1;").arg(toS(bc->pos(), 5)),
                        model->saveStateBranch(bi, uc, rc, QString("Move branch to %1").arg(toS(bc->pos())));
                    } else {
			if (!(e->modifiers() & Qt::ControlModifier)) {
			    // only animate snappack if not Ctrl-moving e.g. MC
			    animationContainers << bc;
			    animationCurrentPositions << bc->pos();
			}
                    }
                } // children of tmpParentContainer
                model->saveStateEndBlock();
            }   // Empty tmpParenContainer

            if (animationUse && animationContainers.count() > 0) {
                int i = 0;
                foreach(BranchContainer *bc, animationContainers) {
                    startAnimation(bc, animationCurrentPositions.at(i), bc->getOriginalPos());
                    i++;
                }
            }
        } // Branches moved, but not relinked

        // Let's see if we moved images with tmpParentContainer
        if (tmpParentContainer->childImages().count() > 0 ) {
            repositionNeeded = true;
        }

        foreach(ImageContainer *ic, tmpParentContainer->childImages()) {
            ImageItem *ii = ic->getImageItem();
            BranchItem *pi = ii->parentBranch();

            // Update parent of moved container to original imageContainer
            // in parent branch
            pi->addToImagesContainer(ic);

            QString pold = toS(ic->getOriginalPos());
            QString pnow = toS(ic->pos());
            model->saveState( // FIXME-1 saveState: add setPos to ImageWrapper
                    ii, "setPos " + pold,
                    ii, "setPos " + pnow,
                    QString("Move %1 to %2") .arg(model->getObjectName(ii),  pnow));

        } // Image moved, but not relinked

        // Finally resize scene, if needed
        scene()->update();
        vPan = QPoint();
    } // MovingObject or MovingObjecttmpXLinked

    if (editorState != EditingHeading) {
        setState(Neutral); // Continue editing after double click!
    }

    // Restore cursor
    updateCursor();

    movingItems.clear();
    QGraphicsView::mouseReleaseEvent(e);

    if (repositionNeeded)
        model->reposition();    // FIXME-3 really reposition whole model? Or only affected MapCenters?
}

void MapEditor::mouseDoubleClickEvent(QMouseEvent *e)
{
    // Allow selecting text in QLineEdit if necessary
    if (model->isSelectionBlocked()) {
        e->ignore();
        QGraphicsView::mouseDoubleClickEvent(e);
        return;
    }

    if (e->button() == Qt::LeftButton) {
        QPointF p = mapToScene(e->pos());
        TreeItem *ti = findMapItem(p);
        if (ti) {
            if (editorState == EditingHeading)
                editHeadingFinished();
            model->select(ti);
            BranchItem *selbi = model->getSelectedBranch();
            if (selbi) {
                BranchContainer *bc = selbi->getBranchContainer();
                QUuid uid = bc->findFlagByPos(p);

                // Don't edit heading when double clicking flag:
                if (!uid.isNull())
                    return;
            }
            e->accept();
            editHeading();
        }
    }
}

void MapEditor::wheelEvent(QWheelEvent *e)
{
    if (e->modifiers() & Qt::ControlModifier &&
        e->angleDelta().y() != 0) {

        qreal f_vp;
        qreal f_zf;
        if (e->angleDelta().y() > 0) {
            // Zoom in
	    f_vp = 1 - zoomDelta;           // vector to center of viewport shrinks
	    f_zf = 1 + zoomDelta + 0.046;   // view transformation grows
        } else {
            // Zoom out
	    f_vp = 1 + zoomDelta;
	    f_zf = 1 - zoomDelta + 0.046;
        }

        if (rotationAnimation.state() == QAbstractAnimation::Running)
            rotationAnimation.stop();

        transformationOrigin = mapToScene(e->position().toPoint());
        vp_center = mapToScene(viewport()->rect().center());

        // Calculate center of scaled viewport with p as transformation origin
        vp_center = (vp_center - transformationOrigin) * f_vp + transformationOrigin;

        useTransformationOrigin = true;
        //setZoomFactorTarget(zoomFactorTargetInt * f_zf);
        setViewCenterTarget(vp_center, zoomFactorTargetInt * f_zf, rotationTargetInt);
    }
    else {
        scrollBarPosAnimation.stop();
        QGraphicsView::wheelEvent(e);
    }
}

void MapEditor::focusOutEvent(QFocusEvent *)
{
    // qDebug()<<"ME::focusOutEvent"<<e->reason();
    if (editorState == EditingHeading)
        editHeadingFinished();
}

void MapEditor::resizeEvent(QResizeEvent *e) { QGraphicsView::resizeEvent(e); }

void MapEditor::dragEnterEvent(QDragEnterEvent *event)
{
    // for (unsigned int i=0;event->format(i);i++) // Debug mime type
    //	cerr << event->format(i) << endl;

    if (event->mimeData()->hasImage())
        event->acceptProposedAction();
    else if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void MapEditor::dragMoveEvent(QDragMoveEvent *) {}

void MapEditor::dragLeaveEvent(QDragLeaveEvent *event) { event->accept(); }

void MapEditor::dropEvent(QDropEvent *event)
{
    BranchItem *selbi = model->getSelectedBranch();
    if (selbi) {
        if (debug) {
            foreach (QString format, event->mimeData()->formats())
                qDebug() << "MapEditor: Dropped format: " << qPrintable(format);
            foreach (QUrl url, event->mimeData()->urls()) {
                qDebug() << "  URL-path:" << url.path();
                qDebug() << "URL-string:" << url.toString();
                qDebug() << "       enc:" << url.toEncoded();
                qDebug() << "     valid:" << url.isValid();
            }
            qDebug() << "-------------- mimeData -------------------";
            qDebug() << "has-img : " << event->mimeData()->hasImage();
            qDebug() << "has-urls: " << event->mimeData()->hasUrls();
            qDebug() << "    text: " << event->mimeData()->text();
            qDebug() << "-------------------------------------------";
        }

        if (event->mimeData()->hasUrls()) {
            // Try text representation first, which works on windows, but in
            // Linux only for https, not local images
            QString url = event->mimeData()->text();
            if (url.isEmpty()) {
                QByteArray ba =
                    event->mimeData()->urls().first().path().toLatin1();
                QByteArray ba2;
                for (int i = 0; i < ba.size(); i++)
                    if (ba.at(i) != 0)
                        ba2.append(ba.at(i));
                url = ba2;
            }

            BranchItem *bi = nullptr;
            // Workaround to avoid adding empty branches
            if (!url.isEmpty()) {
                if (url.startsWith("file://"))
                    url.remove(0, 7);

#if defined(Q_OS_WIN32)
                if (url.startsWith("/"))
                    url.remove(0, 1);
#endif
                if (isImage(url)) {
                    if (debug)
                        qDebug() << "dropped url seems to be image: " << url;
                    // Image, try to download or set image from local file
                    if (url.startsWith("http"))
                        model->downloadImage(url);
                    else
                        model->loadImage(bi, url);
                    if (debug)
                        qDebug() << "finished loading image";
                }
                else {
                    bi = model->addNewBranch();
                    if (bi) {
                        model->select(bi);
                        if (url.endsWith(".vym", Qt::CaseInsensitive))
                            model->setVymLink(url);
                        else {
                            model->setUrl(url);

                            // Shorten long URLs for heading
                            int i = url.indexOf("?");
                            QString url_short = url.left(i);
                            if (i > 0) 
                                url_short = url_short + "...";
                            model->setHeadingPlainText(url_short);
                        }

                        model->select(bi->parent());
                    }
                }
            }
        }
    }
    event->acceptProposedAction();
}

void MapEditor::updateCursor()
{
    if (qApp->queryKeyboardModifiers() & Qt::ShiftModifier) {
        switch (mainWindow->getModMode()) {
            case Main::ModModePoint:
                setCursor(Qt::ArrowCursor);
                break;
            case Main::ModModeColor:
                setCursor(PickColorCursor);
                break;
            case Main::ModModeXLink:
                setCursor(XLinkCursor);
                break;
            case Main::ModModeMoveObject:
                setCursor(Qt::PointingHandCursor);
                break;
            case Main::ModModeMoveView:
                setCursor(Qt::OpenHandCursor);
                break;
            default:
                setCursor(Qt::ArrowCursor);
                break;
        }
    } else {
        setCursor(Qt::ArrowCursor);
    }
}

void MapEditor::setState(EditorState s)
{
    editorState = s;
    /* if (debug) {
        QString s;
        switch (state) {
            case Neutral:
                s = "Neutral";
                break;
            case EditingHeading:
                s = "EditingHeading";
                break;
            case EditingLink:
                s = "EditingLink";
                break;
            case MovingObject:
                s = "MovingObject";
                break;
            case MovingObjectWithoutLinking:
                s = "MovingObjectWithoutLinking";
                break;
            case MovingView:
                s = "MovingView";
                break;
            case PickingColor:
                s = "PickingColor";
                break;
            case CreatingXLink:
                s = "CreatingXLink";
                break;
            default:
                s = "Unknown editor state";
                break;
        }
        qDebug() << "MapEditor: State " << s << " of " << model->getMapName();
    }
    */
}

MapEditor::EditorState MapEditor::state() { return editorState; }


MapEditor::SelectionMode MapEditor::currentSelectionMode(TreeItem *selti)
{
    // Selections should consider logical relations, e.g. siblings and parents
    // but also geometric. Return the most appropriate mode depending on 
    // rotation of view and layout of selected item.
    if (!selti) {
        qDebug() << "ME::selectionMode: Classic";
        return ClassicSelection;
    }

    if (rotationInt != 0) {
        qDebug() << "ME::selectionMode: Geometric";
        return GeometricSelection;
    }

    Container *c = nullptr;
    if (selti->hasTypeBranch()) {
        BranchContainer *bc = ((BranchItem*)selti)->getBranchContainer();
        if (bc->branchesContainerLayout() == Container::Horizontal) {
            qDebug() << "ME::selectionMode: OrgChart";
            return OrgChartSelection;
        }
    } else if (selti->hasTypeImage()) {
        qDebug() << "ME::selectionMode: Geometric";
        return GeometricSelection;
    }

    if (!c) {
        qDebug() << "ME::selectionMode: Classic";
        return ClassicSelection;
    }

    if (c->isFloating()) {
        qDebug() << "ME::selectionMode: Geometric";
        return GeometricSelection;
    }

    qDebug() << "ME::selectionMode: Classic";
    return ClassicSelection;
}

void MapEditor::updateData(const QModelIndex &sel)
{
    TreeItem *ti = static_cast<TreeItem *>(sel.internalPointer());

    // qDebug() << "ME::updateData for " << model->headingText(ti);

    if (ti && ti->hasTypeBranch())
        ((BranchItem*)ti)->updateVisuals();

    if (winter) {
        QList<QRectF> obstacles;
        BranchContainer *bc;
        BranchItem *cur = nullptr;
        BranchItem *prev = nullptr;
        model->nextBranch(cur, prev);
        while (cur) {
            if (!cur->hasHiddenExportParent()) {
                // Branches
                bc = cur->getBranchContainer();
                if (bc && bc->isVisible()) {
                    HeadingContainer *hc = bc->getHeadingContainer();
                    obstacles.append(hc->mapRectToScene(hc->boundingRect()));
                }
            }
            model->nextBranch(cur, prev);
        }
        winter->setObstacles(obstacles);
    }
}

void MapEditor::togglePresentationMode()
{
    mainWindow->togglePresentationMode();
}
