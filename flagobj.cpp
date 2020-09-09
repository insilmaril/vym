#include <QDebug>

#include "flagobj.h"

/////////////////////////////////////////////////////////////////
// FlagObj
/////////////////////////////////////////////////////////////////
FlagObj::FlagObj(QGraphicsItem *parent):MapObj(parent) 
{
    //qDebug() << "Const FlagObj  this=" << this;
    init ();
}

FlagObj::~FlagObj()
{
    //qDebug() << "Destr FlagObj  this=" << this << "  " << name;
    if (imageObj) delete (imageObj);
}


void FlagObj::init ()
{
    name = "undefined";

    // FIXME-0 org:   imageObj = new ImageObj (parentItem());
    imageObj = new ImageObj (parentItem() );
    imageObj->setPos (QPointF(absPos.x(), absPos.y()) );
    avis = true;
}

void FlagObj::copy (FlagObj* other)
{
    MapObj::copy(other);
    name  = other->name;
    uid   = other->uid;
    avis  = other->avis;
    imageObj->copy(other->imageObj);
    setVisibility (other->isVisibleObj() );
}

void FlagObj::move(double x, double y)
{
    MapObj::move(x,y);
    imageObj->setPos(QPointF(x,y));
    positionBBox();
}

void FlagObj::moveBy(double x, double y)
{
    move (x+absPos.x(),y+absPos.y() );
}

void FlagObj::setZValue (double z)
{
    imageObj->setZValue (z);
}

void FlagObj::setVisibility (bool v)
{
    MapObj::setVisibility(v);
    if (v) 
	imageObj->setVisibility(true);
    else
	imageObj->setVisibility(false);
}

void FlagObj::load (const QString &fn)
{
    imageObj->load(fn);
    calcBBoxSize();
    positionBBox();
}

void FlagObj::load (ImageObj* io)
{
    imageObj->copy(io);   // Creates deep copy of pixmap or svg!
    calcBBoxSize();
    positionBBox();
}

void FlagObj::setName(const QString &n)
{
    name = n;
}

const QString FlagObj::getName()
{
    return name;
}

void FlagObj::setUuid(const QUuid &id)
{
    uid = id;
}

const QUuid FlagObj::getUuid()
{
    return uid;
}

void FlagObj::setAlwaysVisible(bool b)
{
    avis=b;
}

bool FlagObj::isAlwaysVisible()
{
    return avis;
}

void FlagObj::saveToDir (const QString &tmpdir, const QString &prefix)
{
    QString fn=tmpdir + prefix + name;
    imageObj->save (fn);
}

void FlagObj::positionBBox()
{
    bbox.moveTopLeft (absPos );
    clickPoly=QPolygonF (bbox);
}

void FlagObj::calcBBoxSize()
{
    if (visible)
	bbox.setSize (	QSizeF(
	    imageObj->boundingRect().width(), 
	    imageObj->boundingRect().height() ) );
    else
	bbox.setSize (QSizeF(0, 0));
    clickPoly = QPolygonF (bbox); 
}

