#include <QDebug>
#include <QImageReader>

#include "floatimageobj.h"
#include "branchobj.h"


/////////////////////////////////////////////////////////////////
// FloatImageObj 
/////////////////////////////////////////////////////////////////

FloatImageObj::FloatImageObj (QGraphicsItem * parent,TreeItem *ti):FloatObj(parent,ti)
{
    // qDebug() << "Const FloatImageObj this=" << this << "  ti=" << ti;
    imageObj = new ImageObj (parent);
    imageObj->setPos (absPos.x(), absPos.y() );
    imageObj->setVisibility (true);
    clickPoly = bbox;
    useRelPos = true;

//    setLinkStyle (LinkableMapObj::Parabel);
}

FloatImageObj::~FloatImageObj ()
{
    //qDebug() << "Destr FloatImageObj "<<this<<"";
    delete(imageObj);
}

void FloatImageObj::copy (FloatImageObj* other)
{		    
    FloatObj::copy (other);
    imageObj->copy (other->imageObj);
    positionBBox();
}

void FloatImageObj::setZValue (const int &i)
{

//    qDebug()<<"FIO::setZValue z="<<i;
//    qDebug()<<"  imageObj="<<imageObj;
//    qDebug()<<"  this="<<this;	 
    imageObj->setZValue (i);
}

int FloatImageObj::z ()
{
    return qRound (imageObj->zValue());
}

void FloatImageObj::load (const QImage *image)
{
    imageObj->load(QPixmap::fromImage(*image));

    if (!imageObj->parentItem() ) imageObj->setParentItem(this);  // Add to scene initially
    bbox.setSize ( QSizeF(
        imageObj->boundingRect().width(), 
        imageObj->boundingRect().height()));

    clickPoly = bbox;
    positionBBox();
}

void FloatImageObj::setParObj (QGraphicsItem *p)
{
    setParentItem (p);
    imageObj->setParentItem (p);
    parObj = (LinkableMapObj*)p;
/*
    qDebug()<<"FIO::setParentItem";
    qDebug()<<"  this="<<this;
    qDebug()<<"  imageObj="<<imageObj;
*/
}

void FloatImageObj::setVisibility(bool v)
{
    OrnamentedObj::setVisibility(v);
    if (v)
	imageObj->setVisibility(true);
    else
	imageObj->setVisibility(false);
}

void FloatImageObj::moveCenter (double x, double y)
{
    FloatObj::moveCenter(x, y);
    imageObj->setPos(bbox.topLeft() );
}

void FloatImageObj::move (double x, double y)
{
    FloatObj::move(x,y);
    imageObj->setPos (x,y); 
    positionBBox();
}

void FloatImageObj::move (QPointF p)
{
    FloatImageObj::move (p.x(),p.y());
}

void FloatImageObj::positionBBox()
{
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

