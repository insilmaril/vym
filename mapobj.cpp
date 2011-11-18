#include "geometry.h"
#include "mapobj.h"
#include "misc.h"

/////////////////////////////////////////////////////////////////
// MapObj
/////////////////////////////////////////////////////////////////
MapObj::MapObj ()
{
    //qWarning ( "Const MapObj (): Please set scene somehow!!!");
    scene=NULL;
    init ();
}

MapObj::MapObj (QGraphicsScene *s, TreeItem *ti)
{
//  cout << "Const MapObj\n";
    scene=s;
    treeItem=ti;
    init ();
}


MapObj::MapObj (MapObj* mo)
{
//    cout << "CopyConst MapObj\n";
    copy (mo);
}

MapObj::~MapObj ()
{
//    cout << "Destr MapObj\n";
}

void MapObj::init ()
{
    absPos=QPointF(0,0);
    visible=true;
    boundingPolygon=NULL;
}

void MapObj::copy(MapObj* other)
{
//    scene=other->scene;   // already set in constr. of child, use that one...
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


QGraphicsScene* MapObj::getScene()
{
    return scene;
}

qreal MapObj::x() 
{
    return absPos.x();
}

qreal MapObj::y() 
{
    return absPos.y();
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
    absPos.setX( x);
    absPos.setY( y);
    bbox.moveTo(QPointF(x,y));
    clickPoly=QPolygonF (bbox); 
}

void MapObj::move (QPointF p)
{
    absPos=p;
    bbox.moveTo (p);
    clickPoly=QPolygonF (bbox);
}

void MapObj::moveBy (double x, double y) 
{
    MapObj::move (x+absPos.x(),y+absPos.y() );
    bbox.moveTo (bbox.x()+x,bbox.y()+y);
    clickPoly.translate (x,y);
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

bool MapObj::isInClickBox (const QPointF &p)
{
    return  clickPoly.containsPoint (p,Qt::OddEvenFill);
}

QSizeF MapObj::getSize()
{
    return bbox.size();
}


bool MapObj::isVisibleObj()
{
    return visible;
}

void MapObj::setVisibility(bool v)
{
    visible=v;
}

