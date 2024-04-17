#include <QDebug>

#include "xlinkobj.h"

#include "branchitem.h"
#include "geometry.h"
#include "math.h" // atan
#include "misc.h" // max

/////////////////////////////////////////////////////////////////
// XLinkObj
/////////////////////////////////////////////////////////////////

int XLinkObj::arrowSize = 6; // make instances
int XLinkObj::clickBorder = 8;
int XLinkObj::pointRadius = 10;
int XLinkObj::d_control = 300;

XLinkObj::XLinkObj(Link *l)
{
    //qDebug()<< "Const XLinkObj (link)";
    link = l;
    init();
}

XLinkObj::XLinkObj(QGraphicsItem *parent, Link *l) : MapObj(parent)
{
    //qDebug()<< "Const XLinkObj (Link, parent)";
    link = l;
    init();
}

XLinkObj::~XLinkObj()
{
    //qDebug() << "Destr XLinkObj";
    delete (poly);
    delete (path);

    // If ctrl point is selected, then deleting ctrl point will
    // also delete selection_ellipse
    delete (c0_ellipse);
    delete (c1_ellipse);

    delete (endArrow);
    delete (beginArrow);
}

void XLinkObj::init()
{
    visBranch = nullptr;

    stateVis = Hidden;

    QPen pen = link->getPen();

    QGraphicsScene *scene = link->getBeginBranch()->getBranchContainer()->scene();
    scene->addItem(this);

    path = scene->addPath(QPainterPath(), pen, Qt::NoBrush);

    beginArrow = new ArrowObj(this);
    beginArrow->setPen(pen);
    beginArrow->setUseFixedLength(true);
    beginArrow->setFixedLength(0);

    endArrow = new ArrowObj(this);
    endArrow->setPen(pen);
    endArrow->setUseFixedLength(true);
    endArrow->setFixedLength(0);

    pen.setStyle(Qt::SolidLine);
    poly = scene->addPolygon(QPolygonF(), pen, pen.color());

    // Control points for bezier path
    // (We have at least a begin branch, consider its orientation)
    initC0();
    initC1();

    c0_ellipse = scene->addEllipse(0, 0, clickBorder * 2,
                                  clickBorder * 2, pen, pen.color());
    c1_ellipse = scene->addEllipse(0, 0, clickBorder * 2,
                                  clickBorder * 2, pen, pen.color());

    selection_ellipse = nullptr;

    beginOrient = endOrient = BranchContainer::UndefinedOrientation;
    pen.setWidth(1);
    pen.setStyle(Qt::DashLine);

    selectionTypeInt = Empty;

    setVisibility(true);
}

QPointF XLinkObj::getAbsPos()
{
    switch (selectionTypeInt) {
    case C0:
        return c0;
        break;
    case C1:
        return c1;
        break;
    default:
        return QPointF();
        break;
    }
}

void XLinkObj::setStyleBegin(const QString &s) { beginArrow->setStyleEnd(s); }

void XLinkObj::setStyleBegin(ArrowObj::OrnamentStyle os)
{
    beginArrow->setStyleEnd(os);
}

ArrowObj::OrnamentStyle XLinkObj::getStyleBegin()
{
    return beginArrow->getStyleEnd();
}

void XLinkObj::setStyleEnd(const QString &s) { endArrow->setStyleEnd(s); }

void XLinkObj::setStyleEnd(ArrowObj::OrnamentStyle os)
{
    endArrow->setStyleEnd(os);
}

ArrowObj::OrnamentStyle XLinkObj::getStyleEnd()
{
    return endArrow->getStyleEnd();
}

QPointF XLinkObj::getBeginPos() { return beginPos; }

QPointF XLinkObj::getEndPos() { return endPos; }


void XLinkObj::setEnd(QPointF p) { endPos = p; }

void XLinkObj::setSelectionType(SelectionType s)
{
    selectionTypeInt = s;
    updateVisibility();
}

void XLinkObj::updateGeometry()
{
    QPointF a, b;
    QPolygonF pa;

    //qDebug() << "XLO::updateGeometry";
    BranchContainer *beginBC = nullptr;
    BranchContainer *endBC = nullptr;
    BranchItem *bi = link->getBeginBranch();
    if (bi)
        beginBC = bi->getBranchContainer();
    bi = link->getEndBranch();
    if (bi)
        endBC = bi->getBranchContainer();

    /* FIXME-2 check orientation to position xlink ctrl point
    if (beginBC) {
        if (beginOrient != BranchContainer::UndefinedOrientation &&
            beginOrient != beginBC->getOrientation())
            c0.setX(-c0.x());
        beginOrient = beginBC->getOrientation();
    }
    if (endBC) {
        if (endOrient != BranchContainer::UndefinedOrientation &&
            endOrient != endBC->getOrientation())
            c1.setX(-c1.x());
        endOrient = endBC->getOrientation();
    }
    */

    if (visBranch) {
        // Only one of the linked branches is visible
        // Draw arrowhead   //FIXME-3 missing shaft of arrow
        BranchContainer *bc = visBranch->getBranchContainer();
        if (!bc)
            return;

        a = b = bc->downLinkPos();

        if (bc->getOrientation() == BranchContainer::RightOfParent) {
            b.setX(b.x() + 2 * arrowSize);
            pa.clear();
            pa << a << b << QPointF(b.x(), b.y() - arrowSize)
               << QPointF(b.x() + arrowSize, b.y())
               << QPointF(b.x(), b.y() + arrowSize) << b << a;
            poly->setPolygon(pa);
        }
        else {
            b.setX(b.x() - 2 * arrowSize);
            pa.clear();
            pa << a << b << QPointF(b.x(), b.y() - arrowSize)
               << QPointF(b.x() - arrowSize, b.y())
               << QPointF(b.x(), b.y() + arrowSize) << b << a;
            poly->setPolygon(pa);
        }
    }
    else {
        // Both linked branches are visible

        // If a link is just drawn in the editor,
        // we have already a beginBranch
        if (beginBC)
            beginPos = beginBC->downLinkPos();
        if (endBC)
            endPos = endBC->downLinkPos();

        if (beginBC && endBC) {
            beginArrow->setPos(beginPos);
            beginArrow->setEndPoint(beginPos + c0);

            endArrow->setPos(endPos);
            endArrow->setEndPoint(endPos + c1);
        }
    }

    // Update control points for bezier
    QPainterPath p(beginPos);
    p.cubicTo(beginPos + c0, endPos + c1, endPos);

    clickPath = p;
    path->setPath(p);

    // Go back to create closed curve,
    // needed for intersection check:
    clickPath.cubicTo(endPos + c1, beginPos + c0, beginPos);

    QPen pen = link->getPen();
    path->setPen(pen);
    poly->setBrush(pen.color());

    beginArrow->setPen(pen);
    endArrow->setPen(pen);

    pen.setStyle(Qt::SolidLine);

    c0_ellipse->setPos(beginPos + c0);
    c0_ellipse->setRect(- pointRadius / 2, - pointRadius / 2, pointRadius, pointRadius);
    c0_ellipse->setPen(pen);
    c0_ellipse->setBrush(pen.color());

    c1_ellipse->setPos(endPos + c1);
    c1_ellipse->setRect(- pointRadius / 2, - pointRadius / 2, pointRadius, pointRadius);
    c1_ellipse->setPen(pen);
    c1_ellipse->setBrush(pen.color());

    BranchItem *bi_begin = link->getBeginBranch();
    BranchItem *bi_end = link->getEndBranch();

    // called often during drawing, but needed during reposition
    // caused by toggling scroll state
    updateVisibility();
}

void XLinkObj::updateVisibility()
{
    BranchContainer *beginBC = nullptr;
    BranchItem *beginBI = link->getBeginBranch();
    if (beginBI)
        beginBC = beginBI->getBranchContainer();

    BranchItem *endBI = link->getEndBranch();
    BranchContainer *endBC = nullptr;
    if (endBI)
        endBC = endBI->getBranchContainer();

    //qDebug() << "XLO::updateVis() beginBI="<<beginBI << "endBI=" << endBI;

    if (beginBC && endBC) {
        if (beginBC->isVisible() &&
            endBC->isVisible()) { // Both ends are visible
            visBranch = nullptr;
            if (selectionTypeInt != Empty)
                stateVis = FullShowControls;
            else
                stateVis = Full;
            setVisibility(true);
        }
        else {
            if (!beginBC->isVisible() &&
                !endBC->isVisible()) { // None of the ends is visible
                visBranch = nullptr;
                stateVis = Hidden;
                setVisibility(false);
            }
            else { // Just one end is visible, draw a symbol that shows
                // that there is a link to a scrolled branch
                if (beginBC->isVisible()) {
                    stateVis = OnlyBegin;
                    visBranch = beginBI;
                }
                else {
                    visBranch = endBI;
                    stateVis = OnlyEnd;
                }
                setVisibility(true);
            }
        }
    }
}

void XLinkObj::setVisibility(bool b)
{
    //qDebug() << "XLO::setVis(b)  b=" << b << "stateVis=" << stateVis;
    if (stateVis == FullShowControls) {
        c0_ellipse->show();
        c1_ellipse->show();
        beginArrow->setUseFixedLength(false);
        endArrow->setUseFixedLength(false);
    }
    else {
        c0_ellipse->hide();
        c1_ellipse->hide();
        beginArrow->setUseFixedLength(true);
        beginArrow->setFixedLength(0);
        endArrow->setUseFixedLength(true);
        endArrow->setFixedLength(0);
    }

    if (b) {
        if (stateVis == OnlyBegin) {
            path->hide();
            poly->show();
            beginArrow->hide();
            endArrow->hide();
        }
        else if (stateVis == OnlyEnd) {
            path->hide();
            poly->show();
            beginArrow->hide();
            endArrow->hide();
        }
        else {
            path->show();
            poly->hide();
            beginArrow->show();
            endArrow->show();
        }
    }
    else {
        poly->hide();
        path->hide();
        beginArrow->hide();
        endArrow->hide();
    }
}

void XLinkObj::initC0()
{
    if (!link)
        return;
    BranchItem *beginBranch = link->getBeginBranch();
    if (!beginBranch)
        return;
    BranchContainer *bc = beginBranch->getBranchContainer();
    if (!bc)
        return;
    if (bc->getOrientation() == BranchContainer::RightOfParent)
        c0 = QPointF(d_control, 0);
    else
        c0 = QPointF(-d_control, 0);
}

void XLinkObj::initC1()
{
    if (!link)
        return;
    BranchItem *endBranch = link->getEndBranch();
    if (!endBranch)
        return;
    BranchContainer *bc =endBranch->getBranchContainer();
    if (!bc)
        return;
    if (bc->getOrientation() == BranchContainer::RightOfParent)
        c1 = QPointF(d_control, 0);
    else
        c1 = QPointF(-d_control, 0);
}

void XLinkObj::setC0(const QPointF &p) { c0 = p; }

QPointF XLinkObj::getC0() { return c0; }

void XLinkObj::setC1(const QPointF &p)
{
    c1 = p;
}

QPointF XLinkObj::getC1() { return c1; }

// FIXME-3 XLO::setSelection only needed in VM and XLI to "update" selection  
void XLinkObj::setSelectedCtrlPoint(const QPointF &p)
{
    switch (selectionTypeInt) {
        case C0:
            c0 = p - beginPos;
            break;
        case C1:
            c1 = p - endPos;
            break;
        default:
            break; }
    updateGeometry();
}

QRectF XLinkObj::boundingRect() {
    // Return bounding box in scene coordinates of selected ctrl point
    switch (selectionTypeInt) {
        case C0:
            [[fallthrough]]; // intentional fallthrough
        case C1:
            return selection_ellipse->mapToScene(selection_ellipse->boundingRect()).boundingRect();
        default:
            return QRectF();
    }
}

XLinkObj::SelectionType XLinkObj::couldSelect(const QPointF &p)
{
    QPointF v;
    qreal d;
    qreal d_max = 10;
    switch (stateVis) {
        case FullShowControls:
            v = c0_ellipse->pos() - p;
            d = Geometry::distance(c0_ellipse->pos(), p);
            if (d < d_max) {
                setSelectionType(C0);
                return C0;
            }

            v = c1_ellipse->pos() - p;
            d = Geometry::distance(c1_ellipse->pos(), p);
            if (d < d_max) {
                setSelectionType(C1);
                return C1;
            }
            break;
        case OnlyBegin:
        case OnlyEnd:
            // not selected, only partially visible
            /*
            if (poly->boundingRect().contains(p))
                b = true;
            */
            break;
        default:
            // not selected, but path is fully visible
            QRectF r(p.x() - clickBorder,
                    p.y() - clickBorder,
                    clickBorder * 2,
                    clickBorder * 2);
            if (clickPath.intersects(r))
                return Path;
    }
    return XLinkObj::Empty;
}

void XLinkObj::select(const QPen &pen, const QBrush &brush)
{
    if (!selection_ellipse) {
        QGraphicsScene *scene = link->getBeginBranch()->getBranchContainer()->scene();
        qreal r = clickBorder * 2.5;
        selection_ellipse = scene->addEllipse(-r / 2 , -r / 2, r, r, pen, brush);
        selection_ellipse->setFlag(QGraphicsItem::ItemStacksBehindParent);
    }

    switch (selectionTypeInt) {
        case C0:
            selection_ellipse->setParentItem(c0_ellipse);
            break;
        case C1:
            selection_ellipse->setParentItem(c1_ellipse);
            break;
        default:
            break;
    }
}

void XLinkObj::unselect()
{
    delete selection_ellipse;
    selection_ellipse = nullptr;
    setSelectionType(Empty);
}

