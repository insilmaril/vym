#ifndef MAPOBJ_H
#define MAPOBJ_H

#include <QGraphicsItem>

#include "xmlobj.h"

/*! \brief Base class for all objects visible on a map
 */

class MapObj : public QGraphicsItem {
  public:
    MapObj(QGraphicsItem *parent = nullptr);
    virtual ~MapObj();
    virtual void init();

    QRectF boundingRect() const;
    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);

    virtual bool isVisibleObj();
    virtual void setVisibility(bool);

  protected:
    bool visible;
};

#endif
