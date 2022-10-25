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
    absPos = QPointF(0, 0);
    visible = true;
}

void MapObj::copy(MapObj *other)
{
    absPos = other->absPos;
    bbox.setX(other->bbox.x());
    bbox.setY(other->bbox.y());
    bbox.setSize(QSizeF(other->bbox.width(), other->bbox.height()));
}

qreal MapObj::x() { return getAbsPos().x(); }

qreal MapObj::y() { return getAbsPos().y(); }

qreal MapObj::width() { return bbox.width(); }

qreal MapObj::height() { return bbox.height(); }

QPointF MapObj::getAbsPos() { return absPos; }

QString MapObj::getPos() { return qpointFToString(absPos); }

QRectF MapObj::boundingRect() const { return QRectF(); }

void MapObj::paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *) {}

bool MapObj::isVisibleObj() { return visible; }

void MapObj::setVisibility(bool v) { visible = v; }
