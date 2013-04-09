#include <QDebug>

#include "floatobj.h"
#include "mapitem.h"

extern bool debug;

/////////////////////////////////////////////////////////////////
// FloatObj
/////////////////////////////////////////////////////////////////

FloatObj::FloatObj (QGraphicsItem *parent, TreeItem *ti):OrnamentedObj(parent,ti)
{
    //qDebug() << "Const FloatObj s="<<s<<"  ti="<<ti<<"  treeItem="<<treeItem;
    init();
}

FloatObj::~FloatObj ()
{
//   qDebug() << "Destr FloatObj";
}

void FloatObj::init () 
{
    setLinkStyle (LinkableMapObj::Parabel);
    ((MapItem*)treeItem)->setHideLinkUnselected(true);
}

void FloatObj::copy (FloatObj* other)
{
    LinkableMapObj::copy (other);
    setVisibility (other->visible);
}

void FloatObj::move (double x, double y)    // FIXME-8 Changed to use centers for now
{
    MapObj::move(x,y);
}

void FloatObj::move (QPointF p)
{
    FloatObj::move(p.x(), p.y());
}

void FloatObj::moveCenter (double x, double y)
{
    absPos=QPointF(x,y);
    bbox.moveTo(x - bbox.width()/2, y - bbox.height()/2 );
    clickPoly=QPolygonF (bbox);
    if (debug) qDebug()<<"FO::moveCenter "<<x<<","<<y<<"  bbox="<<bbox;
}

void FloatObj::moveCenter2RelPos(double x, double y)  // FIXME-8 
{
    setRelPos (QPointF(x,y));
    if (parObj)
    {
	QPointF p=parObj->getFloatRefPos();
	moveCenter (p.x() + x, p.y() + y);
    }
}

void FloatObj::move2RelPos(double x, double y)  // FIXME-8 overloaded to use floatRefPos instead of childRefPos
{
    setRelPos (QPointF(x,y));
    if (parObj)
    {
	QPointF p=parObj->getFloatRefPos();
	move (p.x() + x, p.y() + y);
    }
}

void FloatObj::move2RelPos(QPointF p)           // FIXME-8 overloaded to use floatRefPos instead of childRefPos
{
    move2RelPos (p.x(), p.y());
}


void FloatObj::setDockPos()
{
    parPos=absPos;
    childRefPos=absPos; // FIXME-8  better floatRefPos? used at all?
}

void FloatObj::reposition()
{
    if (debug) 
    {
        qDebug()<<"FO:reposition relPos="<<relPos; //FIXME-8
        if (parObj)
        {
            qDebug()<<"    parObj->childRefPos="<<parObj->getChildRefPos();
            qDebug()<<"    parObj->floatRefPos="<<parObj->getFloatRefPos();
        }
    }
    moveCenter2RelPos (relPos.x(), relPos.y());
    updateLinkGeometry();   
}

QRectF FloatObj::getBBoxSizeWithChildren()
{
    return bboxTotal;
}

