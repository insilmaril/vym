#include "arrowobj.h"
#include "misc.h"

#include <QDebug>
#include <QGraphicsScene>

/////////////////////////////////////////////////////////////////
// ArrowObj
/////////////////////////////////////////////////////////////////

ArrowObj::ArrowObj (MapObj* parent):MapObj(parent)
{
    init();
}

ArrowObj::~ArrowObj ()
{
    // FIXME-0 delete all used graphicsitems...
    delete arrowEnd;
    delete line;
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

void ArrowObj::setPen (QPen p)
{
    pen = p;
    line->setPen( pen);

    // end shall have same style as xlink
    QPen pen_solid = pen;
    pen_solid.setStyle (Qt::SolidLine);
    arrowEnd->setPen( pen_solid );

    setOrnamentStyleBegin( beginStyle );
    setOrnamentStyleEnd( endStyle );
}

QPen ArrowObj::getPen()
{
    return pen;
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

void ArrowObj::show()
{
    setVisibility( true );
}

void ArrowObj::hide()
{
    setVisibility( false );
}

void ArrowObj::setVisibility (bool b)
{
    MapObj::setVisibility (b);
    if (b)
    {
        if (endStyle != None)
            arrowEnd->show();
        else
            arrowEnd->hide();
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

void ArrowObj::setOrnamentStyleBegin (OrnamentStyle os) // FIXME-0 missing
{
    beginStyle = os;
}

ArrowObj::OrnamentStyle ArrowObj::getOrnamentStyleBegin()
{
    return beginStyle;
}

void ArrowObj::setOrnamentStyleEnd (OrnamentStyle os)
{
    // FIXME-0 needs real implementation (and shared with method for begin)

    endStyle = os;
    QPointF a,b,c;
    QPolygonF pa;
    switch (endStyle) 
    {
        case HeadFull:
            b = a + QPointF( -arrowSize *2, -arrowSize);
            c = a + QPointF( -arrowSize *2, +arrowSize);
            pa << a << b << c;
            arrowEnd->setPolygon( pa );
            arrowEnd->setBrush( pen.color() ); 
            break;
        case Foot: break;
        case None: break;
    }
}

ArrowObj::OrnamentStyle ArrowObj::getOrnamentStyleEnd()
{
    return endStyle;
}

