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
#include "link-container.h"
#include "mainwindow.h"
#include "misc.h"
#include "shortcuts.h"
#include "warningdialog.h"
#include "winter.h"
#include "xlinkitem.h"

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

    if (debug) {
        // Add origin for debugging
        QGraphicsRectItem *x_axis = new QGraphicsRectItem(-100, 0, 200, 1 );
        QGraphicsRectItem *y_axis = new QGraphicsRectItem(0, -100, 1, 200 );
        x_axis->setBrush(Qt::NoBrush);
        y_axis->setBrush(Qt::NoBrush);
        x_axis->setPen(QColor(Qt::blue));
        y_axis->setPen(QColor(Qt::blue));

        mapScene->addItem(x_axis);
        mapScene->addItem(y_axis);
    }

    zoomFactor = zoomFactorTarget = 1;
    angle = angleTarget = 0;

    model = vm;
    model->registerEditor(this);

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
    tmpParentContainer = new BranchContainer (mapScene, nullptr, nullptr);
    tmpParentContainer->setZValue(1000);    // See also z-values in mapobj.h
    tmpParentContainer->setName("tmpParentContainer");
    tmpParentContainer->setType(Container::TmpParent);
    tmpParentContainer->setLayout(Container::FloatingBounded);
    tmpParentContainer->setBranchesContainerLayout(Container::FloatingBounded);
    tmpParentContainer->setBrush(Qt::NoBrush);
    tmpParentContainer->setPen(QPen(Qt::NoPen));
    tmpParentContainer->reposition();

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

    // Selections
    selectionColor = QColor(255, 255, 0);
    selectionColor.setAlpha(150);

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
        winter = NULL;
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

void MapEditor::scrollTo(const QModelIndex &index)  // FIXME-2 also consider images
{
    if (index.isValid()) {
        TreeItem *ti = static_cast<TreeItem *>(index.internalPointer());
        QRectF r;
        bool scroll = false;
        if ( ti->hasTypeBranch())  {
            r = ((BranchItem*)ti)->getBranchContainer()->getHeadingRect();
            scroll = true;
        }

        // FIXME-2 Also check for images, not just branches
        
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
    QRectF viewRect = transform().mapRect(rect);

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

        if (c->containerType() == Container::Branch)
            ((BranchContainer*)c)->updateUpLink();

        if (!c->isAnimated()) 
            animatedContainers.removeAll(c);
    }
    model->emitSelectionChanged();

    if (!animatedContainers.isEmpty())
        animationTimer->start(animationInterval);
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

void MapEditor::setViewCenterTarget()// FIXME-1 add ImageItem and -Container
{
    MapItem *selti = (MapItem *)(model->getSelectedItem());
    if (selti) {
        if (selti->hasTypeBranch())
            setViewCenterTarget(((BranchItem*)selti)->getBranchContainer()->rect().center(), 1, 0);
    }
}

QPointF MapEditor::getViewCenterTarget() { return viewCenterTarget; }

void MapEditor::setViewCenter(const QPointF &vc) { centerOn(vc); }

QPointF MapEditor::getViewCenter() { return viewCenter; }

void MapEditor::updateMatrix()
{
    double a = M_PI / 180 * angle;
    double sina = sin((double)a);
    double cosa = cos((double)a);

    QMatrix zm(zoomFactor, 0, 0, zoomFactor, 0, 0);
    // QMatrix translationMatrix(1, 0, 0, 1, 50.0, 50.0);
    QMatrix rm(cosa, sina, -sina, cosa, 0, 0);
    setMatrix(zm * rm);
}

void MapEditor::minimizeView() { 
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
        printer->setOrientation(QPrinter::Landscape);
    else
        // recommend portrait
        printer->setOrientation(QPrinter::Portrait);

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
            frame->setZValue(0);
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

QRectF MapEditor::getTotalBBox()    // FIXME-2 really needed? Overlaps with scene and VM...
{
    minimizeView();
    return sceneRect();
}

QImage MapEditor::getImage(QPointF &offset)
{
    QRectF mapRect = getTotalBBox(); // minimized sceneRect

    int d = 10; // border
    offset = QPointF(mapRect.x() - d / 2, mapRect.y() - d / 2);
    QImage pix(mapRect.width() + d, mapRect.height() + d, QImage::Format_RGB32);

    QPainter pp(&pix);
    pp.setRenderHints(renderHints());
    mapScene->render(&pp,
                     // Destination:
                     QRectF(0, 0, mapRect.width() + d, mapRect.height() + d),
                     // Source in scene:
                     QRectF(mapRect.x() - d / 2, mapRect.y() - d / 2,
                            mapRect.width() + d, mapRect.height() + d));
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

TreeItem *MapEditor::findMapItem(QPointF p, const QList <TreeItem*> &excludedItems) 
{
    // Search XLinks
    Link *link;
    for (int i = 0; i < model->xlinkCount(); i++) {
        link = model->getXLinkNum(i);
        if (link) {
            XLinkObj *xlo = link->getXLinkObj();
            if (xlo && xlo->isInClickBox(p)) {
                // Found XLink, now return the nearest XLinkItem of p
                qreal d0 = Geometry::distance(p, xlo->getBeginPos());
                qreal d1 = Geometry::distance(p, xlo->getEndPos());
                if (d0 > d1)
                    return link->getBeginLinkItem();
                else
                    return link->getEndLinkItem();
            }
        }
    }

    // Search branches (and their childs, e.g. images
    // Start with mapcenter, no images allowed at rootItem
    int i = 0;
    BranchItem *bi = model->getRootItem()->getFirstBranch();
    TreeItem *found = nullptr;
    while (bi) {
        found = bi->findMapItem(p, excludedItems);
        if (found)
            return found;
        i++;
        bi = model->getRootItem()->getBranchNum(i);
    }
    return nullptr;
}

BranchItem *MapEditor::findMapBranchItem(QPointF p, const QList <TreeItem*> &excludedItems) 
{
    TreeItem *ti = findMapItem(p, excludedItems);
    if (ti && ti->hasTypeBranch())
        return (BranchItem*)ti;
    else
        return nullptr;
}

void MapEditor::testFunction1()
{
    BranchItem *selbi = model->getSelectedBranch();
    if (!selbi)
        qWarning() << "Nothing selected";
    else {
        BranchContainer *bc = selbi->getBranchContainer();
        qDebug() << bc->info();
        Container *c = bc->getBranchesContainer();
        if (c)
            for (int i=0; i < c->childItems().count(); i++)
                qDebug() << "  i=" << i << ((Container*)c->childItems().at(i))->info();
        c = bc->getImagesContainer();
        if (c)
            for (int i=0; i < c->childItems().count(); i++)
                qDebug() << "  i=" << i << ((Container*)c->childItems().at(i))->info();
    }
}

void MapEditor::testFunction2() { autoLayout(); }

void MapEditor::toggleWinter() // FIXME-2 Not working yet with containers (also shaking snow from branch!)
{
    if (winter) {
        delete winter;
        winter = NULL;
    }
    else {
        winter = new Winter(this);
        QList<QRectF> obstacles;
        BranchObj *bo;
        BranchItem *cur = NULL;
        BranchItem *prev = NULL;
        model->nextBranch(cur, prev);
        while (cur) {
            if (!cur->hasHiddenExportParent()) {
                // Branches
                bo = (BranchObj *)(cur->getLMO());
                if (bo && bo->isVisibleObj())
                    obstacles.append(bo->getBBox());
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
    if (state() == MapEditor::EditingHeading)
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
    if (state() == MapEditor::EditingHeading)
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
    if (state() == MapEditor::EditingHeading)
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
    if (state() == MapEditor::EditingHeading)
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
    if (state() == EditingHeading) {
        editHeadingFinished();
        return;
    }

    BranchItem *bi = model->getSelectedBranch();
    if (bi) {
        VymText heading = bi->getHeading();
        if (heading.isRichText()) {
            mainWindow->windowShowHeadingEditor();
            return;
        }
        model->setSelectionBlocked(true);

        lineEdit = new QLineEdit;
        QGraphicsProxyWidget *pw = mapScene->addWidget(lineEdit);
        pw->setZValue(Z_LINEEDIT);
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
            tl = bc->getHeadingRect().topLeft();
            br = tl + QPointF(w, h);
        }
        else {
            br = bc->getHeadingRect().bottomRight();
            tl = br - QPointF(w, h);
        }
        QRectF r(tl, br);
        lineEdit->setGeometry(r.toRect());

        setScrollBarPosTarget(r);
        scene()->update();

        // Set focus to MapEditor first
        // To avoid problems with Cursor up/down
        setFocus();

        animateScrollBars();
        lineEdit->setText(heading.getTextASCII());
        lineEdit->setFocus();
        lineEdit->selectAll(); // Hack to enable cursor in lineEdit
        lineEdit->deselect();  // probably a Qt bug...
        setState(EditingHeading);
    }
}

void MapEditor::editHeadingFinished()
{
    if (state() != EditingHeading || !lineEdit ) {
        qWarning() << "ME::editHeadingFinished not editing heading!";
        return;
    }

    setState(Neutral);
    // lineEdit->releaseKeyboard();
    lineEdit->clearFocus();
    QString s = lineEdit->text();
    s.replace(QRegExp("\\n"), " "); // Don't paste newline chars
    if (s.length() == 0)
        s = " "; // Don't allow empty lines, which would screw up drawing
    model->setHeadingPlainText(s);
    model->setSelectionBlocked(false);
    delete (lineEdit);
    lineEdit = nullptr;

    animateScrollBars();

    // Maybe reselect previous branch
    mainWindow->editHeadingFinished(model);

    // Autolayout to avoid overlapping branches with longer headings
    if (settings.value("/mainwindow/autoLayout/use", "true") == "true")
        autoLayout();
}

void MapEditor::contextMenuEvent(QContextMenuEvent *e)
{
    // Lineedits are already closed by preceding
    // mouseEvent, we don't need to close here.

    QPointF p = mapToScene(e->pos());
    TreeItem *ti = findMapItem(p);

    if (ti) { // MapObj was found
        model->select(ti);

        BranchItem *selbi = model->getSelectedBranch();

        // Context Menu
        if (selbi) {
            QString sysFlagName;
            /* FIXME-1 context menu not work yet with task flag
            QUuid uid = ((BranchObj *)lmo)->findSystemFlagUidByPos(p);
            if (!uid.isNull()) {
                Flag *flag = systemFlagsMaster->findFlagByUid(uid);
                if (flag)
                    sysFlagName = flag->getName();
            }
            */

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
    if (state() == EditingHeading) editHeadingFinished();

    /*
    qDebug() << "ME::mouse pressed\n";
    qDebug() << "   ti_found=" << ti_found;
    //if (ti_found) qDebug() << "   ti_found="<<ti_found->getHeading();
    qDebug() << " flag=" << sysFlagName;
    */

    // If Modifier mode "view" is set, all other clicks can be ignored, 
    // nothing will be selected
    if ((e->modifiers() & Qt::ShiftModifier) && 
            mainWindow->getModMode() == Main::ModModeMoveView) {
        startPanningView(e);    // FIXME-2 should be set in mouseMove later
        return;
    }

    QString sysFlagName;    // FIXME-2 could be local to ti_found clause below?

    /* FIXME-2 not supported yet // XLink modifier, create new XLink
    BranchItem *selbi = model->getSelectedBranch();
    if (selbi && mainWindow->getModMode() == Main::ModModeXLink &&
        (e->modifiers() & Qt::ShiftModifier)) {
        setState(DrawingLink);
        tmpLink = new Link(model);
        tmpLink->setBeginBranch(selbi);
        tmpLink->createMapObj();
        tmpLink->setStyleBegin("None");
        tmpLink->setStyleEnd("None");
        tmpLink->setEndPoint(movingObj_initialScenePos);
        tmpLink->updateLink();
        return;
    }
    */

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

        /* 
        // FIXME-2 check for flags
        QUuid uid = ((BranchObj *)lmo_found)->findSystemFlagUidByPos(p);
        if (!uid.isNull()) {
            Flag *flag = systemFlagsMaster->findFlagByUid(uid);
            if (flag)
                sysFlagName = flag->getName();
        }
        */

        // Check vymlink  modifier (before selecting object!)   // FIXME-2 why here and not below with other system flags? Why before selecting?
        if (sysFlagName == "system-vymLink") {
            model->select(ti_found);
            if (e->modifiers() & Qt::ControlModifier)
                mainWindow->editOpenVymLink(true);
            else
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
        movingItems = model->getSelectedItems();

        int i = movingItems.indexOf(ti_found);
        if (i > 0)
            // Make sure currently clicked item is first in list
            movingItems.move(i, 0);

        // Left Button	    Move Branches
        if (e->button() == Qt::LeftButton) {
            // No system flag clicked, take care of moving modes or simply
            // start moving
            if (ti_found->hasTypeBranch())
            {
                BranchContainer *bc = ((BranchItem*)ti_found)->getBranchContainer();
                movingObj_initialContainerOffset = bc->mapFromScene(movingObj_initialScenePos);
            }

            if (mainWindow->getModMode() == Main::ModModeMoveObject &&
                    e->modifiers() & Qt::ShiftModifier) {
                setState(MovingObjectWithoutLinking);
            }
            else
                setState(MovingObject);
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

    // Take care of  remaining system flags _or_ modifier modes // FIXME-2
    /*
    if (lmo_found) {
        if (!sysFlagName.isEmpty()) {
            // systemFlag clicked
            if (sysFlagName.contains("system-url")) {
                if (e->modifiers() & Qt::ControlModifier)
                    mainWindow->editOpenURLTab();
                else
                    mainWindow->editOpenURL();
            }
            else if (sysFlagName == "system-vymLink") {
                if (e->modifiers() & Qt::ControlModifier)
                    mainWindow->editOpenVymLink(true);
                else
                    mainWindow->editOpenVymLink(false);
                // tabWidget may change, better return now
                // before segfaulting...
            }
            else if (sysFlagName == "system-note")
                mainWindow->windowToggleNoteEditor();
            else if (sysFlagName == "hideInExport")
                model->toggleHideExport();
            else if (sysFlagName.startsWith("system-task-"))
                model->cycleTaskStatus();
            return;
        }
        else {
            // Take care of xLink: Open context menu with targets
            // if clicked near to begin of xlink
            if (ti_found->xlinkCount() > 0 &&
                ti_found->getType() != TreeItem::MapCenter &&
                lmo_found->getBBox().width() > 30) {
                if ((lmo_found->getOrientation() !=
                         BranchContainer::RightOfParent && // FIXME-x already changed to Container here
                     p.x() < lmo_found->getBBox().left() + 10) ||
                    (lmo_found->getOrientation() !=
                         BranchContainer::LeftOfParent && // FIXME-x already changed to Container here
                     p.x() > lmo_found->getBBox().right() - 10)) {
                    // FIXME-4 similar code in mainwindow::updateActions
                    QMenu menu;
                    QList<QAction *> alist;
                    QList<BranchItem *> blist;
                    for (int i = 0; i < ti_found->xlinkCount(); i++) {
                        XLinkItem *xli = ti_found->getXLinkItemNum(i);
                        BranchItem *bit = xli->getPartnerBranch();
                        if (bit)
                            alist.append(
                                new QAction(ti_found->getXLinkItemNum(i)
                                                ->getPartnerBranch()
                                                ->getHeadingPlain(),
                                            &menu));
                    }
                    menu.addActions(alist);
                    QAction *ra = menu.exec(e->globalPos());
                    if (ra)
                        model->select(blist.at(alist.indexOf(ra)));
                    while (!alist.isEmpty()) {
                        QAction *a = alist.takeFirst();
                        delete a;
                    }
                    return;
                }
            }
        }
    }   // system flags or modModes
    */

    /*
    else { // No lmo found, check XLinks
        if (ti_found) {
            if (ti_found->getType() == TreeItem::XLink) {
                // FIXME-2 xlink not supported yet with containers
                XLinkObj *xlo = (XLinkObj *)((MapItem *)ti_found)->getMO();
                if (xlo) {
                    setState(DrawingXLink);
                    int i = xlo->ctrlPointInClickBox(p);
                    if (i >= 0)
                        xlo->setSelection(i);
                    // FIXME-2 remove variable (also in header)? still used when panning!
                    // Note: movingObj_offset renamed to movingObj_PointerPos and QPoint instead of QPOintF meanwhile // FIXME-4
                    movingObj_offset.setX(p.x() - xlo->x());
                    movingObj_offset.setY(p.y() - xlo->y());
                    movingObj_orgPos.setX(xlo->x());
                    movingObj_orgPos.setY(xlo->y());
                }
            }
        }
    }
    */
}

void MapEditor::mouseMoveEvent(QMouseEvent *e)  // FIXME-1  Shift to only move MC or floating parent not implemented yet
{
    QPointF p_event = mapToScene(e->pos());

    // Show mouse position for debugging in statusBar
    if (debug && e->modifiers() & Qt::ControlModifier)
        mainWindow->statusMessage(
            QString("ME::mouseMoveEvent  Scene coord: %1  viewport coord: %2")
                .arg(qpointFToString(p_event))
                .arg(qpointFToString(e->pos())));

    // Allow selecting text in QLineEdit if necessary
    if (model->isSelectionBlocked()) {
        e->ignore();
        QGraphicsView::mouseMoveEvent(e);
        return;
    }

    // Pan view
    if (state() == PanningView) {
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
        (state() == MovingObject || 
         state() == MovingObjectTmpLinked || 
         state() == MovingObjectWithoutLinking ||
         state() == DrawingXLink)) {
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
    if (state() == DrawingLink) {
        tmpLink->setEndPoint(p_event);
        tmpLink->updateLink();
    }
}

void MapEditor::moveObject(QMouseEvent *e, const QPointF &p_event)
{
    // If necessary pan the view using animation
    if (!panningTimer->isActive())
        panningTimer->start(50);

    // reset cursor if we are moving and don't copy

    // Check if we could link (temporary)
    TreeItem *targetItem = findMapItem(p_event, movingItems);

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

    // Check if we could link and position tmpParentContainer
    BranchContainer::Orientation newOrientation;
    BranchContainer *targetBranchContainer = nullptr;

    if (targetItem && targetItem->hasTypeBranch() && 
            !(editorState == MovingObjectWithoutLinking && (e->modifiers() & Qt::ShiftModifier))) {
        targetBranchContainer = ((BranchItem*)targetItem)->getBranchContainer();

        int d_pos;
        if (e->modifiers() & Qt::ShiftModifier) // FIXME-0 should also change targetBranchContainer
            d_pos = 1;
        else if (e->modifiers() & Qt::ControlModifier)
            d_pos = -1;
        else
            d_pos = 0;
        tmpParentContainer->setPos(targetBranchContainer->getPositionHintRelink(tmpParentContainer, d_pos, p_event));
        if (!tmpParentContainer->isTemporaryLinked())
            tmpParentContainer->setTemporaryLinked();
    } else {
        // Since moved containers are relative to tmpParentContainer anyway, just move 
        // tmpParentContainer to pointer position:
        tmpParentContainer->setPos(p_event - movingObj_initialContainerOffset);


        if (tmpParentContainer->isTemporaryLinked()) {
            tmpParentContainer->unsetTemporaryLinked();
        }
    }
    
    if (movingItems.count() > 0 && (tmpParentContainer->childrenCount() == 0)) {
        // Add selected branches and images temporary to tmpParentContainer,
        // if they are not there yet:
        BranchContainer *bc;
        BranchContainer *bc_first = nullptr;
        qreal h_total;
        foreach (TreeItem *ti, movingItems)
        {
            // The item structure in VymModel remaines untouched so far,
            // only containers will be reparented temporarily!
            if (ti->hasTypeBranch()) {
                bc = ((BranchItem*)ti)->getBranchContainer();

                if (tmpParentContainer->branchCount() == 0 || bc->parentItem() != tmpParentContainer->getBranchesContainer()) {
                    bc->setOriginalPos();
                    bc->setOriginalOrientation();
                    tmpParentContainer->addToBranchesContainer(bc, true);
                }

                if (!bc_first) {
                    bc_first = bc;
                    h_total = bc->rect().height();
                }

                if (bc_first && bc_first != bc) {
                    // Animate other items to position below first one
                    startAnimation (
                            bc, 
                            bc->pos(), 
                            QPointF(bc_first->pos().x(), bc_first->pos().y() + h_total));
                    h_total += bc->rect().height();
                }
            } else if (ti->hasTypeImage()) {
                ImageContainer *ic = ((ImageItem*)ti)->getImageContainer();
                if (ic->parentItem() != tmpParentContainer->getImagesContainer()) {
                    ic->setOriginalPos();
                    tmpParentContainer->addToImagesContainer(ic, true);
                }
            }
            /* FIXME-2 check xlinks later
                // Deleted above:  TreeItem *seli = model->getSelectedItem();

            else if (seli && seli->getType() == TreeItem::XLink) {
                // Move XLink control point
                MapObj *mosel = ((MapItem *)seli)->getMO();
                if (mosel) {
                    mosel->move(p - movingObj_offset); // FIXME-3 Missing savestate
                    model->setChanged();
                    model->emitSelectionChanged();
                }
            }
            else
                qWarning("ME::moveObject  Huh? I'm confused. No LMO or XLink moved");   // FIXME-2 shouldn't happen
            */
        }

    } // add to tmpParentContainer

    // Set orientation
    if (targetBranchContainer) {
        if (targetItem->depth() == 0) {
            if (tmpParentContainer->pos().x() > targetBranchContainer->pos().x())
                newOrientation = BranchContainer::RightOfParent;
            else
                newOrientation = BranchContainer::LeftOfParent;
        }
        else {
            newOrientation = targetBranchContainer->getOrientation();
        }
    } else {
        //tmpParentContainer->setOrientation(BranchContainer::UndefinedOrientation);
        /* FIXME-1 orientation while moving
         */
        // Try to set orientation for not relinked tmpParentContainer by checking the
        // layout and "original" parent
        Container *c = tmpParentContainer->getBranchesContainer();
        if (c && !c->childItems().isEmpty()) {
            // Consider orientation of *last* selected branch   // FIXME-1 meanwhile use *first* branch of movingItems
            BranchContainer *bc = (BranchContainer*)(c->childItems().last());
            if (bc->isOriginalFloating())  {
                if (tmpParentContainer->pos().x() > bc->getOriginalParentPos().x())
                {
                    // FIXME-1 qDebug() << "  rop";
                    newOrientation = BranchContainer::RightOfParent;
                }
                else
                {
                    // FIXME-1 qDebug() << "  lop";
                    newOrientation = BranchContainer::LeftOfParent;
                }
            } else {
                // FIXME-1 qDebug() << "ok2";
                newOrientation = bc->getOriginalOrientation();
            }
        }
    }

    // Reposition if required   // FIXME-2 not required currently. Re-check when changing orientation of children containers
    /*
    qDebug() << "   repos  tPC: " << tmpParentContainer->getOrientation() << "  new: " << newOrientation;
    if (newOrientation != tmpParentContainer->getOrientation()) {

        tmpParentContainer->reposition();
    }
    */

    // Update links
    foreach (TreeItem *ti, movingItems)
    {
        if (ti->hasTypeBranch()) {  // FIXME-2 later it should work the same for images!
            BranchContainer *bc = ((BranchItem*)ti)->getBranchContainer();
            if (tmpParentContainer->isTemporaryLinked())
                bc->setTemporaryLinked();
            else
                bc->unsetTemporaryLinked();
            bc->updateUpLink();
        }
    }
            
    // Update selection
    QItemSelection sel = model->getSelectionModel()->selection();
    updateSelection(sel, sel);

    scene()->update();

    return;
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

    destinationBranch = findMapBranchItem(p, movingItems);

    bool repositionNeeded = false;

    // Have we been picking color?
    if (state() == PickingColor) {
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
    if (state() == DrawingLink) {
        setState(Neutral);

        TreeItem *seli = movingItems.first();

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
    if (state() == MovingObject || state() == MovingObjectTmpLinked) {
        panningTimer->stop();

        int relinkedObjectsCount = tmpParentContainer->childBranches().count()  + tmpParentContainer->childImages().count();

        // Check if we have a destination and should relink
        if (destinationBranch && state() != MovingObjectWithoutLinking) {

            model->saveStateBeginBlock(
                    QString("Relink %1 objects to \"%2\"")
                        .arg(relinkedObjectsCount)
                        .arg(destinationBranch->getHeadingPlain()));
            
            // Loop over branches
            foreach(BranchContainer *bc, tmpParentContainer->childBranches()) {
                bc->unsetTemporaryLinked();

                BranchItem *bi = bc->getBranchItem();
                BranchItem *pi = bi->parentBranch();

                // Consider modifiers when relinking a branch:
                // Default is to append to destinationBranch
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

                // Prepare relinking: Save old position for undo, if required
                // 
                // tmpParentContainer always has floating layout, 
                // check original parent instead:
                /*
                BranchItem *pbi = bc->getBranchItem()->parentBranch();
                if (pbi) {
                    Container *originalParentContainer = pbi->getBranchesContainer(); // FIXME-00 MC will have no parentBranch and crash
                    if (originalParentContainer->hasFloatingLayout()) {
                        model->saveState(   // FIXME-2 check if undo/redo for moving floats and MCs works correctly 
                                bc->getBranchItem(), 
                                QString("setPos %1;").arg(qpointFToString(bc->getOriginalPos())),
                                nullptr,
                                "", 
                                QString("Move %1") .arg(bc->getBranchItem()->getHeadingPlain()));
                    }
                }
                */ 

                // Save current scene position for redo or if moved to floating layout
                QPointF sp = bc->scenePos();    // FIXME-00 use realPos instead?

                // Relink
                model->relinkBranch(bi, dst_branch, dst_num, true); // FIXME-00 saving positions should be done in VymModel completely...

                // After relinking: Save new position for redo, if required 
                if (dst_branch->getBranchesContainer()->hasFloatingLayout()) {  
                    bc->setPos(dst_branch->getBranchesContainer()->sceneTransform().inverted().map(sp));
                    model->saveState(
                            nullptr,
                            "", 
                            bc->getBranchItem(), 
                            QString("setPos %1;").arg(qpointFToString(bc->pos())),
                            QString("Move %1") .arg(bc->getBranchItem()->getHeadingPlain()));
                }
            }   // Loop to relink branches

            // Loop over images
            foreach(ImageContainer *ic, tmpParentContainer->childImages()) {
                ImageItem *ii = ic->getImageItem();
                model->relinkImage(ii, destinationBranch);
            }
            // Destination available and movingObject

            model->reposition();

            model->saveStateEndBlock();
        } else {
            // Branches moved, but not relinked
            QPointF t = p - movingObj_initialScenePos;    // Defined in mousePressEvent

            QList <BranchContainer*> childBranches = tmpParentContainer->childBranches();
            QList <QPointF> animationCurrentPositions;   // After reposition start animations
            QList <BranchContainer*> animationContainers;

            if (!childBranches.isEmpty()) {
                repositionNeeded = true;

                // We begin a saveStateBlock, if nothing is really moved, this
                // block will be discarded later
                model->saveStateBeginBlock(
                    QString("Move %1 branch(es)").arg(childBranches.count())
                );
                // Empty the tmpParentContainer, which is used for moving
                // Updating the stacking order also resets the original parents

                foreach(BranchContainer *bc, childBranches) {
                    BranchItem *bi = bc->getBranchItem();

                    // Relink container to original parent container 
                    // and keep (!) current absolute position
                    bi->updateContainerStackingOrder();

                    // Floating layout or mapcenter moved, saveState
                    if (bc->isFloating() || bi->depth() == 0)
                    {
                        // Relative positioning
                        model->saveState(
                            bi, QString("setPos%1").arg(qpointFToString(bc->getOriginalPos())),
                            bi, QString("setPos%1").arg(qpointFToString(bc->getOriginalPos() + t)));
                        if (bi->depth() == 0)
                            bc->setPos(tmpParentContainer->pos());
                    } else {
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
        if (tmpParentContainer->childImages().count() > 0 ) repositionNeeded = true;

        foreach(ImageContainer *ic, tmpParentContainer->childImages()) {
            ImageItem *ii = ic->getImageItem();
            BranchItem *pi = ii->parentBranch();

            // Update parent of moved container to original imageContainer 
            // in parent branch
            pi->addToImagesContainer(ic);

            QString pold = qpointFToString(ic->getOriginalPos());
            QString pnow = qpointFToString(ic->pos());
            model->saveState(ii, "setPos " + pold, ii,
                             "setPos " + pnow,
                             QString("Move %1 to %2")
                                 .arg(model->getObjectName(ii))
                                 .arg(pnow));

        } // Image moved, but not relinked

        if (repositionNeeded) {
            model->reposition();
            model->emitSelectionChanged();
        }

        // Finally resize scene, if needed
        scene()->update();
        vPan = QPoint();
    } // MovingObject or MovingObjectTmpLinked
    else
        // maybe we moved View: set old cursor
        setCursor(Qt::ArrowCursor);

    if (state() != EditingHeading)
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
        LinkableMapObj *lmo;
        if (ti) {
            if (state() == EditingHeading)
                editHeadingFinished();
            model->select(ti);
            BranchItem *selbi = model->getSelectedBranch();
            if (selbi) {
                lmo = ((MapItem *)ti)->getLMO();
                if (lmo) {
                    QUuid uid = ((BranchObj *)lmo)->findSystemFlagUidByPos(p);

                    // Don't edit heading when double clicking system flag:
                    if (!uid.isNull())
                        return;
                }
            }
            e->accept();
            editHeading();
        }
    }
}

void MapEditor::wheelEvent(QWheelEvent *e)
{
    if (e->modifiers() & Qt::ControlModifier &&
        e->orientation() == Qt::Vertical) {
        QPointF p = mapToScene(e->pos());
        if (e->delta() > 0)
            // setZoomFactorTarget (zoomFactorTarget*1.15);
            setViewCenterTarget(p, zoomFactorTarget * 1.15, 0);
        else
            // setZoomFactorTarget (zoomFactorTarget*0.85);
            setViewCenterTarget(p, zoomFactorTarget * 0.85, 0);
    }
    else {
        scrollBarPosAnimation.stop();
        QGraphicsView::wheelEvent(e);
    }
}

void MapEditor::focusOutEvent(QFocusEvent *)
{
    // qDebug()<<"ME::focusOutEvent"<<e->reason();
    if (state() == EditingHeading)
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
                            model->setHeadingPlainText(url);
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

void MapEditor::updateSelection(QItemSelection nsel, QItemSelection dsel)
{
    Q_UNUSED(nsel);

    QList<MapItem *> itemsSelected;
    QList<MapItem *> itemsDeselected;

    QItemSelection sel = model->getSelectionModel()->selection();

    LinkableMapObj *lmo;
    // Add new selected objects
    foreach (QModelIndex ix, sel.indexes()) {
        MapItem *mi = static_cast<MapItem *>(ix.internalPointer());
        if (mi->hasTypeBranch() || mi->getType() == TreeItem::Image ||
            mi->getType() == TreeItem::XLink)
            if (!itemsSelected.contains(mi))
                itemsSelected.append(mi);
        lmo = mi->getLMO(); // FIXME-2
        if (lmo)
            mi->getLMO()->updateVisibility();
    }

    // Delete objects meanwhile removed from selection
    foreach (QModelIndex ix, dsel.indexes()) {
        MapItem *mi = static_cast<MapItem *>(ix.internalPointer());
        if (mi->hasTypeBranch() || mi->getType() == TreeItem::Image ||
            mi->getType() == TreeItem::XLink)
            if (!itemsDeselected.contains(mi))
                itemsDeselected.append(mi);
        lmo = mi->getLMO(); // FIXME-2 xlink does return nullptr
        if (lmo)
            mi->getLMO()->updateVisibility();
    }

    // Trim list of selection paths
    while (itemsSelected.count() < selPathList.count())
        delete selPathList.takeFirst();

    // Reduce polygons
    while (itemsSelected.count() < selPathList.count())
        delete selPathList.takeFirst();

    // Add additonal polygons
    QGraphicsPathItem *sp;
    while (itemsSelected.count() > selPathList.count()) {
        sp = mapScene->addPath(QPainterPath(), QPen(selectionColor),
                               selectionColor);
        sp->show();
        selPathList.append(sp);
    }

    // Reposition polygons
    for (int i = 0; i < itemsSelected.count(); ++i) {
        //MapObj *mo = itemsSelected.at(i)->getMO();  // FIXME-2 remove MO here
        sp = selPathList.at(i);
        sp->setPath(itemsSelected.at(i)->getSelectionPath());
        sp->setPen(selectionColor);
        sp->setBrush(selectionColor);
        //sp->setParentItem(mo);
        //sp->setZValue(dZ_SELBOX); // which z to use?
    }

    scene()->update();
}

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
        BranchObj *bo;
        BranchItem *cur = nullptr;
        BranchItem *prev = nullptr;
        model->nextBranch(cur, prev);
        while (cur) {
            if (!cur->hasHiddenExportParent()) {
                // Branches
                bo = (BranchObj *)(cur->getLMO());
                if (bo && bo->isVisibleObj())
                    obstacles.append(bo->getBBox());
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

void MapEditor::setSelectionColor(QColor col)
{
    selectionColor = col;
    selectionColor.setAlpha(200);
    QItemSelection sel = model->getSelectionModel()->selection();
    updateSelection(sel, sel);
}

QColor MapEditor::getSelectionColor() { return selectionColor; }