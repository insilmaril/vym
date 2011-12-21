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
    name="undefined";

    icon=new ImageObj (this);
    icon->setPos (absPos.x(), absPos.y() );
    state=false;
    avis=true;
}

void FlagObj::copy (FlagObj* other)
{
    MapObj::copy(other);
    name=other->name;
    state=other->state;
    avis=other->avis;
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
    if (v && state)
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
    name=n;
}

const QString FlagObj::getName()
{
    return name;
}

void FlagObj::setAlwaysVisible(bool b)
{
    avis=b;
}

bool FlagObj::isAlwaysVisible()
{
    return avis;
}

bool FlagObj::isActive()
{
    return state;
}

void FlagObj::toggle()
{
    if (state)
	deactivate();
    else
	activate();
}

void FlagObj::activate()
{
    state=true;
    // only show icon, if flag itself is visible 
    if (visible) 
    {
	icon->setVisibility (true);
	calcBBoxSize();
    }	
}

void FlagObj::deactivate()
{
    state=false;
    // if flag itself is invisible we don't need to call 
    if (visible) 
    {
	icon->setVisibility (false);
	calcBBoxSize();
    }	
}

void FlagObj::saveToDir (const QString &tmpdir, const QString &prefix)
{
    QString fn=tmpdir + prefix + name + ".png";
    icon->save (fn,"PNG");
}

void FlagObj::positionBBox()
{
    bbox.moveTopLeft (absPos );
    clickPoly=QPolygonF (bbox);
}

void FlagObj::calcBBoxSize()
{
    if (visible && state)
	bbox.setSize (	QSizeF(
	    icon->boundingRect().width(), 
	    icon->boundingRect().height() ) );
    else
	bbox.setSize (QSizeF(0,0));
    clickPoly= QPolygonF (bbox); 
}

