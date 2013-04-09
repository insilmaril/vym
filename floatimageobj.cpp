#include <QDebug>
#include <QImageReader>

#include "floatimageobj.h"
#include "branchobj.h"

extern bool debug;  //FIXME-8

/////////////////////////////////////////////////////////////////
// FloatImageObj 
/////////////////////////////////////////////////////////////////

FloatImageObj::FloatImageObj (QGraphicsItem * parent,TreeItem *ti):FloatObj(parent,ti)
{
    //qDebug() << "Const FloatImageObj this="<<this<<"  ti="<<ti;
    icon=new ImageObj (parent);
    icon->setPos (absPos.x(), absPos.y() );
    icon->setVisibility (true);
    clickPoly=bbox;
    useRelPos=true;

    //Hide flags
    systemFlags->setShowFlags(false);

//    setLinkStyle (LinkableMapObj::Parabel);
}

FloatImageObj::~FloatImageObj ()
{
//  qDebug() << "Destr FloatImageObj "<<this<<"";
    delete(icon);
}

void FloatImageObj::copy (FloatImageObj* other)
{		    
    FloatObj::copy (other);
    icon->copy (other->icon);
    positionBBox();
}

void FloatImageObj::setZValue (const int &i)
{

//    qDebug()<<"FIO::setZValue z="<<i;
//    qDebug()<<"  icon="<<icon;
//    qDebug()<<"  this="<<this;	 
    icon->setZValue (i);
}

int FloatImageObj::z ()
{
    return qRound (icon->zValue());
}

void FloatImageObj::load (const QImage &img)
{
    icon->load(QPixmap::fromImage(img));
    if (!icon->parentItem() ) icon->setParentItem(this);  // Add to scene initially
    bbox.setSize ( QSizeF(
            icon->boundingRect().width()  + 8, 
            icon->boundingRect().height() + 8));
    clickPoly=bbox;
    positionBBox();
}

void FloatImageObj::setParObj (QGraphicsItem *p)
{
    setParentItem (p);
    icon->setParentItem (p);
    parObj=(LinkableMapObj*)p;
/*
    qDebug()<<"FIO::setParentItem";
    qDebug()<<"  this="<<this;
    qDebug()<<"  icon="<<icon;
*/
}

void FloatImageObj::setVisibility(bool v)
{
    OrnamentedObj::setVisibility(v);
    if (v)
	icon->setVisibility(true);
    else
	icon->setVisibility(false);
}

void FloatImageObj::moveCenter (double x, double y)
{
    FloatObj::moveCenter(x, y);
    icon->setPos(bbox.topLeft() );
    if (debug) qDebug()<<"FIO::moveCenter "<<x<<","<<y<<"  bbox="<<bbox;
}

void FloatImageObj::move (double x, double y)
{
    if (debug) qDebug()<<"FIO::move "<<x<<","<<y;
    FloatObj::move(x,y);
    icon->setPos (x+4,y+4); // FIXME-8 why this offset?? (see bbox in FIO::load)
    positionBBox();
}

void FloatImageObj::move (QPointF p)
{
    if (debug) qDebug()<<"FIO::move "<<p<<" and calling FIO::move";
    FloatImageObj::move (p.x(),p.y());
}

void FloatImageObj::positionBBox()
{
    if (debug) qDebug()<<"FIO::positionBBox";
    clickPoly=QPolygonF(bbox);
    setZValue (dZ_FLOATIMG);
}

void FloatImageObj::calcBBoxSize()
{
    // TODO
}

QRectF FloatImageObj::getBBoxSizeWithChildren()
{
    //TODO abstract in linkablemapobj.h, not calculated
    return bboxTotal;
}

void FloatImageObj::calcBBoxSizeWithChildren()
{
    //TODO abstract in linkablemapobj.h
}

