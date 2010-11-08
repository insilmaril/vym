#include "floatobj.h"
#include "mapitem.h"

/////////////////////////////////////////////////////////////////
// FloatObj
/////////////////////////////////////////////////////////////////

FloatObj::FloatObj (QGraphicsScene* s, TreeItem *ti):OrnamentedObj(s,ti)
{
    //cout << "Const FloatObj s="<<s<<"  ti="<<ti<<"  treeItem="<<treeItem<<endl;
    setParObj (this);	
    init();
}

FloatObj::~FloatObj ()
{
//   cout << "Destr FloatObj\n";
}

void FloatObj::init () 
{
    zPlane=Z_INIT;
    setLinkStyle (LinkableMapObj::Parabel);
    ((MapItem*)treeItem)->setHideLinkUnselected(true);
}

void FloatObj::copy (FloatObj* other)
{
    LinkableMapObj::copy (other);
    setVisibility (other->visible);
}

void FloatObj::setZValue(const int &i)
{
    zPlane=i;
}

int FloatObj::zValue()
{
    return zPlane;
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
    move2RelPos (relPos);
    updateLinkGeometry();   
}

QRectF FloatObj::getBBoxSizeWithChildren()
{
    return bboxTotal;
}

