#include <iostream>
#include <math.h>

#include "linkable-container.h"
#include "vymmodel.h"

extern bool debug;

/////////////////////////////////////////////////////////////////
// LinkableContainer
/////////////////////////////////////////////////////////////////

LinkableContainer::LinkableContainer(QGraphicsItem *parent = nullptr)
    : Container(parent)
{
    // qDebug() << "Const LinkableContainer this=" << this;
    init();
}

LinkableContainer::~LinkableContainer()
{
    // qDebug()<< "Destructor LC  this="<<this<<" style="<<style<<" l="<<l<<"
    // p="<<p<<"  segment="<<segment.count();
    delLink();
}

void LinkableContainer::init()
{
    parPos = QPointF(0, 0);
    childRefPos = QPointF(0, 0);
    floatRefPos = QPointF(0, 0);
    link2ParPos = false;
    l = NULL;
    p = NULL;
    linkwidth = 20;
    thickness_start = 8;
    style = UndefinedStyle;
    linkpos = Bottom;
    arcsegs = 13;

    // TODO instead of linkcolor pen.color() could be used	all around
    pen.setWidth(1);
    pen.setColor(linkcolor);
    pen.setCapStyle(Qt::RoundCap);

    useBottomline = false;
    bottomline = NULL;
}

void LinkableContainer::createBottomLine()
{
    bottomline = scene()->addLine(QLineF(1, 1, 1, 1), pen);
    bottomline->setZValue(dZ_LINK);
}

void LinkableContainer::delLink()
{
    if (bottomline) {
        delete (bottomline);
        bottomline = NULL;
    }
    switch (style) {
    case Line:
        delete (l);
        break;
    case Parabel:
        while (!segment.isEmpty())
            delete segment.takeFirst();
        break;
    case PolyLine:
        delete (p);
        break;
    case PolyParabel:
        delete (p);
        break;
    default:
        break;
    }
}

void LinkableContainer::copy(LinkableContainer *other)
{
    Container::copy(other);
    setLinkStyle(other->style);
    setLinkColor(other->linkcolor);
}

void LinkableContainer::setLinkStyle(Style newstyle)
{
    // qDebug()<<"LC::setLinkStyle s="<<newstyle;	//FIXME-4 called very
    // often?!?! qDebug()<<"LC::setLinkStyle s="<<newstyle<<" for "<<this<<"
    // "<<treeItem->getHeading()<<"  parentContainer="<<parentContainer;
    delLink();

    style = newstyle;

    QGraphicsLineItem *cl;
    switch (style) {
    case Line:
        l = scene()->addLine(QLineF(1, 1, 1, 1), pen);
        l->setZValue(dZ_LINK);
        if (visible)
            l->show();
        else
            l->hide();
        createBottomLine();
        break;
    case Parabel:
        for (int i = 0; i < arcsegs; i++) {
            cl = scene()->addLine(QLineF(i * 5, 0, i * 10, 100), pen);
            cl->setZValue(dZ_LINK);
            if (visible)
                cl->show();
            else
                cl->hide();
            segment.append(cl);
        }
        pa0.resize(arcsegs + 1);
        createBottomLine();
        break;
    case PolyLine:
        p = scene()->addPolygon(QPolygonF(), pen, linkcolor);
        p->setZValue(dZ_LINK);
        if (visible)
            p->show();
        else
            p->hide();
        pa0.resize(3);
        createBottomLine();
        break;
    case PolyParabel:
        p = scene()->addPolygon(QPolygonF(), pen, linkcolor);
        p->setZValue(dZ_LINK);
        if (visible)
            p->show();
        else
            p->hide();
        pa0.resize(arcsegs * 2 + 2);
        pa1.resize(arcsegs + 1);
        pa2.resize(arcsegs + 1);
        createBottomLine();
        break;
    default:
        break;
    }
}

LinkableContainer::Style LinkableContainer::getLinkStyle() { return style; }

void LinkableContainer::setLinkPos(Position lp) { linkpos = lp; }

LinkableContainer::Position LinkableContainer::getLinkPos() { return linkpos; }

void LinkableContainer::setLinkColor(QColor col)
{
    linkcolor = col;
    pen.setColor(col);
    if (bottomline)
        bottomline->setPen(pen);
    switch (style) {
    case Line:
        l->setPen(pen);
        break;
    case Parabel:
        for (int i = 0; i < segment.size(); ++i)
            segment.at(i)->setPen(pen);
        break;
    case PolyLine:
        p->setBrush(QBrush(col));
        p->setPen(pen);
        break;
    case PolyParabel:
        p->setBrush(QBrush(col));
        p->setPen(pen);
        break;
    default:
        break;
    }
}

QColor LinkableContainer::getLinkColor() { return linkcolor; }

void LinkableContainer::setVisibility(bool v)
{
    Container::setVisibility(v);
    updateVisibility();
}

void LinkableContainer::updateVisibility()
{
    bool visnow = visible;

    // FIXME-1 Hide links of unselected objects (if wanted)
    /*
    if (((MapItem *)treeItem)->getHideLinkUnselected() &&
        !treeItem->getModel()->isSelected(treeItem))
        visnow = false;
    */

    if (visnow) {
        if (bottomline) {
            if (useBottomline)
                bottomline->show();
            else
                bottomline->hide();
        }

        switch (style) {
        case Line:
            if (l)
                l->show();
            break;
        case Parabel:
            for (int i = 0; i < segment.size(); ++i)
                segment.at(i)->show();
            break;
        case PolyLine:
            if (p)
                p->show();
            else
                qDebug() << "LC::updateVis p==0 (PolyLine)"; // FIXME-4
            break;
        case PolyParabel:
            if (p)
                p->show();
            else
                qDebug() << "LC::updateVis p==0 (PolyParabel) ";
                         //<< treeItem->getHeadingPlain(); // FIXME-4
            break;
        default:
            break;
        }
    }
    else {
        if (bottomline)
            bottomline->hide();
        switch (style) {
        case Line:
            if (l)
                l->hide();
            break;
        case Parabel:
            for (int i = 0; i < segment.size(); ++i)
                segment.at(i)->hide();
            break;
        case PolyLine:
            if (p)
                p->hide();
            break;
        case PolyParabel:
            if (p)
                p->hide();
            break;
        default:
            break;
        }
    }
}

void LinkableContainer::updateLinkGeometry()
{
    // needs:
    //	childRefPos of parent
    //	orient   of parent
    //	style
    //
    // sets:
    //	childRefPos    (by calling setDockPos())
    //	parPos	    (by calling setDockPos())
    //  bottomlineY
    //	drawing of the link itself

    // updateLinkGeometry is called from move, but called from constructor we
    // don't have parents yet...

    if (style == UndefinedStyle) {
        setDockPos();
        return;
    }

    switch (linkpos) {
        case Middle:
            //FIXME-0 bottomlineY = bbox.top() + bbox.height() / 2; // draw link to middle (of frame)
            break;
        case Bottom:
            // bottomlineY = bbox.bottom()-1;  // draw link to bottom of box
            // FIXME-0 bottomlineY = bbox.bottom(); // FIXME-1 - botPad;
            break;
    }

    double p2x, p2y; // Set P2 Before setting
    if (!link2ParPos) {
        p2x = QPointF(parentContainer->getChildRefPos()).x(); // P1, we have to look at
        p2y = QPointF(parentContainer->getChildRefPos()).y(); // orientation
    }
    else {
        p2x = QPointF(parentContainer->getParPos()).x();
        p2y = QPointF(parentContainer->getParPos()).y();
    }

    //FIXME-1 no longer here: setOrientation();
    setDockPos(); // Call overloaded method

    double p1x = parPos.x(); // Link is drawn from P1 to P2
    double p1y = parPos.y();

    double vx = p2x - p1x; // V=P2-P1
    double vy = p2y - p1y;

    int z;
    // FIXME-1 Hack to z-move links to MapCenter (d==1) below MCOs frame (d==0)
    // //FIXME-4 no longer used?
    /*
    if (treeItem->depth() < 2)
        // z=(treeItem->depth() -2)*dZ_DEPTH + dZ_LINK;
        z = -dZ_LINK;
    else
        z = dZ_LINK;
    */

    // qDebug()<<"LC::updateGeo d="<<treeItem->depth()<<"  this="<<this<<"
    // "<<treeItem->getHeading();

    // Draw the horizontal line below heading (from childRefPos to ParPos)
    if (bottomline) {
        bottomline->setLine(QLineF(childRefPos.x(), childRefPos.y(), p1x, p1y));
        bottomline->setZValue(z);
    }

    double a; // angle
    if (vx > -0.000001 && vx < 0.000001)
        a = M_PI_2;
    else
        a = atan(vy / vx);
    // "turning point" for drawing polygonal links
    QPointF tp(-qRound(sin(a) * thickness_start),
               qRound(cos(a) * thickness_start));

    // Draw the link
    switch (style) {
    case Line:
        l->setLine(QLine(qRound(parPos.x()), qRound(parPos.y()), qRound(p2x),
                         qRound(p2y)));
        l->setZValue(z);
        break;
    case Parabel:
        parabel(pa0, p1x, p1y, p2x, p2y);
        for (int i = 0; i < segment.size(); ++i) {
            segment.at(i)->setLine(QLineF(pa0.at(i).x(), pa0.at(i).y(),
                                          pa0.at(i + 1).x(),
                                          pa0.at(i + 1).y()));
            segment.at(i)->setZValue(z);
        }
        break;
    case PolyLine:
        pa0.clear();
        pa0 << QPointF(qRound(p2x + tp.x()), qRound(p2y + tp.y()));
        pa0 << QPointF(qRound(p2x - tp.x()), qRound(p2y - tp.y()));
        pa0 << QPointF(qRound(parPos.x()), qRound(parPos.y()));
        p->setPolygon(QPolygonF(pa0));
        p->setZValue(z);
        break;
    case PolyParabel:
        parabel(pa1, p1x, p1y, p2x + tp.x(), p2y + tp.y());
        parabel(pa2, p1x, p1y, p2x - tp.x(), p2y - tp.y());
        pa0.clear();
        for (int i = 0; i <= arcsegs; i++)
            pa0 << QPointF(pa1.at(i));
        for (int i = 0; i <= arcsegs; i++)
            pa0 << QPointF(pa2.at(arcsegs - i));
        p->setPolygon(QPolygonF(pa0));
        p->setZValue(z);
        break;
    default:
        break;
    }
}

QPointF LinkableContainer::getChildRefPos() { return childRefPos; }

QPointF LinkableContainer::getFloatRefPos() { return floatRefPos; }

QPointF LinkableContainer::getParPos() { return parPos; }

void LinkableContainer::parabel(QPolygonF &ya, qreal p1x, qreal p1y, qreal p2x,
                             qreal p2y)

{
    qreal vx = p2x - p1x; // V=P2-P1
    qreal vy = p2y - p1y;

    qreal dx; // delta x during calculation of parabel

    qreal pnx; // next point
    qreal pny;
    qreal m;

    if (vx > -0.0001 && vx < 0.0001)
        m = 0;
    else
        m = (vy / (vx * vx));
    dx = vx / (arcsegs);
    ya.clear();
    ya << QPointF(p1x, p1y);
    for (int i = 1; i <= arcsegs; i++) {
        pnx = p1x + dx;
        pny = m * (pnx - parPos.x()) * (pnx - parPos.x()) + parPos.y();
        ya << QPointF(pnx, pny);
        p1x = pnx;
        p1y = pny;
    }
}
