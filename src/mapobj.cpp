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
}

MapObj::~MapObj()
{
    //qDebug() << "Destr MapObj "<<this;
    foreach (QGraphicsItem *i, childItems())
        // Avoid that QGraphicsScene deletes children
        i->setParentItem(nullptr);
}

QRectF MapObj::boundingRect() const { return QRectF(); }

void MapObj::paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *) {}
