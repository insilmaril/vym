#include "arrowobj.h"

#include <QPen>

/////////////////////////////////////////////////////////////////
// ArrowObj
/////////////////////////////////////////////////////////////////

ArrowObj::ArrowObj (MapObj* parent):MapObj(parent)
{
    //qDebug()<< "Const ArrowObj ()";
    init();
}

ArrowObj::~ArrowObj ()
{
    //qDebug() << "Destr ArrowObj";
    //delete (polyBeg);
}


void ArrowObj::init () 
{
    QPen pen;

    pen.setStyle (Qt::SolidLine);
    //poly=scene()->addPolygon (QPolygonF(), pen, pen.color());	
    //poly->setZValue (dZ_XLINK);
}

void ArrowObj::setVisibility (bool b)
{
    MapObj::setVisibility (b);
    /*
    if (b)
        poly->show();
    else
	poly->hide();
    */
}

void ArrowObj::setEndPoint (QPointF p)
{
    endPoint = p;
}

QPointF ArrowObj::getEndPoint ()
{
    return endPoint;
}

void ArrowObj::setLengthStyle (LengthStyle l)
{
}

ArrowObj::LengthStyle ArrowObj::getLengthStyle ()
{
}

void ArrowObj::setOrnamentStyleBegin (OrnamentStyle os)
{
}

ArrowObj::OrnamentStyle ArrowObj::getOrnamentStyleBegin()
{
}

void ArrowObj::setOrnamentStyleEnd (OrnamentStyle os)
{
}

ArrowObj::OrnamentStyle ArrowObj::getOrnamentStyleEnd()
{
}

