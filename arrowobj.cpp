#include "arrowobj.h"
#include "misc.h"

#include <QDebug>
#include <QGraphicsScene>
#include <QPen>

/////////////////////////////////////////////////////////////////
// ArrowObj
/////////////////////////////////////////////////////////////////

ArrowObj::ArrowObj (MapObj* parent):MapObj(parent)
{
    init();
}

ArrowObj::~ArrowObj ()
{
    //delete (poly);
    // FIXME-0 delete all used graphicsitems...
}


void ArrowObj::init () 
{
    QPen pen;

    pen.setStyle (Qt::SolidLine);
    arrowEnd=scene()->addPolygon (QPolygonF(), pen );	
    arrowEnd->setZValue (dZ_XLINK);

    line=scene()->addLine ( QLineF(), pen );	
    line->setZValue (dZ_XLINK);

    arrowSize=4;
    fixedLength = -1;
    setOrnamentStyleBegin (HeadFull);
    setOrnamentStyleEnd   (HeadFull);
}

void ArrowObj::setColor (QColor c)
{
    color = c;
    QPen pen;
    pen.setStyle (Qt::SolidLine);
    pen.setColor ( color );
    line->setPen( pen );
    arrowEnd->setPen( pen );
}

QColor ArrowObj::getColor()
{
    return color;
}

void ArrowObj::setArrowSize(qreal r)
{
    arrowSize = r;
}

qreal ArrowObj::getArrowSize()
{
    return arrowSize;
}

void ArrowObj::setFixedLength(int i)
{
    fixedLength = i;
}

int ArrowObj::getFixedLength()
{
    return fixedLength;
}

void ArrowObj::setVisibility (bool b)
{
    MapObj::setVisibility (b);
    if (b)
    {
        arrowEnd->show();
        if (fixedLength == 0)
            line->hide();
        else
            line->show();
    }
    else
    {
	arrowEnd->hide();
	line->hide();
    }
}

void ArrowObj::setEndPoint (QPointF p)
{
    endPoint = p;

    line->setLine(absPos.x(),absPos.y(), p.x(), p.y());
    arrowEnd->setPos(absPos);

    qreal a = getAngle( endPoint - absPos );
    arrowEnd->setRotation( -a / 6.28 * 360);
    arrowEnd->setPos( endPoint );
}

QPointF ArrowObj::getEndPoint ()
{
    return endPoint;
}

void ArrowObj::setOrnamentStyleBegin (OrnamentStyle os)
{
}

ArrowObj::OrnamentStyle ArrowObj::getOrnamentStyleBegin()
{
}

void ArrowObj::setOrnamentStyleEnd (OrnamentStyle os)
{
    // FIXME-0 needs real implementation (and shared with method for begin)
    QPointF a,b,c;
    b = a + QPointF( -arrowSize *2, -arrowSize);
    c = a + QPointF( -arrowSize *2, +arrowSize);
    QPolygonF pa;
    pa << a << b << c;
    arrowEnd->setPolygon(pa);
}

ArrowObj::OrnamentStyle ArrowObj::getOrnamentStyleEnd()
{
}

