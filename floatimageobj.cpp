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

bool FloatImageObj::load (const QImage *image)
{
    if (!imageObj->load(QPixmap::fromImage(*image)) ) return false;

    if (!imageObj->parentItem() ) imageObj->setParentItem(this);  // Add to scene initially
    bbox.setSize ( QSizeF(
        imageObj->boundingRect().width(), 
        imageObj->boundingRect().height()));

    clickPoly = bbox;
    positionBBox();
}

bool FloatImageObj::load (const QString &fname) // FIXME-0 testing, add filename or svg as parameter
{
    if (!imageObj->load(fname) ) return false;

    bbox.setSize ( QSizeF(
        imageObj->boundingRect().width(), 
        imageObj->boundingRect().height()));

    clickPoly = bbox;
    positionBBox();
    return true;
}

bool FloatImageObj::save (const QString &fname) 
{
    return imageObj->save(fname); 
}

QString FloatImageObj::getExtension()
{
    return imageObj->getExtension();
}

void FloatImageObj::setParObj (QGraphicsItem *p)
{
    setParentItem (p);
    imageObj->setParentItem (p);
    parObj = (LinkableMapObj*)p;
/*
    qDebug()<<"FIO::setParentItem";
    qDebug()<<"     this = "<<this;
    qDebug()<<"  imageObj=" << imageObj;
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

void  FloatImageObj::setScaleFactor(qreal f) 
{
    imageObj->setScaleFactor(f);
    bbox.setSize ( QSizeF(
        imageObj->boundingRect().width(), 
        imageObj->boundingRect().height()));
    positionBBox();
}

qreal  FloatImageObj::getScaleFactor()
{
    return imageObj->getScaleFactor();
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
    clickPoly = QPolygonF(bbox);
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

