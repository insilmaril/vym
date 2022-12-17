#ifndef MAPOBJ_H
#define MAPOBJ_H

#include <QGraphicsItem>

#include "xmlobj.h"

//#define dZ_BBOX 0 // FIXME-1 remove all global z-values, no longer needed
//#define dZ_SELBOX 5
//#define dZ_FRAME_LOW 10
//#define dZ_LINK 20
//#define dZ_XLINK 40
//#define dZ_FLOATIMG 70
//#define dZ_ICON 80
//#define dZ_TEXT 90
//#define dZ_DEPTH 100
#define Z_SNOW 2000
//#define Z_INIT 9999
#define Z_LINEEDIT 10000

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
