#include <QDebug>

#include "geometry.h"
#include "mapobj.h"
#include "misc.h"

/////////////////////////////////////////////////////////////////
// MapObj
/////////////////////////////////////////////////////////////////
MapObj::MapObj(QGraphicsItem *parent, TreeItem *ti) : QGraphicsItem(parent)
{
    //qDebug() << "Const MapObj (this,ti)=(" << this << "," << ti << ")";
    init();
}

MapObj::~MapObj()
{
    //qDebug() << "Destr MapObj "<<this;
    foreach (QGraphicsItem *i, childItems())
        // Avoid that QGraphicsScene deletes children
        i->setParentItem(nullptr);
}

void MapObj::init()
{
    visible = true;
}

QRectF MapObj::boundingRect() const { return QRectF(); }

void MapObj::paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *) {}

QString MapObj::getPosString() { return qpointFToString(pos()); }

bool MapObj::isVisibleObj() { return visible; }

void MapObj::setVisibility(bool v) { visible = v; }
