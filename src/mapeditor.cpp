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
#include "link-container.h"
#include "mainwindow.h"
#include "misc.h"
#include "shortcuts.h"
#include "warningdialog.h"
#include "winter.h"
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
    mapScene->setItemIndexMethod(QGraphicsScene::NoIndex);  // FIXME-2 Avoiding crashes...
                                                            // Alternatively call removeFromIndex() in destructor
                                                            // or maybe also prepareGeometryChange()

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
        p = QPointF(130,0);
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

    zoomFactor = zoomFactorTarget = 1;
    angle = angleTarget = 0;

    model = vm;
    model->registerMapEditor(this);

    setScene(mapScene);

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

    // Shortcuts and actions
    QAction *a;

    a = new QAction("Select upper branch", this);
    a->setShortcut(Qt::Key_Up);
    a->setShortcutContext(Qt::WidgetShortcut);
    connect(a, SIGNAL(triggered()), this, SLOT(cursorUp()));
    addAction(a);

    a = new QAction("Add upper branch to selection", this);
    a->setShortcut(Qt::Key_Up + Qt::SHIFT);
    a->setShortcutContext(Qt::WidgetShortcut);
    addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(cursorUpToggleSelection()));

    a = new QAction("Select lower branch", this);
    a->setShortcut(Qt::Key_Down);
    a->setShortcutContext(Qt::WidgetShortcut);
    addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(cursorDown()));

    a = new QAction("Add lower branch to selection", this);
    a->setShortcut(Qt::Key_Down + Qt::SHIFT);
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
        setScrollBarPosTarget(r);   // FIXME-2   mapToScene first?   

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
        qreal new_angle)
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
        new_angle = angle;

    // Calculate required width and height considering rotation of view
    qreal a = new_angle / 180 * M_PI;
    qreal area_w_viewCoord = abs(sin(a) * area.height()) + abs(cos(a) * area.width());
    qreal area_h_viewCoord = abs(sin(a) * area.width()) + abs(cos(a) * area.height());
    qreal z_x = 1.0 * visibleViewCoord.width() / area_w_viewCoord;
    qreal z_y = 1.0 * visibleViewCoord.height() / area_h_viewCoord;

    qreal zf = min (z_x, z_y);

    bool zoomOutRequired = 
        (visibleViewCoord.width() < areaViewCoord.width() ||
         visibleViewCoord.height() < areaViewCoord.height());
    bool zoomInRequired = 
        (visibleViewCoord.width() > areaViewCoord.width() &&
         visibleViewCoord.height() > areaViewCoord.height());

    int animDuration = 2000;
    QEasingCurve easingCurve = QEasingCurve::OutQuint;
    
    //qDebug() << " zoom out: " << zoomOutRequired;
    //qDebug() << " zoom  in: " << zoomInRequired << " zoomFactor=" << zoomFactor << " zf=" << zf;
    if (zoomOutRequired || scaled) {
        setViewCenterTarget(
                area.center(), 
                zf, 
                new_angle,
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
                zoomFactor,
                new_angle,
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
        if (ti->getType() == TreeItem::Image)
            c = ((ImageItem*)ti)->getImageContainer();
        else if (ti->hasTypeBranch())
            c = ((BranchItem*)ti)->getBranchContainer()->getHeadingContainer();
        if (c) {
            if (firstIteration) {
                bbox = c->mapToScene(c->rect()).boundingRect();
                firstIteration = false;
            } else
                bbox = bbox.united(c->mapToScene(c->rect()).boundingRect());
        }
    }
    int new_angle = round_int(angle) % 360;

    if (rotated && selis.count() == 1) {
        if (selis.first()->hasTypeBranch()) {
            BranchContainer *bc = ((BranchItem*)selis.first())->getBranchContainer();
            
            // Avoid rotations > 360°
            setAngle(new_angle);

            qreal rotScene = bc->rotationHeadingInScene();
            int d_angle = new_angle + round_int(rotScene) % 360;
            if (d_angle > 180)
                d_angle = d_angle - 360;
            new_angle = new_angle - d_angle;
        }
    }

    ensureAreaVisibleAnimated(bbox, scaled, rotated, new_angle);
}

void MapEditor::scrollTo(const QModelIndex &index)
{
    if (index.isValid()) {
        TreeItem *ti = static_cast<TreeItem *>(index.internalPointer());
        QRectF r;
        bool scroll = false;
        if ( ti->hasTypeBranch())  {
            r = ((BranchItem*)ti)->getBranchContainer()->headingRect();
            scroll = true;
        }

        // FIXME-2 ME::scrollTo Also check for images, not only branches. 
        // FIXME-2 ME::scrollTo Consider also zoomFactor & rotation
        // FIXME-2 Moving xlink ctrl point to upper edge of view can cause infinite scrolling

        if (scroll) {
            setScrollBarPosTarget(r);
            animateScrollBars();
        }
    }
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
        scale(zoomFactorTarget, zoomFactorTarget).
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

void MapEditor::startAnimation(Container *c, const QPointF &v)  // FIXME-2 only used in ME::autoLayout
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

void MapEditor::setZoomFactorTarget(const qreal &zft)
{
    zoomFactorTarget = zft;
    if (zoomAnimation.state() == QAbstractAnimation::Running)
        zoomAnimation.stop();
    if (settings.value("/animation/use/", true).toBool()) {
        zoomAnimation.setTargetObject(this);
        zoomAnimation.setPropertyName("zoomFactor");
        zoomAnimation.setDuration(
            settings.value("/animation/duration/zoom", 2000).toInt());
        zoomAnimation.setEasingCurve(QEasingCurve::OutQuint);
        zoomAnimation.setStartValue(zoomFactor);
        zoomAnimation.setEndValue(zft);
        zoomAnimation.start();
    }
    else
        setZoomFactor(zft);
}

qreal MapEditor::getZoomFactorTarget() { return zoomFactorTarget; }

void MapEditor::setZoomFactor(const qreal &zf)
{
    zoomFactor = zf;
    updateMatrix();
}

qreal MapEditor::getZoomFactor() { return zoomFactor; }

void MapEditor::setAngleTarget(const qreal &at)
{
    angleTarget = at;
    if (rotationAnimation.state() == QAbstractAnimation::Running)
        rotationAnimation.stop();
    if (settings.value("/animation/use/", true).toBool()) {
        rotationAnimation.setTargetObject(this);
        rotationAnimation.setPropertyName("angle");
        rotationAnimation.setDuration(
            settings.value("/animation/duration/rotation", 2000).toInt());
        rotationAnimation.setEasingCurve(QEasingCurve::OutQuint);
        rotationAnimation.setStartValue(angle);
        rotationAnimation.setEndValue(at);
        rotationAnimation.start();
    }
    else
        setAngle(angleTarget);
}

qreal MapEditor::getAngleTarget() { return angleTarget; }

void MapEditor::setAngle(const qreal &a)
{
    angle = a;
    updateMatrix();
    if (winter)
        winter->updateView();
}

qreal MapEditor::getAngle() { return angle; }

void MapEditor::setViewCenterTarget(const QPointF &p, const qreal &zft,
                                    const qreal &at, const int duration,
                                    const QEasingCurve &easingCurve)
{
    viewCenterTarget = p;
    zoomFactorTarget = zft;
    angleTarget = at;

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
        rotationAnimation.setPropertyName("angle");
        rotationAnimation.setDuration(
            settings.value("/animation/duration/rotation", duration).toInt());
        rotationAnimation.setEasingCurve(easingCurve);
        rotationAnimation.setStartValue(angle);
        rotationAnimation.setEndValue(angleTarget);
        rotationAnimation.start();

        zoomAnimation.setTargetObject(this);
        zoomAnimation.setPropertyName("zoomFactor");
        zoomAnimation.setDuration(
            settings.value("/animation/duration/zoom", duration).toInt());
        zoomAnimation.setEasingCurve(easingCurve);
        zoomAnimation.setStartValue(zoomFactor);
        zoomAnimation.setEndValue(zoomFactorTarget);
        zoomAnimation.start();
    }
    else {
        setAngle(angleTarget);
        setZoomFactor(zft);
        setViewCenter(viewCenterTarget);
    }
}

void MapEditor::setViewCenterTarget()// FIXME-2 add ImageItem and -Container (center on image)
{
    MapItem *selti = (MapItem *)(model->getSelectedItem());
    if (selti) {
        if (selti->hasTypeBranch()) {
            Container *c = ((BranchItem*)selti)->getBranchContainer()->getHeadingContainer();
            setViewCenterTarget(c->mapToScene(c->rect().center()), 1, 0);
        }
    }
}

QPointF MapEditor::getViewCenterTarget() { return viewCenterTarget; }

void MapEditor::setViewCenter(const QPointF &vc) { centerOn(vc); }

QPointF MapEditor::getViewCenter() { return viewCenter; }

void MapEditor::updateMatrix()
{
    QTransform t_zoom;
    t_zoom.scale(zoomFactor, zoomFactor);
    QTransform t_rot;
    t_rot.rotate(angle);
    setTransform(t_zoom * t_rot);
}

void MapEditor::minimizeView() {    // FIXME-2 review if "jumping" can be improved
    // If we only would set scene rectangle to existing items, then
    // view fould "jump", when Qt automatically tries to center.
    // Better consider the currently visible viewport (with slight offset)
    QRectF r = mapToScene(viewport()->geometry()).boundingRect();
    r.translate(-2,-3);
    setSceneRect(scene()->itemsBoundingRect().united(r));
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

void MapEditor::autoLayout()    // FIXME-2 not ported yet to containers
{
    /*
    // Create list with all bounding polygons
    QList<LinkableMapObj *> mapobjects;
    QList<ConvexPolygon> polys;
    ConvexPolygon p;
    QList<Vector> vectors;
    QList<Vector> orgpos;
    QStringList headings; // FIXME-3 testing only
    Vector v;
    BranchItem *bi;
    BranchItem *bi2;
    BranchObj *bo;

    // Outer loop: Iterate until we no more changes in orientation
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
                headings.append(bi->getHeadingPlain());
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
                    headings.append(bi2->getHeadingPlain());
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
                        // FIXME-3 outer loop, "i" get's changed several
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

    model->emitSelectionChanged();
    */
}

TreeItem *MapEditor::findMapItem(
        QPointF p,
        const QList <TreeItem*> &excludedItems,
        bool findNearCenter)
{
    // Search XLinks
    Link *link;
    for (int i = 0; i < model->xlinkCount(); i++) {
        link = model->getXLinkNum(i);
        if (link) {
            XLinkObj *xlo = link->getXLinkObj();
            if (xlo) {
                XLinkObj::SelectionType xlinkSelection = xlo->couldSelect(p);
                if (xlinkSelection == XLinkObj::Path) {
                    // Found path of XLink, now return the nearest XLinkItem of p
                    qreal d0 = Geometry::distance(p, xlo->getBeginPos());
                    qreal d1 = Geometry::distance(p, xlo->getEndPos());
                    if (d0 < d1)
                        return link->getBeginLinkItem();
                    else
                        return link->getEndLinkItem();
                }
                if (xlinkSelection == XLinkObj::C0)
                    return link->getBeginLinkItem();
                if (xlinkSelection == XLinkObj::C1)
                    return link->getEndLinkItem();
            }
        }
    }

    // Search branches (and their childs, e.g. images
    // Start with mapcenter, no images allowed at rootItem
    BranchItem *nearestMapCenter = nullptr;
    qreal d = 0;
    int i = 0;
    BranchItem *bi = model->getRootItem()->getFirstBranch();
    TreeItem *found = nullptr;
    while (bi) {
        found = bi->findMapItem(p, excludedItems);
        if (found)
            return found;

        if (findNearCenter) {
            // Try to find nearest MapCenter
            Container *hc = bi->getBranchContainer()->getHeadingContainer();
            QPointF q = hc->mapToScene(hc->center());
            if (!nearestMapCenter) {
                nearestMapCenter = bi;
                d = Geometry::distance(p, q);
            } else {
                qreal d2 = Geometry::distance(p, q);
                if (d2 < d) {
                    d = d2;
                    nearestMapCenter = bi;
                }
            }
        }

        i++;
        bi = model->getRootItem()->getBranchNum(i);
    }

    if (nearestMapCenter && d < 80 && !excludedItems.contains(nearestMapCenter))
        return nearestMapCenter;

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

void MapEditor::testFunction2()
{
    TreeItem *selti = model->getSelectedItem();
    if (selti)
    {
        if (selti->hasTypeBranch()) {
            BranchContainer *bc = ((BranchItem*)selti)->getBranchContainer();
            //bc->setScrollOpacity(bc->getScrollOpacity() * 0.9);   // FIXME-2 animation test
        } else if (selti->hasTypeImage()) {
            ImageContainer *ic = ((ImageItem*)selti)->getImageContainer();
            qDebug() << ic->info() << ic;
        } else
            qDebug() << "Unknown type";
    } else
        qWarning() << "Nothing selected";
}

void MapEditor::testFunction1()
{
    /*
    */
    BranchItem *selbi = model->getSelectedBranch();
    if (selbi) {
        selbi->getBranchContainer()->printStructure();
    }
    //autoLayout();
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

BranchItem *MapEditor::getBranchDirectAbove(BranchItem *bi)
{
    if (bi) {
        int i = bi->num();
        if (i > 0)
            return bi->parent()->getBranchNum(i - 1);
    }
    return nullptr;
}

BranchItem *MapEditor::getBranchAbove(BranchItem *selbi)
{
    if (selbi) {
        int dz = selbi->depth(); // original depth
        bool invert = false;
        if (selbi->getBranchContainer()->getOrientation() == BranchContainer::LeftOfParent)
            invert = true;

        BranchItem *bi;

        // Look for branch with same parent but directly above
        if (dz == 1 && invert)
            bi = getBranchDirectBelow(selbi);
        else
            bi = getBranchDirectAbove(selbi);

        if (bi)
            // direct predecessor
            return bi;

        // Go towards center and look for predecessor
        while (selbi->depth() > 0) {
            selbi = (BranchItem *)(selbi->parent());
            if (selbi->depth() == 1 && invert)
                bi = getBranchDirectBelow(selbi);
            else
                bi = getBranchDirectAbove(selbi);
            if (bi) {
                // turn
                selbi = bi;
                while (selbi->depth() < dz) {
                    // try to get back to original depth dz
                    bi = selbi->getLastBranch();
                    if (!bi) {
                        return selbi;
                    }
                    selbi = bi;
                }
                return selbi;
            }
        }
    }
    return nullptr;
}

BranchItem *MapEditor::getBranchDirectBelow(BranchItem *bi)
{
    if (bi) {
        int i = bi->num();
        if (i + 1 < bi->parent()->branchCount())
            return bi->parent()->getBranchNum(i + 1);
    }
    return nullptr;
}

BranchItem *MapEditor::getBranchBelow(BranchItem *selbi)
{
    if (selbi) {
        BranchItem *bi;
        int dz = selbi->depth(); // original depth
        bool invert = false;
        if (selbi->getBranchContainer()->getOrientation() == BranchContainer::LeftOfParent)
            invert = true;

        // Look for branch with same parent but directly below
        if (dz == 1 && invert)
            bi = getBranchDirectAbove(selbi);
        else
            bi = getBranchDirectBelow(selbi);
        if (bi)
            // direct successor
            return bi;

        // Go towards center and look for neighbour
        while (selbi->depth() > 0) {
            selbi = (BranchItem *)(selbi->parent());
            if (selbi->depth() == 1 && invert)
                bi = getBranchDirectAbove(selbi);
            else
                bi = getBranchDirectBelow(selbi);
            if (bi) {
                // turn
                selbi = bi;
                while (selbi->depth() < dz) {
                    // try to get back to original depth dz
                    bi = selbi->getFirstBranch();
                    if (!bi) {
                        return selbi;
                    }
                    selbi = bi;
                }
                return selbi;
            }
        }
    }
    return nullptr;
}

BranchItem *MapEditor::getLeftBranch(TreeItem *ti)
{
    if (!ti)
        return nullptr;

    if (ti->hasTypeBranch()) {
        BranchItem *bi = (BranchItem *)ti;
        if (bi->depth() == 0) {
            // Special case: use alternative selection index
            BranchItem *newbi = bi->getLastSelectedBranchAlt();
            if (!newbi) {
                BranchContainer *bc;
                // Try to find a mainbranch left of center
                for (int i = 0; i < bi->branchCount(); i++) {
                    newbi = bi->getBranchNum(i);
                    bc = newbi->getBranchContainer();
                    if (bc &&
                        bc->getOrientation() == BranchContainer::LeftOfParent)
                        break;
                }
            }
            return newbi;
        }
        if (bi->getBranchContainer()->getOrientation() ==
            BranchContainer::RightOfParent)
            // right of center
            return (BranchItem *)(bi->parent());
        else
            // left of center
            if (bi->getType() == TreeItem::Branch)
            return bi->getLastSelectedBranch();
    }

    if (ti->parent() && ti->parent()->hasTypeBranch())
        return (BranchItem *)(ti->parent());
    return nullptr;
}

BranchItem *MapEditor::getRightBranch(TreeItem *ti)
{
    if (!ti)
        return nullptr;

    if (ti->hasTypeBranch()) {
        BranchItem *bi = (BranchItem *)ti;
        if (bi->depth() == 0) {
            // Special case: use alternative selection index
            BranchItem *newbi = bi->getLastSelectedBranch();
            if (!newbi) {
                BranchContainer *bc;
                // Try to find a mainbranch right of center
                for (int i = 0; i < bi->branchCount(); i++) {
                    newbi = bi->getBranchNum(i);
                    bc = newbi->getBranchContainer();
                    if (bc &&
                        bc->getOrientation() == BranchContainer::RightOfParent)
                        qDebug()
                            << "BI found right: " << newbi->getHeadingPlain();
                }
            }
            return newbi;
        }
        if (bi->getBranchContainer()->getOrientation() ==
            BranchContainer::LeftOfParent)
            // left of center
            return (BranchItem *)(bi->parent());
        else
            // right of center
            if (bi->getType() == TreeItem::Branch)
            return (BranchItem *)bi->getLastSelectedBranch();
    }

    if (ti->parent() && ti->parent()->hasTypeBranch())
        return (BranchItem *)(ti->parent());

    return nullptr;
}

void MapEditor::cursorUp()
{
    if (editorState == MapEditor::EditingHeading)
        return;

    BranchItem *selbi = model->getSelectedBranch();
    BranchItem *bi;
    if (selbi) {
        // Exactly one branch is currently selected
        bi = getBranchAbove(selbi);
        if (bi) {
            model->select(bi);
        }
    } else {
        // Nothing selected or already multiple selections
        TreeItem *ti = model->lastToggledItem();
        if (ti && ti->hasTypeBranch()) {
            bi = getBranchAbove( (BranchItem*)ti);
            if (bi)
                model->select(bi);
        }
    }
}

void MapEditor::cursorUpToggleSelection()
{
    if (editorState == MapEditor::EditingHeading)
        return;

    BranchItem *selbi = model->getSelectedBranch();
    BranchItem *bi;

    if (selbi) {
        // Exactly one branch is currently selected
        bi = getBranchAbove(selbi);
        if (bi) model->selectToggle(bi);
    } else {
        // Nothing selected or already multiple selections
        TreeItem *ti = model->lastToggledItem();
        if (ti && ti->hasTypeBranch()) {
            if (lastToggleDirection == toggleUp)
                bi = getBranchAbove( (BranchItem*)ti);
            else
                bi = (BranchItem*)ti;

            if (bi)
                model->selectToggle(bi);
        }
    }
    lastToggleDirection = toggleUp;
}

void MapEditor::cursorDown()
{
    if (editorState == MapEditor::EditingHeading)
        return;

    BranchItem *selbi = model->getSelectedBranch();
    BranchItem *bi;
    if (selbi) {
        // Exactly one branch is currently selected
        bi = getBranchBelow(selbi);
        if (bi) {
            model->select(bi);
        }
    } else {
        // Nothing selected or already multiple selections
        TreeItem *ti = model->lastToggledItem();
        if (ti && ti->hasTypeBranch()) {
            bi = getBranchBelow( (BranchItem*)ti);

            if (bi)
                model->select(bi);
        }
    }
}

void MapEditor::cursorDownToggleSelection()
{
    if (editorState == MapEditor::EditingHeading)
        return;

    BranchItem *selbi = model->getSelectedBranch();
    BranchItem *bi;
    if (selbi) {
        // Exactly one branch is currently selected
        bi = getBranchBelow(selbi);
        if (bi) {
            model->selectToggle(bi);
        }
    } else {
        // Nothing selected or already multiple selections
        TreeItem *ti = model->lastToggledItem();
        if (ti && ti->hasTypeBranch()) {
            if (lastToggleDirection == toggleDown)
                bi = getBranchBelow( (BranchItem*)ti);
            else
                bi = (BranchItem*)ti;

            if (bi)
                model->selectToggle(bi);
        }
    }
    lastToggleDirection = toggleDown;
}

void MapEditor::cursorLeft()
{
    TreeItem *ti = model->getSelectedItem();
    if (!ti) {
        ti = model->lastToggledItem();
        if (!ti) return;
    }

    BranchItem *bi = getLeftBranch(ti);
    if (bi)
        model->select(bi);
    else {
        ImageItem *ii = ti->getFirstImage();
        if (ii)
            model->select(ii);
    }
}

void MapEditor::cursorRight()
{
    TreeItem *ti = model->getSelectedItem();
    if (!ti) {
        ti = model->lastToggledItem();
        if (!ti) return;
    }

    BranchItem *bi = getRightBranch(ti);
    if (bi)
        model->select(bi);
    else {
        ImageItem *ii = ti->getFirstImage();
        if (ii)
            model->select(ii);
    }
}

void MapEditor::cursorFirst() { model->selectFirstBranch(); }

void MapEditor::cursorLast() { model->selectLastBranch(); }

void MapEditor::editHeading()
{
    if (editorState == EditingHeading) {
        editHeadingFinished();
        return;
    }

    BranchItem *bi = model->getSelectedBranch();
    if (bi) {
        VymText heading = bi->getHeading();
        if (heading.isRichText() || bi->getHeadingPlain().contains("\n")) {
            mainWindow->windowShowHeadingEditor();
            ensureSelectionVisibleAnimated();
            return;
        }
        model->setSelectionBlocked(true);

        lineEdit = new QLineEdit;
        QGraphicsProxyWidget *proxyWidget = mapScene->addWidget(lineEdit);
        proxyWidget->setZValue(10000);
        // FIXME-3 get total rotation XXX for BC in scene and do "proxyWidget->setRotation(XXX);
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
        s.replace(QRegExp("\\n"), " "); // Don't paste newline chars
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

    if (e->modifiers() & Qt::ShiftModifier) {
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
            setCursor(QPixmap(":/mode-move-view.png"));
            break;
        default:
            setCursor(Qt::ArrowCursor);
            break;
        }
    }
    QGraphicsView::keyPressEvent(e);
}

void MapEditor::keyReleaseEvent(QKeyEvent *e)
{
    if (!(e->modifiers() & Qt::ControlModifier))
        setCursor(Qt::ArrowCursor);
}

void MapEditor::startPanningView(QMouseEvent *e)
{
    setState(PanningView);
    panning_initialPointerPos = e->globalPos();
    panning_initialScrollBarValues =                  // Used for scrollbars when moving view
        QPoint(horizontalScrollBar()->value(), verticalScrollBar()->value());
    setCursor(HandOpenCursor);
}

void MapEditor::mousePressEvent(QMouseEvent *e)
{
    // Ignore right clicks
    if (e->button() == Qt::RightButton) {
        e->ignore();
        QGraphicsView::mousePressEvent(e);
        return;
    }

    // Check if we need to reset zoomFactor for middle button + Ctrl
    if (e->button() == Qt::MidButton && e->modifiers() & Qt::ControlModifier) {
        setZoomFactorTarget(1);
        setAngleTarget(0);
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
    //if (ti_found) qDebug() << "   ti_found="<<ti_found->getHeading();

    // If Modifier mode "view" is set, all other clicks can be ignored,
    // nothing will be selected
    if ((e->modifiers() & Qt::ShiftModifier) &&
            mainWindow->getModMode() == Main::ModModeMoveView) {
        startPanningView(e);    // FIXME-2 should be set in mouseMove later
        return;
    }

    QString sysFlagName;    // FIXME-2 could be local to ti_found clause below?

    BranchItem *selbi = model->getSelectedBranch();
    BranchContainer *selbc = nullptr;
    if (selbi) {
        selbc = selbi->getBranchContainer();

        // XLink modifier, create new XLink // FIXME-1 move below to ti_found???
        if (mainWindow->getModMode() == Main::ModModeXLink &&
            (e->modifiers() & Qt::ShiftModifier)) {
            setState(DrawingLink);
            tmpLink = new Link(model);
            tmpLink->setBeginBranch(selbi);
            tmpLink->createXLinkObj();
            tmpLink->setStyleBegin("None");
            tmpLink->setStyleEnd("None");
            tmpLink->setEndPoint(movingObj_initialScenePos);
            tmpLink->updateLink();
            return;
        }
    }

    if (ti_found) {
        // Check modifier key (before selecting object!)
        if (e->modifiers() & Qt::ShiftModifier) {
            if (mainWindow->getModMode() == Main::ModModeColor) {
                setState(PickingColor);
                mainWindow->setCurrentColor(ti_found->getHeadingColor());
                if (e->modifiers() & Qt::ControlModifier)
                    model->colorBranch(ti_found->getHeadingColor());
                else
                    model->colorSubtree(ti_found->getHeadingColor());
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
                model->select(ti_found);    // FIXME-2 Selecting "ends" of xLinks does not work properly, scrolls to center of the two end points
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
                movingObj_initialContainerOffset = bc->mapFromScene(movingObj_initialScenePos);
                qDebug() << "ME::mousePressed offset=" << toS(movingObj_initialContainerOffset,0);
            }

            if (mainWindow->getModMode() == Main::ModModeMoveObject &&
                    e->modifiers() & Qt::ShiftModifier) {
                setState(MovingObjectWithoutLinking);
            }
            else
                setState(MovingObject);

            // Set initial position and size of tmpParentContainer
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
            if (e->button() == Qt::MidButton)
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
            if (ti_found->getType() == TreeItem::XLink) {
                XLinkObj *xlo = ((XLinkItem *)ti_found)->getLink()->getXLinkObj();
                if (xlo) {
                    setState(DrawingXLink); // FIXME-2 state correct? creating new xlink or editing existing?
                }
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
                .arg(toS(p_event, 0))
                .arg(toS(e->pos())));

    // Allow selecting text in QLineEdit if necessary
    if (model->isSelectionBlocked()) {
        e->ignore();
        QGraphicsView::mouseMoveEvent(e);
        return;
    }

    // Pan view
    if (editorState == PanningView) {
        QPoint p = e->globalPos();
        QPoint v_pan;
        v_pan.setX(-p.x() + panning_initialPointerPos.x());
        v_pan.setY(-p.y() + panning_initialPointerPos.y());
        horizontalScrollBar()->setSliderPosition(
            (int)(panning_initialScrollBarValues.x() + v_pan.x()));
        verticalScrollBar()->setSliderPosition(
            (int)(panning_initialScrollBarValues.y() + v_pan.y()));

        // Avoid flickering
        scrollBarPosAnimation.stop();
        viewCenterAnimation.stop();
        rotationAnimation.stop();
        // zoomAnimation.stop();    // FIXME-2 why no longer used?

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
         editorState == DrawingXLink)) {
        int margin = 50;

        // Check if we have to scroll
        vPan.setX(0);
        vPan.setY(0);
        if (e->y() >= 0 && e->y() <= margin)
            vPan.setY(e->y() - margin);
        else if (e->y() <= height() && e->y() > height() - margin)
            vPan.setY(e->y() - height() + margin);
        if (e->x() >= 0 && e->x() <= margin)
            vPan.setX(e->x() - margin);
        else if (e->x() <= width() && e->x() > width() - margin)
            vPan.setX(e->x() - width() + margin);

        moveObject(e, p_event);
    } // selection && moving_obj

    // Draw a link from one branch to another
    if (editorState == DrawingLink) {
        tmpLink->setEndPoint(p_event);
        tmpLink->updateLink();
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
                // qWarning() << "ME::moveObject " << targetItem->getHeadingPlain() << "is child of " << ti->getHeadingPlain();
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
                        qDebug() << "ME::mO bc->orient=Undefined for " << bc->info();
                        tmpParentContainer->setOrientation(BranchContainerBase::RightOfParent);
                    } else
                        tmpParentContainer->setOrientation(bc->getOrientation());
                }

                if (tmpParentContainer->branchCount() == 0 || 
                    bc->parentItem() != tmpParentContainer->getBranchesContainer()) {

                    // For additional floating containers use scenePos, so that bc_first
                    // could be moved with additional containers keeping their positions
                    bc->setOriginalPos();
                    bc->setOriginalOrientation();   // Also sets originalParentBranchContainer
                    qDebug() << "ME::mO adding to tpc: " << bc->info();
                    tmpParentContainer->addToBranchesContainer(bc);

                    // Save position of children branches in case we only want to
                    // move this branch and keep children unchanged using CTRL modifier
                    Container *c = bc->getBranchesContainer();
                    if (bc->hasFloatingBranchesLayout() && c)
                        foreach(QGraphicsItem *i, c->childItems())
                            ((BranchContainer*)i)->setOriginalScenePos();
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
            else if (ti->getType() == TreeItem::XLink) {
                // Move XLink control point
                XLinkObj *xlo = ((XLinkItem *)ti)->getXLinkObj();
                if (xlo) {
                    xlo->setSelectedCtrlPoint(p_event); // FIXME-3 Missing savestate
                    model->setChanged();
                    model->emitSelectionChanged();
                }
            }
            else
                qWarning("ME::moveObject  Huh? I'm confused. No BC, IC or XLink moved");
        }
        qDebug() << "ME::mO filled tPC. branch count=" << tmpParentContainer->childBranches().count();
    } // add to tmpParentContainer

    BranchContainer *targetBranchContainer = nullptr;

    // Check if we could link and position tmpParentContainer
    if (targetItem && targetItem->hasTypeBranch() &&
                !(mainWindow->getModMode() == Main::ModModeMoveObject &&
                    (e->modifiers() & Qt::ShiftModifier))) {

        setState(MovingObjectTmpLinked);

        targetBranchContainer = ((BranchItem*)targetItem)->getBranchContainer();

        Container *targetRefContainer = targetBranchContainer->getBranchesContainer();
        Container *movingRefContainer = tmpParentContainer;
        Container::PointName targetRefPointName;
        Container::PointName movingRefPointName;
        QPointF linkOffset;                     // Distance for temporary link

        if (e->modifiers() & Qt::ShiftModifier) { // FIXME-2 Find nice position. Maybe using ME::getBranchAbove/Below (maybe move these to VM...)
            targetBranchContainer = targetBranchContainer->parentBranchContainer();

            if (targetBranchContainer->getOrientation() == BranchContainer::RightOfParent) {
                // Shift modifier: Link right above
                targetRefPointName = Container::TopRight;
                movingRefPointName = Container::BottomLeft;
                linkOffset = QPointF(model->mapDesign()->linkWidth(), 0);
            } else if (targetBranchContainer->getOrientation() == BranchContainer::LeftOfParent) {
                    // Shift modifier: Link left above
                    targetRefPointName = Container::TopLeft;
                    movingRefPointName = Container::BottomRight;
                    linkOffset = QPointF(- model->mapDesign()->linkWidth(), 0);
            } else {
                qDebug() << "ME::moveObject -  targetBranchContainer has undefined orientation with shift modifier"; // FIXME-0
            }
        } else if (e->modifiers() & Qt::ControlModifier) {
            targetBranchContainer = targetBranchContainer->parentBranchContainer();
            if (targetBranchContainer->getOrientation() == BranchContainer::RightOfParent) {
                // Shift modifier: Link right below
                targetRefPointName = Container::BottomRight;
                movingRefPointName = Container::TopLeft;
                linkOffset = QPointF(model->mapDesign()->linkWidth(), 0);
            } else if (targetBranchContainer->getOrientation() == BranchContainer::LeftOfParent) {
                    // Shift modifier: Link left below
                    targetRefPointName = Container::BottomLeft;
                    movingRefPointName = Container::TopRight;
                    linkOffset = QPointF(- model->mapDesign()->linkWidth(), 0);
            } else {
                qDebug() << "ME::moveObject -  targetBranchContainer has undefined orientation with shift modifier"; // FIXME-0
            }
        } else {
            // No modifier used, temporary link to target itself
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
            } else {
                qDebug() << "ME::moveObject -  targetBranchContainer has undefined orientation without modifier"; // FIXME-0
                targetRefPointName = Container::Center;
                movingRefPointName = Container::Center;   // FIXME-0 Could also be close to mid points or similar, e.g. MapCenter
            }
        }

        if (!targetRefContainer)
            targetRefContainer = ((BranchItem*)targetItem)->getBranchContainer();    // FIXME-0 if tBC has no children, assume whole tBC. What about bounded images, then?

        // Align tPC to point in target, which has been selected above
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
        // Update state of MapEditor
        if (mainWindow->getModMode() == Main::ModModeMoveObject &&
                e->modifiers() & Qt::ShiftModifier)
            setState(MovingObjectWithoutLinking);
        else
            setState(MovingObject);

        // tmpParentContainer to pointer position:
        tmpParentContainer->setPos(p_event - movingObj_initialContainerOffset);

        if (tmpParentContainer->movingState() == BranchContainerBase::TemporaryLinked)
            tmpParentContainer->setMovingState(BranchContainerBase::Moving);

        updateUpLinksRequired = true;
    }

    // Update children branch containers (for updating links later)
    foreach (BranchContainer *bc, tmpParentContainer->childBranches()) {
        if (tmpParentContainer->movingState() == BranchContainerBase::TemporaryLinked)
            bc->setMovingState(BranchContainerBase::TemporaryLinked, targetBranchContainer);
        else
            bc->setMovingState(BranchContainerBase::Moving);
    }

    // When moving MapCenters with Ctrl  modifier, don't move mainbranches (in scene)   // FIXME-2 not only MCs, but all floating branches
    if (e->modifiers() & Qt::ControlModifier) {
        foreach(BranchContainer *bc, tmpParentContainer->childBranches()) {
            BranchItem *bi = bc->getBranchItem();
            if (bi->depth() >= 0 && bc->hasFloatingBranchesLayout()) {
                foreach(BranchContainer *bc2, bc->childBranches()) {
                    bc2->setPos( bc->sceneTransform().inverted().map(bc2->getOriginalPos()));
                    bc2->updateUpLink();
                }
            }
        }
    }

    if (tmpParentContainer->childBranches().count() > 0)
        // If ME::moveObject is called AFTER tPC has been filled previously, 
        // bc_first still might be unset here
        bc_first = tmpParentContainer->childBranches().first();

    // Set orientation
    BranchContainer::Orientation newOrientation;
    Container *tpc_bc = tmpParentContainer->getBranchesContainer();
    if (targetBranchContainer && tpc_bc && !tpc_bc->childItems().contains(targetBranchContainer)) {
        // tmpParentContainer has children and these do NOT contain targetBranchContainer
        if (targetBranchContainer->hasFloatingBranchesLayout()) {
            if (p_event.x() > targetBranchContainer->pos().x())
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
            //if (tmpParentContainer->getOrientation() == BranchContainer::LeftOfParent)
                if (p_event.x() > bc_first->getOriginalParentPos().x())
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

        movingObj_initialContainerOffset.setX( - movingObj_initialContainerOffset.x());
        tmpParentContainer->setPos(p_event - movingObj_initialContainerOffset);
    }

    if (repositionRequired)
        tmpParentContainer->reposition();
    else if (updateUpLinksRequired)
        foreach(BranchContainer *bc, tmpParentContainer->childBranches())   
            bc->updateUpLink();

    model->repositionXLinks();

    scene()->update();
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
    if (editorState == DrawingLink) {
        setState(Neutral);

        TreeItem *seli;
        if (movingItems.count() > 0)
            seli = movingItems.first();
        else
            seli = nullptr;

        // Check if we are over another branch
        if (destinationBranch) {
            tmpLink->setEndBranch(destinationBranch);
            tmpLink->activate();
            tmpLink->updateLink();
            if (model->createLink(tmpLink)) {
                model->saveState(
                    tmpLink->getBeginLinkItem(), "remove ()", seli,
                    QString("addXLink (\"%1\",\"%2\",%3,\"%4\",\"%5\")")
                        .arg(model->getSelectString(tmpLink->getBeginBranch()))
                        .arg(model->getSelectString(tmpLink->getEndBranch()))
                        .arg(tmpLink->getPen().width())
                        .arg(tmpLink->getPen().color().name())
                        .arg(penStyleToString(tmpLink->getPen().style())),
                    QString("Adding Link from %1 to %2")
                        .arg(model->getObjectName(seli))
                        .arg(model->getObjectName(destinationBranch)));
                return;
            }
        }
        delete (tmpLink);
        tmpLink = nullptr;
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

            model->relinkBranches(
                    movingBranches,
                    dst_branch,
                    dst_num);

            // If dst is scrolled, select it
            if (dst_branch->isScrolled())
                model->select(dst_branch);

            // Loop over images // FIXME-2 refactor in VM similar to relinkBranches
            foreach(ImageContainer *ic, tmpParentContainer->childImages()) {
                ImageItem *ii = ic->getImageItem();
                model->relinkImage(ii, destinationBranch);
            }
        } else {
            // Branches moved, but not relinked
            QPointF t = p - movingObj_initialScenePos;    // Defined in mousePressEvent

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
                        model->saveState(
                            bi, QString("setPos%1").arg(toS(bc->getOriginalPos())),
                            bi, QString("setPos%1").arg(toS(bc->pos())));
                    } else {    // FIXME-0 only animate snappack if not Ctrl-moving e.g. MC
                        animationContainers << bc;
                        animationCurrentPositions << bc->pos();
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
            model->saveState(ii, "setPos " + pold, ii,
                             "setPos " + pnow,
                             QString("Move %1 to %2")
                                 .arg(model->getObjectName(ii))
                                 .arg(pnow));

        } // Image moved, but not relinked

        if (repositionNeeded) {
            model->reposition();    // FIXME-3 really reposition whole model? Or only affected MapCenters?
            model->emitSelectionChanged();
        }

        // Finally resize scene, if needed
        scene()->update();
        vPan = QPoint();
    } // MovingObject or MovingObjectTmpLinked
    else
        // maybe we moved View: set old cursor
        setCursor(Qt::ArrowCursor);

    if (editorState != EditingHeading)
        setState(Neutral); // Continue editing after double click!

    movingItems.clear();
    QGraphicsView::mouseReleaseEvent(e);
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

void MapEditor::wheelEvent(QWheelEvent *e)  // FIXME-2 stop current animations
{
    if (e->modifiers() & Qt::ControlModifier &&
        e->angleDelta().y() != 0) {
        QPointF p = mapToScene(e->position().toPoint());
        if (e->angleDelta().y() > 0)
            // setZoomFactorTarget (zoomFactorTarget*1.15);
            setViewCenterTarget(p, zoomFactorTarget * 1.15, angleTarget);
        else
            // setZoomFactorTarget (zoomFactorTarget*0.85);
            setViewCenterTarget(p, zoomFactorTarget * 0.85, angleTarget);
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
            qDebug() << "============== mimeData ===================";
            qDebug() << "has-img : " << event->mimeData()->hasImage();
            qDebug() << "has-urls: " << event->mimeData()->hasUrls();
            qDebug() << "    text: " << event->mimeData()->text();
            qDebug() << "===========================================";
        }

        if (event->mimeData()->hasUrls()) {
            // Try text representation first, which works on windows, but in
            // Linux only for https, not local images
            QString url = event->mimeData()->text();
            if (url.isEmpty()) {
                QByteArray ba =
                    event->mimeData()->urls().first().path().toLatin1();
                QByteArray ba2;
                for (int i = 0; i < ba.count(); i++)
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
                            model->setURL(url);

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

void MapEditor::setState(EditorState s)
{
    editorState = s;
    /* if (debug)
    {
        QString s;
        switch (state)
        {
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
        case DrawingLink:
            s = "DrawingLink";
            break;
        default:
            s = "Unknown editor state";
            qDebug() << "MapEditor::setState" << s;
            break;
        }
        qDebug() << "MapEditor: State " << s << " of " << model->getMapName();
    }
    */
}

MapEditor::EditorState MapEditor::state() { return editorState; }


void MapEditor::updateData(const QModelIndex &sel)
{
    TreeItem *ti = static_cast<TreeItem *>(sel.internalPointer());

    /* testing
        qDebug() << "ME::updateData";
        if (!ti)
        {
        qDebug() << "  ti=nullptr";
        return;
        }
        qDebug() << "  ti="<<ti;
        qDebug() << "  h="<<ti->getHeadingPlain();
    */

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

// FIXME-2 Feature: updateSelection - New settings (maybe with keys to toggle) autorotation to adapt view to selection: Adapt to heading/subtree
void MapEditor::updateSelection(QItemSelection newsel, QItemSelection dsel)
{
    /*
    qDebug() << "ME::updateSel";
    qDebug() << "  newsel=" << newsel;
    qDebug() << "  dsel=" << dsel;
    */
    QList<MapItem *> itemsSelected;

    // Select objects
    foreach (QModelIndex ix, newsel.indexes()) {
        MapItem *mi = static_cast<MapItem *>(ix.internalPointer());
        if (mi->hasTypeBranch() || mi->getType() == TreeItem::Image ||
            mi->getType() == TreeItem::XLink) {
            if (!itemsSelected.contains(mi)) {
                itemsSelected.append(mi);
                if (mi->hasTypeBranch())
                    ((BranchItem*)mi)->getBranchContainer()->select();
                if (mi->hasTypeImage())
                    ((ImageItem*)mi)->getImageContainer()->select();
                if (mi->hasTypeXLink())
                    ((XLinkItem*)mi)->getXLinkObj()->select(
			model->mapDesign()->selectionPen(),
			model->mapDesign()->selectionBrush());
            }
        }
        /* FIXME-2 ME::updateSelection - hide links of unselected objects
         * also for unselect below
        lmo = mi->getLMO(); // FIXME-X xlink does return nullptr
        if (lmo)
            mi->getLMO()->updateVisibility();
        */
    }

    // Unselect objects (if not part of selection)
    foreach (QModelIndex ix, dsel.indexes()) {
        MapItem *mi = static_cast<MapItem *>(ix.internalPointer());
        //qDebug() << "ME::updateSel   deselecting mi=" << mi << mi->getHeadingPlain();
        if (mi->hasTypeBranch() || mi->getType() == TreeItem::Image ||
            mi->getType() == TreeItem::XLink) {
            if (!itemsSelected.contains(mi)) {
                if (mi->hasTypeBranch())
                    ((BranchItem*)mi)->getBranchContainer()->unselect();
                if (mi->hasTypeImage())
                    ((ImageItem*)mi)->getImageContainer()->unselect();
                if (mi->hasTypeXLink())
                    ((XLinkItem*)mi)->getXLinkObj()->unselect();
            }
        }
    }

    // Show count of multiple selected items
    int selCount = model->getSelectionModel()->selection().indexes().count();
    if (selCount > 1)
        mainWindow->statusMessage(
            tr("%1 items selected","Status message when selecting multiple items").arg(selCount));
    else
        mainWindow->statusMessage("");
}

void MapEditor::updateSelection()
{
    QList<MapItem *> itemsSelected;

    // Select objects
    foreach (QModelIndex ix, model->getSelectionModel()->selection().indexes()) {
        MapItem *mi = static_cast<MapItem *>(ix.internalPointer());
        if (mi->hasTypeBranch() || mi->getType() == TreeItem::Image ||
            mi->getType() == TreeItem::XLink) {
                if (mi->hasTypeBranch())
                    ((BranchItem*)mi)->getBranchContainer()->select();
                if (mi->hasTypeImage())
                    ((ImageItem*)mi)->getImageContainer()->select();
                if (mi->hasTypeXLink())
                    ((XLinkItem*)mi)->getXLinkObj()->select(
			model->mapDesign()->selectionPen(),
			model->mapDesign()->selectionBrush());
        }
    }
}

