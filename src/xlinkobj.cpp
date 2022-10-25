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
    qDebug()<< "Const XLinkObj (link)";
    link = l;
    init();
}

XLinkObj::XLinkObj(QGraphicsItem *parent, Link *l) : MapObj(parent)
{
    qDebug()<< "Const XLinkObj (Link, parent)";
    link = l;
    init();
}

XLinkObj::~XLinkObj()
{
    qDebug() << "Destr XLinkObj";
    delete (poly);
    delete (path);
    delete (ctrl_p0);
    delete (ctrl_p1);
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
    path->setZValue(dZ_XLINK);

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
    poly->setZValue(dZ_XLINK);

    // Control points for bezier path
    // (We have at least a begin branch, consider its orientation)
    initC0();
    initC1();

    ctrl_p0 = scene->addEllipse(0, 0, clickBorder * 2,
                                  clickBorder * 2, pen, pen.color());
    ctrl_p1 = scene->addEllipse(0, 0, clickBorder * 2,
                                  clickBorder * 2, pen, pen.color());

    beginOrient = endOrient = BranchContainer::UndefinedOrientation;
    pen.setWidth(1);
    pen.setStyle(Qt::DashLine);

    curSelection = Empty;

    setVisibility(true);
}

QPointF XLinkObj::getAbsPos()
{
    switch (curSelection) {
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

void XLinkObj::move(QPointF p) // FIXME-0 update ctrl points directly, not c0 and c1? // FIXME-0 make const FIXME-00 remove completely!!!!!
{
    switch (curSelection) {
        case C0:
            //c0 = ctrl_p0->sceneTransform().inverted().map(p);
            c0 = p;

            break;
        case C1:
            c1 = p;
            break;
        default:
            break; }
    updateXLink();
}

void XLinkObj::setEnd(QPointF p) { endPos = p; }

void XLinkObj::setSelection(SelectionType s)
{
    curSelection = s;
    setVisibility();
}

void XLinkObj::setSelection(int cp) // FIXME-0 needed?
{
    if (cp == 0)
        setSelection(C0);
    else if (cp == 1)
        setSelection(C1);
    else
        qWarning() << "XLO::setSelection cp=" << cp;
}

void XLinkObj::updateXLink() // FIXME-2 rewrite to containers
{
    QPointF a, b;
    QPolygonF pa;

    BranchContainer *beginBC = nullptr;
    BranchContainer *endBC = nullptr;
    BranchItem *bi = link->getBeginBranch();
    if (bi)
        beginBC = bi->getBranchContainer();
    bi = link->getEndBranch();
    if (bi)
        endBC = bi->getBranchContainer();

    /* FIXME-000 check orientation
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

        a = b = bc->scenePos(); // FIXME-0 bc->getChildRefPos();   // FIXME-2

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
            beginPos = beginBC->scenePos(); // FIXME-2 beginBC->getChildRefPos();
        if (endBC)
            endPos = endBC->scenePos(); // FIXME-2 endBC->getChildRefPos();

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
    //endArrow->setPen(pen);
    endArrow->setPen(QPen(Qt::red));

    pen.setStyle(Qt::SolidLine);

    ctrl_p0->setPos(beginPos + c0);
    ctrl_p0->setRect(- pointRadius / 2, - pointRadius / 2, pointRadius, pointRadius);
    ctrl_p0->setPen(pen);
    ctrl_p0->setBrush(pen.color());

    ctrl_p1->setPos(endPos + c1);
    ctrl_p1->setRect(- pointRadius / 2, - pointRadius / 2, pointRadius, pointRadius);
    ctrl_p1->setPen(pen);
    ctrl_p1->setBrush(pen.color());

    BranchItem *bi_begin = link->getBeginBranch();
    BranchItem *bi_end = link->getEndBranch();
    if (bi_begin && bi_end && link->getState() == Link::activeXLink)
        // Note: with MapObj being a GraphicsItem now, maybe better reparent the
        // xlinkobj line->setZValue (dZ_DEPTH *
        // max(bi_begin->depth(),bi_end->depth()) + dZ_XLINK);
        path->setZValue(dZ_XLINK);
    else
        path->setZValue(dZ_XLINK);

    setVisibility();
}

void XLinkObj::setVisibility(bool b)
{
    if (stateVis == FullShowControls) {
        ctrl_p0->show();
        ctrl_p1->show();
        beginArrow->setUseFixedLength(false);
        endArrow->setUseFixedLength(false);
    }
    else {
        ctrl_p0->hide();
        ctrl_p1->hide();
        beginArrow->setUseFixedLength(true);
        beginArrow->setFixedLength(0);
        endArrow->setUseFixedLength(true);
        endArrow->setFixedLength(0);
    }

    MapObj::setVisibility(b);
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

void XLinkObj::setVisibility()
{
    BranchContainer *beginBC = nullptr;
    BranchItem *beginBI = link->getBeginBranch();
    if (beginBI)
        beginBC = beginBI->getBranchContainer();

    BranchItem *endBI = link->getEndBranch();
    BranchContainer *endBC = nullptr;
    if (endBI)
        endBC = endBI->getBranchContainer();
    if (beginBC && endBC) {
        if (beginBC->isVisible() &&
            endBC->isVisible()) { // Both ends are visible
            visBranch = NULL;
            if (curSelection != Empty)
                stateVis = FullShowControls;
            else
                stateVis = Full;
            setVisibility(true);
        }
        else {
            if (!beginBC->isVisible() &&
                !endBC->isVisible()) { // None of the ends is visible
                visBranch = NULL;
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

void XLinkObj::setSelectedCtrlPoint(const QPointF &p)
{
    switch (curSelection) {
        case C0:
            c0 = p - beginPos;
            break;
        case C1:
            c1 = p - endPos;
            break;
        default:
            break; }
    updateXLink();
}

QPointF XLinkObj::getSelectedCtrlPoint()    // FIXME-00 Really still used?
{
    switch (curSelection) {
        case C0:
            return c0;
            break;
        case C1:
            return c1;
        default:
            return QPoint();
    }
}

int XLinkObj::ctrlPointInClickBox(const QPointF &p) // Still needed? couldSelect instead?
{
    SelectionType oldSel = curSelection;
    int ret = -1;

    QRectF r(p.x() - clickBorder, p.y() - clickBorder, clickBorder * 2,
             clickBorder * 2);

    if (curSelection == C0 || curSelection == C1) {
        // If Cx selected, check both ctrl points
        curSelection = C0;
        if (getClickPath().intersects(r))       // FIXME-00 this construct looks awkward
            ret = 0;
        curSelection = C1;
        if (getClickPath().intersects(r))
            ret = 1;
    }
    curSelection = oldSel;
    qDebug() << "XLO::ctrlPointInClickBox  p=" << qpointFToString(p,0) << " ret=" << ret;    // FIXME-2 testing
    return ret;
}

XLinkObj::SelectionType XLinkObj::couldSelect(const QPointF &p)
{
    QPointF v;
    qreal d;
    qreal d_max = 10;
    switch (stateVis) {
        case FullShowControls:
            v = ctrl_p0->pos() - p;
            d = Geometry::distance(ctrl_p0->pos(), p);
            if (d < d_max) {
                setSelection(C0);
                return C0;
            }

            v = ctrl_p1->pos() - p;
            d = Geometry::distance(ctrl_p1->pos(), p);
            if (d < d_max) {
                setSelection(C1);
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

bool XLinkObj::isInClickBox(const QPointF &p) // FIXME-00 remove, not needed
{
    // Return, if not visible at all...
    if (stateVis == Hidden)
        return false;

    SelectionType oldSel = curSelection;
    bool b = false;

    QRectF r(p.x() - clickBorder, p.y() - clickBorder, clickBorder * 2,
             clickBorder * 2);

    switch (stateVis) {
        case FullShowControls:
            // If Cx selected, check both ctrl points
            if (ctrlPointInClickBox(p) > -1)
                b = true;

            // Enable selecting the path, when a ctrl point is already selected
            if (!b && curSelection != Empty && clickPath.intersects(r))
                b = true;
            break;
        case OnlyBegin:
        case OnlyEnd:
            // not selected, only partially visible
            if (poly->boundingRect().contains(p))
                b = true;
            break;
        default:
            // not selected, but path is fully visible
            curSelection = Path;
            if (getClickPath().intersects(r))
                b = true;
            break;
    }
    curSelection = oldSel;
    return b;
}

QPainterPath
XLinkObj::getClickPath() // also needs mirroring if oriented left. Create method
                         // to generate the coordinates
{
    QPainterPath p;
    switch (curSelection) {
    case C0:
        p.addEllipse(beginPos + c0, 15, 15);
        return p;
        break;
    case C1:
        p.addEllipse(endPos + c1, 15, 15);
        return p;
        break;
    default:
        return clickPath;
        break;
    }
}
