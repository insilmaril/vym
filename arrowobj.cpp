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
    delete arrowBegin;
    delete arrowEnd;
    delete line;
}

void ArrowObj::init () 
{
    QPen pen;

    pen.setStyle (Qt::SolidLine);
    arrowBegin=scene()->addPolygon (QPolygonF(), pen );	
    arrowBegin->setZValue (dZ_XLINK);
    arrowEnd=scene()->addPolygon (QPolygonF(), pen );	
    arrowEnd->setZValue (dZ_XLINK);

    line=scene()->addLine ( QLineF(), pen );	
    line->setZValue (dZ_XLINK);

    arrowSize=4;
    useFixedLength=false;
    setStyleBegin (None);
    setStyleEnd   (HeadFull);
}

void ArrowObj::setPen (QPen p)
{
    pen = p;
    line->setPen( pen);

    // end shall have same style as xlink
    QPen pen_solid = pen;
    pen_solid.setStyle (Qt::SolidLine);
    arrowBegin->setPen( pen_solid );
    arrowEnd->setPen( pen_solid );

    setStyleBegin( styleBegin );
    setStyleEnd( styleEnd );
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

void ArrowObj::setUseFixedLength( bool b)
{
    useFixedLength = b;
}

bool ArrowObj::getUseFixedLength()
{
    return useFixedLength;
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
        if (styleEnd != None)
            arrowEnd->show();
        else
            arrowEnd->hide();
        if (useFixedLength && fixedLength == 0)
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

void ArrowObj::setStyleBegin (const QString &s) // FIXME-0 missing
{
    if (s=="HeadFull")
        setStyleBegin( ArrowObj::HeadFull );
    else
        setStyleBegin( ArrowObj::None );
}

void ArrowObj::setStyleBegin (OrnamentStyle os) // FIXME-0 missing
{
    // FIXME-0 needs real implementation (and shared with method for end)

    styleBegin = os;
    QPointF a,b,c;
    QPolygonF pa;
    switch (styleBegin) 
    {
        case HeadFull:
            b = a + QPointF( -arrowSize *2, -arrowSize);
            c = a + QPointF( -arrowSize *2, +arrowSize);
            pa << a << b << c;
            arrowBegin->setPolygon( pa );
            arrowBegin->setBrush( pen.color() ); 
            break;
        case Foot: break;
        case None: 
            arrowBegin->setPolygon( QPolygonF() );
            break;
    }
}

ArrowObj::OrnamentStyle ArrowObj::getStyleBegin()
{
    return styleBegin;
}

void ArrowObj::setStyleEnd (const QString &s) // FIXME-0 missing
{
    if (s=="HeadFull")
        setStyleEnd( ArrowObj::HeadFull );
    else
        setStyleEnd( ArrowObj::None );
}

void ArrowObj::setStyleEnd (OrnamentStyle os)
{
    // FIXME-0 needs real implementation (and shared with method for begin)

    styleEnd = os;
    QPointF a,b,c;
    QPolygonF pa;
    switch (styleEnd) 
    {
        case HeadFull:
            b = a + QPointF( -arrowSize *2, -arrowSize);
            c = a + QPointF( -arrowSize *2, +arrowSize);
            pa << a << b << c;
            arrowEnd->setPolygon( pa );
            arrowEnd->setBrush( pen.color() ); 
            break;
        case Foot: break;
        case None: 
            arrowEnd->setPolygon( QPolygonF() );
            break;
    }
}

ArrowObj::OrnamentStyle ArrowObj::getStyleEnd()
{
    return styleEnd;
}

QString ArrowObj::styleToString(const OrnamentStyle &os)
{
    switch (os)
    {
        case HeadFull: return "HeadFull"; break;
        case None: return "None"; break;
        default: qWarning()<<"ArrowObj::styleToString unknown style "<<os;
    }
    return "Unknown";
}

