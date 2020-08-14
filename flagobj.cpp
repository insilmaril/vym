#include <QDebug>

#include "flagobj.h"

/////////////////////////////////////////////////////////////////
// FlagObj
/////////////////////////////////////////////////////////////////
FlagObj::FlagObj(QGraphicsItem *parent):MapObj(parent) 
{
//  qDebug() << "Const FlagObj  this="<<this<<"  scene="<<s;
    init ();
}

FlagObj::~FlagObj()
{
//   qDebug() << "Destr FlagObj  this="<<this <<"  " <<name;
    if (icon) delete (icon);
}


void FlagObj::init ()
{
    name = "undefined";

    // FIXME-0 org:   icon = new ImageObj (parentItem());
    icon = new ImageObj (parentItem() );
    icon->setPos (absPos.x(), absPos.y() );
    avis = true;
}

void FlagObj::copy (FlagObj* other)
{
    MapObj::copy(other);
    name  = other->name;
    uid   = other->uid;
    avis  = other->avis;
    icon->copy(other->icon);
    setVisibility (other->isVisibleObj() );
}

void FlagObj::move(double x, double y)
{
    MapObj::move(x,y);
    icon->setPos(x,y);
    positionBBox();
}

void FlagObj::moveBy(double x, double y)
{
    move (x+absPos.x(),y+absPos.y() );
}

void FlagObj::setZValue (double z)
{
    icon->setZValue (z);
}

void FlagObj::setVisibility (bool v)
{
    MapObj::setVisibility(v);
    if (v) 
	icon->setVisibility(true);
    else
	icon->setVisibility(false);
}

void FlagObj::load (const QString &fn)
{
    icon->load(fn);
    calcBBoxSize();
    positionBBox();
}

void FlagObj::load (const QPixmap &pm)
{
    icon->load(pm);
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
    QString fn=tmpdir + prefix + name + ".png";
    icon->save (fn, "PNG");
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
	    icon->boundingRect().width(), 
	    icon->boundingRect().height() ) );
    else
	bbox.setSize (QSizeF(0, 0));
    clickPoly = QPolygonF (bbox); 
}

