#include <QDebug>

#include "geometry.h"
#include "mapobj.h"
#include "misc.h"

/////////////////////////////////////////////////////////////////
// MapObj
/////////////////////////////////////////////////////////////////
MapObj::MapObj(QGraphicsItem *parent) : QGraphicsItem(parent)
{
    //qDebug() << "Const MapObj";
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

bool MapObj::isVisibleObj() { return visible; } // FIXME-2 Currently not used. Remove?

void MapObj::setVisibility(bool v) { visible = v; }
