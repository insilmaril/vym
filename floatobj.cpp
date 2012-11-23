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

void FloatObj::move (double x, double y)
{
    MapObj::move(x,y);
}

void FloatObj::move (QPointF p)
{
    MapObj::move (p);
}

void FloatObj::setDockPos()
{
    parPos=absPos;
    childPos=absPos;
}

void FloatObj::reposition()
{
    if (debug) 
    {
        qDebug()<<"FO:reposition relPos="<<relPos; //FIXME-8
        if (parObj)
            qDebug()<<"    parObj->childPos="<<parObj->getChildPos();
    }
    move2RelPos (relPos);
    updateLinkGeometry();   
}

QRectF FloatObj::getBBoxSizeWithChildren()
{
    return bboxTotal;
}

