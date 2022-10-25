#ifndef MAPOBJ_H
#define MAPOBJ_H

#include <QGraphicsItem>

#include "xmlobj.h"

#define dZ_BBOX 0 // testing
#define dZ_SELBOX 5
#define dZ_FRAME_LOW 10
#define dZ_LINK 20
#define dZ_XLINK 40
#define dZ_FLOATIMG 70
#define dZ_ICON 80
#define dZ_TEXT 90
#define dZ_DEPTH 100
#define Z_INIT 9999
#define Z_LINEEDIT 10000

class ConvexPolygon;

#include "treeitem.h"

/*! \brief Base class for all objects visible on a map
 */

class MapObj : public QGraphicsItem {
  public:
    MapObj(QGraphicsItem *parent = nullptr, TreeItem *ti = nullptr);
    virtual ~MapObj();
    virtual void init();
    virtual void copy(MapObj *);

    virtual qreal x();
    virtual qreal y();
    virtual qreal width();
    virtual qreal height();
    virtual QPointF getAbsPos();
    virtual QString getPos();              //! Return position as string (x,y)

    virtual QRectF boundingRect() const;
    virtual void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);

    virtual bool isVisibleObj();
    virtual void setVisibility(bool);

  protected:
    QRectF bbox;         // bounding box of MO itself
    QPolygonF clickPoly; // area where mouseclicks are found
    QPointF absPos;      // Position on canvas      // FIXME-1 Obsolete. Use pos() instead.
    bool visible;
};

#endif
