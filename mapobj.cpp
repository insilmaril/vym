#include <QDebug>

#include "geometry.h"
#include "mapobj.h"
#include "misc.h"

/////////////////////////////////////////////////////////////////
// MapObj
/////////////////////////////////////////////////////////////////
MapObj::MapObj (QGraphicsItem *parent, TreeItem *ti):QGraphicsItem (parent)
{
    //qDebug() << "Const MapObj (this,ti)=("<<this<<","<<ti<<")";
    treeItem=ti;
    init ();
}

MapObj::~MapObj ()
{
    //qDebug() << "Destr MapObj "<<this;
    foreach (QGraphicsItem *i,childItems() ) 
	// Avoid tha QGraphicsScene deletes children
	i->setParentItem (NULL);
}

void MapObj::init ()
{
    absPos=QPointF(0,0);
    visible=true;
    boundingPolygon=NULL;
}

void MapObj::copy(MapObj* other)
{
    absPos=other->absPos;
    bbox.setX (other->bbox.x() );
    bbox.setY (other->bbox.y() );
    bbox.setSize (QSizeF(other->bbox.width(), other->bbox.height() ) );
}

void MapObj::setTreeItem (TreeItem *ti)
{
    treeItem=ti;
}

TreeItem* MapObj::getTreeItem () const
{
    return treeItem;
}

qreal MapObj::x() 
{
    return getAbsPos().x();
}

qreal MapObj::y() 
{
    return getAbsPos().y();
}

qreal MapObj::width() 
{
    return bbox.width();
}

qreal MapObj::height() 
{
    return bbox.height();
}

QPointF MapObj::getAbsPos() 
{
    return absPos;
}

QString MapObj::getPos()
{
    return qpointFToString(absPos);
}

void MapObj::move (double x, double y) 
{
    MapObj::move (QPointF(x,y));
}

void MapObj::move (QPointF p)
{
    absPos=p;
    bbox.moveTo (p);
    clickPoly=QPolygonF (bbox);
}

void MapObj::moveBy (double x, double y) 
{
    QPointF v(x,y);
    MapObj::move (absPos + v );
    bbox.moveTo (bbox.topLeft() + v);
    clickPoly.translate (v);
}

QRectF MapObj::boundingRect () const 
{
    return QRectF();
}

void MapObj::paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*)
{
}

QRectF MapObj::getBBox()
{
    return bbox;
}

ConvexPolygon MapObj::getBoundingPolygon()
{
    QPolygonF p;
    p<<bbox.topLeft()<<bbox.topRight()<<bbox.bottomRight()<<bbox.bottomLeft();
    return p;
}

QPolygonF MapObj::getClickPoly()
{
    return clickPoly;
}

QPainterPath MapObj::getClickPath()
{
    QPainterPath p;
    QRectF br=clickPoly.boundingRect();
    p.moveTo (br.topLeft() );
    p.lineTo (br.topRight() );
    p.lineTo (br.bottomRight() );
    p.lineTo (br.bottomLeft() );
    p.lineTo (br.topLeft() );
    return p;
}

bool MapObj::isInClickBox (const QPointF &p)
{
    return  clickPoly.containsPoint (p,Qt::OddEvenFill);
}

QSizeF MapObj::getSize()
{
    return bbox.size();
}


void MapObj::setRotation (const qreal &a)
{
    angle=a;
}

qreal MapObj::getRotation ()
{
    return angle;
}

bool MapObj::isVisibleObj()
{
    return visible;
}

void MapObj::setVisibility(bool v)
{
    visible=v;
}

