#include "mapeditor.h"

#include <QGraphicsProxyWidget>
#include <QMenuBar>
#include <QObject>
#include <QPrintDialog>
#include <QPrinter>
#include <QScrollBar>

#include "branchitem.h"
#include "geometry.h"
#include "mainwindow.h"
#include "misc.h"
#include "shortcuts.h"
#include "warningdialog.h"
#include "winter.h"
#include "xlinkitem.h"

extern Main *mainWindow;
extern QString clipboardDir;
extern QString clipboardFile;
extern uint clipboardItemCount;
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
    mapScene = new QGraphicsScene(NULL);
    mapScene->setBackgroundBrush(QBrush(Qt::white, Qt::SolidPattern));

    zoomFactor = zoomFactorTarget = 1;
    angle = angleTarget = 0;

    model = vm;
    model->registerEditor(this);

    setScene(mapScene);

    setStyleSheet("QGraphicsView:focus {" + editorFocusStyle + "}");

    // Create bitmap cursors, platform dependant
    HandOpenCursor = QCursor(QPixmap(":/cursorhandopen.png"), 1, 1);
    PickColorCursor = QCursor(QPixmap(":/cursorcolorpicker.png"), 5, 27);
    XLinkCursor = QCursor(QPixmap(":/cursorxlink.png"), 1, 7);

    editingBO = NULL;

    printFrame = true;
    printFooter = true;

    setAcceptDrops(true);

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
    lineEdit = NULL;

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

    winter = NULL;
}

MapEditor::~MapEditor()
{
    // qDebug ()<<"Destr MapEditor this="<<this;

    if (winter) {
        delete winter;
        winter = NULL;
    }
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

        // Update currently moving object
        moveObject();
    }
}

void MapEditor::scrollTo(const QModelIndex &index)
{
    if (index.isValid()) {
        LinkableMapObj *lmo = NULL;
        TreeItem *ti = static_cast<TreeItem *>(index.internalPointer());
        if (ti->getType() == TreeItem::Image || ti->isBranchLikeType())
            lmo = ((MapItem *)ti)->getLMO();
        if (lmo) {
            QRectF r = lmo->getBBox();
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

void MapEditor::setViewCenterTarget()
{
    MapItem *selti = (MapItem *)(model->getSelectedItem());
    if (selti) {
        LinkableMapObj *lmo = selti->getLMO();
        if (lmo)
            setViewCenterTarget(lmo->getBBox().center(), 1, 0);
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

void MapEditor::minimizeView() { setSceneRect(scene()->itemsBoundingRect()); }

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
        QGraphicsRectItem *frame = NULL;

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

QRectF MapEditor::getTotalBBox()
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

void MapEditor::autoLayout()
{
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
                model->startAnimation((BranchObj *)mapobjects[i], v);
                if (debug)
                    qDebug() << i << " Weight: " << polys.at(i).weight() << " "
                             << v << " " << headings.at(i);
            }
        }
        /*
        model->reposition();
        orientationChanged=false;
        for (int i=0;i<polys.size();i++)
            if (orients[i]!=mapobjects[i]->getOrientation())
            {
            orientationChanged=true;
            break;
            }
        */

        break;

        // orientationChanged=false;
    } // loop if orientation has changed

    model->emitSelectionChanged();
}

TreeItem *MapEditor::findMapItem(QPointF p, TreeItem *exclude)
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
    TreeItem *found = NULL;
    while (bi) {
        found = bi->findMapItem(p, exclude);
        if (found)
            return found;
        i++;
        bi = model->getRootItem()->getBranchNum(i);
    }
    return NULL;
}

void MapEditor::testFunction1() {}

void MapEditor::testFunction2() { autoLayout(); }

void MapEditor::toggleWinter()
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
    return NULL;
}

BranchItem *MapEditor::getBranchAbove(BranchItem *selbi)
{
    if (selbi) {
        int dz = selbi->depth(); // original depth
        bool invert = false;
        if (selbi->getLMO()->getOrientation() == LinkableMapObj::LeftOfCenter)
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
    return NULL;
}

BranchItem *MapEditor::getBranchDirectBelow(BranchItem *bi)
{
    if (bi) {
        int i = bi->num();
        if (i + 1 < bi->parent()->branchCount())
            return bi->parent()->getBranchNum(i + 1);
    }
    return NULL;
}

BranchItem *MapEditor::getBranchBelow(BranchItem *selbi)
{
    if (selbi) {
        BranchItem *bi;
        int dz = selbi->depth(); // original depth
        bool invert = false;
        if (selbi->getLMO()->getOrientation() == LinkableMapObj::LeftOfCenter)
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
    return NULL;
}

BranchItem *MapEditor::getLeftBranch(TreeItem *ti)
{
    if (!ti)
        return NULL;

    if (ti->isBranchLikeType()) {
        BranchItem *bi = (BranchItem *)ti;
        if (bi->depth() == 0) {
            // Special case: use alternative selection index
            BranchItem *newbi = bi->getLastSelectedBranchAlt();
            if (!newbi) {
                BranchObj *bo;
                // Try to find a mainbranch left of center
                for (int i = 0; i < bi->branchCount(); i++) {
                    newbi = bi->getBranchNum(i);
                    bo = newbi->getBranchObj();
                    if (bo &&
                        bo->getOrientation() == LinkableMapObj::LeftOfCenter)
                        break;
                }
            }
            return newbi;
        }
        if (bi->getBranchObj()->getOrientation() ==
            LinkableMapObj::RightOfCenter)
            // right of center
            return (BranchItem *)(bi->parent());
        else
            // left of center
            if (bi->getType() == TreeItem::Branch)
            return bi->getLastSelectedBranch();
    }

    if (ti->parent() && ti->parent()->isBranchLikeType())
        return (BranchItem *)(ti->parent());
    return NULL;
}

BranchItem *MapEditor::getRightBranch(TreeItem *ti)
{
    if (!ti)
        return NULL;

    if (ti->isBranchLikeType()) {
        BranchItem *bi = (BranchItem *)ti;
        if (bi->depth() == 0) {
            // Special case: use alternative selection index
            BranchItem *newbi = bi->getLastSelectedBranch();
            if (!newbi) {
                BranchObj *bo;
                // Try to find a mainbranch right of center
                for (int i = 0; i < bi->branchCount(); i++) {
                    newbi = bi->getBranchNum(i);
                    bo = newbi->getBranchObj();
                    if (bo &&
                        bo->getOrientation() == LinkableMapObj::RightOfCenter)
                        qDebug()
                            << "BI found right: " << newbi->getHeadingPlain();
                }
            }
            return newbi;
        }
        if (bi->getBranchObj()->getOrientation() ==
            LinkableMapObj::LeftOfCenter)
            // left of center
            return (BranchItem *)(bi->parent());
        else
            // right of center
            if (bi->getType() == TreeItem::Branch)
            return (BranchItem *)bi->getLastSelectedBranch();
    }

    if (ti->parent() && ti->parent()->isBranchLikeType())
        return (BranchItem *)(ti->parent());

    return NULL;
}

void MapEditor::cursorUp()
{
    if (state == MapEditor::EditingHeading)
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
        if (ti && ti->isBranchLikeType()) {
            bi = getBranchAbove( (BranchItem*)ti);
            if (bi) 
                model->select(bi);
        }
    }
}

void MapEditor::cursorUpToggleSelection()
{
    if (state == MapEditor::EditingHeading)
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
        if (ti && ti->isBranchLikeType()) {
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
    if (state == MapEditor::EditingHeading)
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
        if (ti && ti->isBranchLikeType()) {
            bi = getBranchBelow( (BranchItem*)ti);

            if (bi) 
                model->select(bi);
        }
    }
}

void MapEditor::cursorDownToggleSelection()
{
    if (state == MapEditor::EditingHeading)
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
        if (ti && ti->isBranchLikeType()) {
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
    if (state == EditingHeading) {
        editHeadingFinished();
        return;
    }

    BranchObj *bo = model->getSelectedBranchObj();
    BranchItem *bi = model->getSelectedBranch();
    if (bo && bo) {
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

        QPointF tl;
        QPointF br;
        qreal w = 230;
        qreal h = 30;
        if (bo->getOrientation() != LinkableMapObj::LeftOfCenter) {
            tl = bo->getOrnamentsBBox().topLeft();
            br = tl + QPointF(w, h);
        }
        else {
            br = bo->getOrnamentsBBox().bottomRight();
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
    lineEdit = NULL;

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
    TreeItem *ti = findMapItem(p, NULL);

    if (ti) { // MapObj was found
        model->select(ti);

        LinkableMapObj *lmo = NULL;
        BranchItem *selbi = model->getSelectedBranch();
        if (ti)
            lmo = ((MapItem *)ti)->getLMO();

        // Context Menu
        if (lmo && selbi) {
            QString sysFlagName;
            QUuid uid = ((BranchObj *)lmo)->findSystemFlagUidByPos(p);
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
    else { // No MapObj found, we are on the Canvas itself
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

void MapEditor::startMovingView(QMouseEvent *e)
{
    setState(MovingView);
    movingObj = NULL; // move Content not Obj
    movingObj_offset = e->globalPos();
    movingCont_start =
        QPointF(horizontalScrollBar()->value(), verticalScrollBar()->value());
    movingVec = QPointF(0, 0);
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

    QPointF p = mapToScene(e->pos());
    TreeItem *ti_found = findMapItem(p, NULL);
    LinkableMapObj *lmo_found = NULL;
    if (ti_found)
        lmo_found = ((MapItem *)ti_found)->getLMO();

    // Stop editing heading
    if (model->isSelectionBlocked()) {
        if (ti_found && ti_found->isBranchLikeType() &&
            ti_found == model->getSelectedItem()) {
            // return event to LineEdit to allow selecting in LineEdit
            e->ignore();
            QGraphicsView::mousePressEvent(e);
            return;
        }
        else
            // Stop editing in LineEdit
            editHeadingFinished();
    }

    QString sysFlagName;
    QUuid uid;
    if (lmo_found) {
        uid = ((BranchObj *)lmo_found)->findSystemFlagUidByPos(p);
        if (!uid.isNull()) {
            Flag *flag = systemFlagsMaster->findFlagByUid(uid);
            if (flag)
                sysFlagName = flag->getName();
        }
    }

    /*
    qDebug() << "ME::mouse pressed\n";
    qDebug() << "  lmo_found=" << lmo_found;
    qDebug() << "   ti_found=" << ti_found;
    //if (ti_found) qDebug() << "   ti_found="<<ti_found->getHeading();
    qDebug() << " flag=" << sysFlagName;
    */

    // Check modifier key (before selecting object!)
    if (ti_found && (e->modifiers() & Qt::ShiftModifier)) {
        if (mainWindow->getModMode() == Main::ModModeColor) {
            setState(PickingColor);
            mainWindow->setCurrentColor(ti_found->getHeadingColor());
            if (e->modifiers() & Qt::ControlModifier)
                model->colorBranch(ti_found->getHeadingColor());
            else
                model->colorSubtree(ti_found->getHeadingColor());
            return;
        }

        if (mainWindow->getModMode() == Main::ModModeMoveView) {
            startMovingView(e);
            return;
        }
    }

    // Check vymlink  modifier (before selecting object!)
    if (ti_found && sysFlagName == "system-vymLink") {
        model->select(ti_found);
        if (e->modifiers() & Qt::ControlModifier)
            mainWindow->editOpenVymLink(true);
        else
            mainWindow->editOpenVymLink(false);
        return;
    }

    // Select the clicked object, if not moving without linking
    if (ti_found && (e->modifiers() & Qt::ShiftModifier)) {
        if (mainWindow->getModMode() == Main::ModModePoint) {
            model->selectToggle(ti_found);
            lastToggleDirection = toggleUndefined;
        }
    }
    else
        model->select(ti_found);

    e->accept();

    // Take care of  remaining system flags _or_ modifier modes
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
                         LinkableMapObj::RightOfCenter &&
                     p.x() < lmo_found->getBBox().left() + 10) ||
                    (lmo_found->getOrientation() !=
                         LinkableMapObj::LeftOfCenter &&
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
    }

    // XLink modifier, create new XLink
    BranchItem *selbi = model->getSelectedBranch();
    if (selbi && mainWindow->getModMode() == Main::ModModeXLink &&
        (e->modifiers() & Qt::ShiftModifier)) {
        setState(DrawingLink);
        tmpLink = new Link(model);
        tmpLink->setBeginBranch(selbi);
        tmpLink->createMapObj();
        tmpLink->setStyleBegin("None");
        tmpLink->setStyleEnd("None");
        tmpLink->setEndPoint(mapToScene(e->pos()));
        tmpLink->updateLink();
        return;
    }

    // Start moving around
    if (lmo_found) {
        // Left Button	    Move Branches
        if (e->button() == Qt::LeftButton) {
            // No system flag clicked, take care of moving modes or simply
            // moving
            movingObj_offset.setX(p.x() - lmo_found->x());
            movingObj_offset.setY(p.y() - lmo_found->y());
            movingObj_orgPos.setX(lmo_found->x());
            movingObj_orgPos.setY(lmo_found->y());
            if (ti_found->depth() > 0) {
                lmo_found->setRelPos();
                movingObj_orgRelPos = lmo_found->getRelPos();
            }

            if (mainWindow->getModMode() == Main::ModModeMoveObject &&
                e->modifiers() & Qt::ShiftModifier) {
                setState(MovingObjectWithoutLinking);
            }
            else
                setState(MovingObject);

            movingObj = model->getSelectedLMO();
        }
        else
            // Middle Button    Toggle Scroll
            // (On Mac OS X this won't work, but we still have
            // a button in the toolbar)
            if (e->button() == Qt::MidButton)
            model->toggleScroll();
    }
    else { // No lmo found, check XLinks
        if (ti_found) {
            if (ti_found->getType() == TreeItem::XLink) {
                XLinkObj *xlo = (XLinkObj *)((MapItem *)ti_found)->getMO();
                if (xlo) {
                    setState(EditingLink);
                    int i = xlo->ctrlPointInClickBox(p);
                    if (i >= 0)
                        xlo->setSelection(i);
                    movingObj_offset.setX(p.x() - xlo->x());
                    movingObj_offset.setY(p.y() - xlo->y());
                    movingObj_orgPos.setX(xlo->x());
                    movingObj_orgPos.setY(xlo->y());
                }
            }
        }
        else { // No MapObj found, we are on the scene itself
            // Left Button	    move Pos of sceneView
            if (e->button() == Qt::LeftButton ||
                e->button() == Qt::MiddleButton) {
                startMovingView(e);
                return;
            }
        }
    }
}

void MapEditor::mouseMoveEvent(QMouseEvent *e)
{
    // Show mouse position for debugging in statusBar
    if (debug && e->modifiers() & Qt::ControlModifier)
        mainWindow->statusMessage(
            QString("ME::mousePressEvent  Scene: %1  widget: %2")
                .arg(qpointFToString(mapToScene(e->pos())))
                .arg(qpointFToString(e->pos())));

    // Move sceneView
    if (state == MovingView &&
        (e->buttons() == Qt::LeftButton || e->buttons() == Qt::MiddleButton)) {
        QPointF p = e->globalPos();
        movingVec.setX(-p.x() + movingObj_offset.x());
        movingVec.setY(-p.y() + movingObj_offset.y());
        horizontalScrollBar()->setSliderPosition(
            (int)(movingCont_start.x() + movingVec.x()));
        verticalScrollBar()->setSliderPosition(
            (int)(movingCont_start.y() + movingVec.y()));
        // Avoid flickering
        scrollBarPosAnimation.stop();
        viewCenterAnimation.stop();
        rotationAnimation.stop();
        // zoomAnimation.stop();

        return;
    }

    TreeItem *seli = model->getSelectedItem();

    MapObj *mosel = NULL;
    if (seli)
        mosel = ((MapItem *)seli)->getMO();

    // If not already happened during mousepress, we might need to switch state
    if (mainWindow->getModMode() == Main::ModModeMoveObject &&
        e->modifiers() & Qt::ShiftModifier && e->buttons() == Qt::LeftButton) {
        state = MovingObjectWithoutLinking;
    }

    // Move the selected MapObj
    if (mosel &&
        (state == MovingObject || state == MovingObjectWithoutLinking ||
         state == EditingLink)) {
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

        pointerPos = e->pos();
        pointerMod = e->modifiers();
        moveObject();
    } // selection && moving_obj

    // Draw a link from one branch to another
    if (state == DrawingLink) {
        tmpLink->setEndPoint(mapToScene(e->pos()));
        tmpLink->updateLink();
    }
}

void MapEditor::moveObject()
{
    if (!panningTimer->isActive())
        panningTimer->start(50);

    QPointF p = mapToScene(pointerPos);
    TreeItem *seli = model->getSelectedItem();
    LinkableMapObj *lmosel = NULL;
    if (seli)
        lmosel = ((MapItem *)seli)->getLMO();

    objectMoved = true;

    // reset cursor if we are moving and don't copy

    // Check if we could link
    TreeItem *ti_found = findMapItem(p, seli);
    BranchItem *bi_dst = NULL;
    LinkableMapObj *lmo_dst = NULL;
    if (ti_found && ti_found != seli && ti_found->isBranchLikeType()) {
        bi_dst = (BranchItem *)ti_found;
        lmo_dst = bi_dst->getLMO();
    }
    else
        bi_dst = NULL;

    if (lmosel) {
        if (seli->getType() == TreeItem::Image) {
            FloatImageObj *fio = (FloatImageObj *)lmosel;
            fio->moveCenter(p.x() - movingObj_offset.x(),
                            p.y() - movingObj_offset.y());
            fio->setRelPos();
            fio->updateLinkGeometry(); // no need for reposition, if we update
                                       // link here
            model->emitSelectionChanged(); // position has changed

            // Relink float to new mapcenter or branch, if shift is pressed
            // Only relink, if selection really has a new parent
            if (pointerMod == Qt::ShiftModifier && bi_dst &&
                bi_dst != seli->parent()) {
                // Also save the move which was done so far
                QString pold = qpointFToString(movingObj_orgRelPos);
                QString pnow = qpointFToString(fio->getRelPos());
                model->saveState(seli, "moveRel " + pold, seli,
                                 "moveRel " + pnow,
                                 QString("Move %1 to relative position %2")
                                     .arg(model->getObjectName(lmosel))
                                     .arg(pnow));
                model->reposition();

                model->relinkImage((ImageItem *)seli, bi_dst);
                model->select(seli);
            }
        }
        else if (seli->isBranchLikeType()) { // selection != a FloatObj
            if (seli->depth() == 0) {
                // Move mapcenter
                lmosel->move(p - movingObj_offset);
                if (pointerMod == Qt::ShiftModifier) {
                    // Move only mapcenter, leave its children where they are
                    QPointF v;
                    v = lmosel->getAbsPos();
                    for (int i = 0; i < seli->branchCount(); ++i) {
                        seli->getBranchObjNum(i)->setRelPos();
                        seli->getBranchObjNum(i)->setOrientation();
                    }
                }
            }
            else {
                if (seli->depth() == 1) {
                    // Move mainbranch
                    if (!lmosel->hasParObjTmp())
                        lmosel->move(p - movingObj_offset);
                    lmosel->setRelPos();
                }
                else {
                    // d>1, move ordinary branch
                    if (lmosel->getOrientation() ==
                        LinkableMapObj::LeftOfCenter)
                        // Add width of bbox here, otherwise alignRelTo will
                        // cause jumping around
                        lmosel->move(p.x() - movingObj_offset.x(),
                                     p.y() - movingObj_offset.y() +
                                         lmosel->getTopPad());
                    else
                        lmosel->move(p.x() - movingObj_offset.x(),
                                     p.y() - movingObj_offset.y() -
                                         lmosel->getTopPad());
                    BranchItem *selbi = ((BranchItem *)seli);
                    if (selbi->parentBranch()->getChildrenLayout() ==
                        BranchItem::FreePositioning)
                        lmosel->setRelPos();
                }

            } // depth>0

            // Maybe we can relink temporary?
            if (bi_dst && state != MovingObjectWithoutLinking) {
                if (pointerMod == Qt::ControlModifier) {
                    // Special case: CTRL to link below dst
                    lmosel->setParObjTmp(lmo_dst, p, +1);
                }
                else if (pointerMod == Qt::ShiftModifier)
                    lmosel->setParObjTmp(lmo_dst, p, -1);
                else
                    lmosel->setParObjTmp(lmo_dst, p, 0);
            }
            else
                lmosel->unsetParObjTmp();

            // reposition subbranch
            lmosel->reposition();

            QItemSelection sel = model->getSelectionModel()->selection();
            updateSelection(sel, sel); // position has changed

            // In winter mode shake snow from heading
            if (winter)
                model->emitDataChanged(seli);
        } // Moving branchLikeType
    }     // End of lmosel != NULL
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
        qWarning("ME::moveObject  Huh? I'm confused.");

    scene()->update();

    return;
}

void MapEditor::mouseReleaseEvent(QMouseEvent *e)
{
    QPointF p = mapToScene(e->pos());
    TreeItem *seli = model->getSelectedItem();

    TreeItem *dsti = NULL;
    if (seli)
        dsti = findMapItem(p, seli);
    LinkableMapObj *dst = NULL;
    BranchItem *selbi = model->getSelectedBranch();
    if (dsti && dsti->isBranchLikeType())
        dst = ((MapItem *)dsti)->getLMO();
    else
        dsti = NULL;

    // Have we been picking color?
    if (state == PickingColor) {
        setCursor(Qt::ArrowCursor);
        // Check if we are over another branch
        if (dst) {
            if (e->modifiers() & Qt::ShiftModifier)
                model->colorBranch(mainWindow->getCurrentColor());
            else
                model->colorSubtree(mainWindow->getCurrentColor());
        }
        setState(Neutral);
        return;
    }

    // Have we been drawing a link?
    if (state == DrawingLink) {
        setState(Neutral);
        // Check if we are over another branch
        if (dsti) {
            tmpLink->setEndBranch(((BranchItem *)dsti));
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
                        .arg(model->getObjectName(dsti)));
                return;
            }
        }
        delete (tmpLink);
        tmpLink = NULL;
        return;
    }

    // Have we been moving something?
    if (seli && state == MovingObject) {
        panningTimer->stop();
        if (seli->getType() == TreeItem::Image) {
            FloatImageObj *fio = (FloatImageObj *)(((MapItem *)seli)->getLMO());
            if (fio) {
                // Moved Image, we need to reposition
                QString pold = qpointFToString(movingObj_orgRelPos);
                QString pnow = qpointFToString(fio->getRelPos());
                model->saveState(seli, "moveRel " + pold, seli,
                                 "moveRel " + pnow,
                                 QString("Move %1 to relative position %2")
                                     .arg(model->getObjectName(seli))
                                     .arg(pnow));

                model->emitDataChanged(
                    seli->parent()); // Parent of image has changed
                model->reposition();
            }
        }

        if (selbi && selbi->depth() == 0) {
            if (movingObj_orgPos != selbi->getBranchObj()->getAbsPos()) {
                QString pold = qpointFToString(movingObj_orgPos);
                QString pnow =
                    qpointFToString(selbi->getBranchObj()->getAbsPos());

                model->saveState(selbi, "move " + pold, selbi, "move " + pnow,
                                 QString("Move mapcenter %1 to position %2")
                                     .arg(model->getObjectName(selbi))
                                     .arg(pnow));
            }
        }

        if (seli->isBranchLikeType()) //(seli->getType() == TreeItem::Branch )
        {                             // A branch was moved
            LinkableMapObj *lmosel = NULL;
            lmosel = ((MapItem *)seli)->getLMO();

            // save the position in case we link to mapcenter
            QPointF savePos = QPointF(lmosel->getAbsPos());

            // Reset the temporary drawn link to the original one
            lmosel->unsetParObjTmp();

            // For Redo we may need to save original selection
            QString preSelStr = model->getSelectString(seli);

            if (dsti && objectMoved && state != MovingObjectWithoutLinking) {
                // We have a destination, relink to that
                BranchObj *selbo = model->getSelectedBranchObj();

                QString preParStr = model->getSelectString(seli->parent());
                QString preNum = QString::number(seli->num(), 10);
                QString preDstParStr;

                if (e->modifiers() & Qt::ShiftModifier &&
                    dsti->parent()) { // Link above dst
                    preDstParStr = model->getSelectString(dsti->parent());
                    model->relinkBranch((BranchItem *)seli,
                                        (BranchItem *)dsti->parent(),
                                        ((BranchItem *)dsti)->num(), true);
                }
                else if (e->modifiers() & Qt::ControlModifier &&
                         dsti->parent()) {
                    // Link below dst
                    preDstParStr = model->getSelectString(dsti->parent());
                    model->relinkBranch((BranchItem *)seli,
                                        (BranchItem *)dsti->parent(),
                                        ((BranchItem *)dsti)->num() + 1, true);
                }
                else { // Append to dst
                    preDstParStr = model->getSelectString(dsti);
                    model->relinkBranch((BranchItem *)seli, (BranchItem *)dsti,
                                        -1, true, movingObj_orgPos);
                    if (dsti->depth() == 0)
                        selbo->move(savePos);
                }
            }
            else {
                // No destination, undo  temporary move

                if (seli->depth() == 1) {
                    // The select string might be different _after_ moving
                    // around. Therefor reposition and then use string of old
                    // selection, too
                    model->reposition();

                    QPointF rp(lmosel->getRelPos());
                    if (rp != movingObj_orgRelPos) {
                        QString ps = qpointFToString(rp);
                        model->saveState(
                            model->getSelectString(lmosel),
                            "moveRel " + qpointFToString(movingObj_orgRelPos),
                            preSelStr, "moveRel " + ps,
                            QString("Move %1 to relative position %2")
                                .arg(model->getObjectName(lmosel))
                                .arg(ps));
                    }
                }

                if (selbi->parentBranch()->getChildrenLayout() ==
                    BranchItem::FreePositioning) {
                    lmosel->setRelPos();
                    model->reposition();
                }
                else {

                    // Draw the original link, before selection was moved around
                    if (settings.value("/animation/use", true).toBool() &&
                        seli->depth() > 1
                        //		    && distance
                        //(lmosel->getRelPos(),movingObj_orgRelPos)<3
                    ) {
                        lmosel->setRelPos(); // calc relPos first for starting
                                             // point

                        model->startAnimation((BranchObj *)lmosel,
                                              lmosel->getRelPos(),
                                              movingObj_orgRelPos);
                    }
                    else
                        model->reposition();
                }
            }
        }
        // Finally resize scene, if needed
        scene()->update();
        movingObj = NULL;
        objectMoved = false;
        vPan = QPoint();
    }
    else
        // maybe we moved View: set old cursor
        setCursor(Qt::ArrowCursor);

    if (state != EditingHeading)
        setState(Neutral); // Continue editing after double click!

    QGraphicsView::mouseReleaseEvent(e);
}

void MapEditor::mouseDoubleClickEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        QPointF p = mapToScene(e->pos());
        TreeItem *ti = findMapItem(p, NULL);
        LinkableMapObj *lmo;
        if (ti) {
            if (state == EditingHeading)
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
    if (state == EditingHeading)
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

            BranchItem *bi = NULL;
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
    if (state != Neutral && s != Neutral)
        qWarning() << "MapEditor::setState  switching directly from " << state
                   << " to " << s;
    state = s;
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
        }
        qDebug() << "MapEditor: State " << s << " of " << model->getMapName();
    }
    */
}

MapEditor::EditorState MapEditor::getState() { return state; }

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
        if (mi->isBranchLikeType() || mi->getType() == TreeItem::Image ||
            mi->getType() == TreeItem::XLink)
            if (!itemsSelected.contains(mi))
                itemsSelected.append(mi);
        lmo = mi->getLMO();
        if (lmo)
            mi->getLMO()->updateVisibility();
    }

    // Delete objects meanwhile removed from selection
    foreach (QModelIndex ix, dsel.indexes()) {
        MapItem *mi = static_cast<MapItem *>(ix.internalPointer());
        if (mi->isBranchLikeType() || mi->getType() == TreeItem::Image ||
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
        MapObj *mo = itemsSelected.at(i)->getMO();
        sp = selPathList.at(i);
        sp->setPath(mo->getSelectionPath());
        sp->setPen(selectionColor);
        sp->setBrush(selectionColor);
        sp->setParentItem(mo);
        sp->setZValue(dZ_SELBOX);

        // Reposition also LineEdit for heading during animation
        if (lineEdit)
            lineEdit->move(mo->getAbsPos().toPoint());
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
        qDebug() << "  ti=NULL";
        return;
        }
        qDebug() << "  ti="<<ti;
        qDebug() << "  h="<<ti->getHeadingPlain();
    */

    if (ti && ti->isBranchLikeType()) {
        BranchObj *bo = (BranchObj *)(((MapItem *)ti)->getLMO());
        bo->updateVisuals();
    }

    if (winter) {
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

void MapEditor::togglePresentationMode()
{
    mainWindow->togglePresentationMode();
}

void MapEditor::setSelectionColor(QColor col)
{
    selectionColor = col;
    selectionColor.setAlpha(220);
    QItemSelection sel = model->getSelectionModel()->selection();
    updateSelection(sel, sel);
}

QColor MapEditor::getSelectionColor() { return selectionColor; }
