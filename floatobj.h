#ifndef FLOATOBJ_H
#define FLOATOBJ_H

#include "ornamentedobj.h"

/*! \brief Base class for objects floating in the map, which means they can be
 * positioned freely. */

/////////////////////////////////////////////////////////////////////////////
class FloatObj : public OrnamentedObj {
  public:
    FloatObj(QGraphicsItem *, TreeItem *ti = NULL);
    ~FloatObj();
    virtual void init();
    virtual void copy(FloatObj *);
    virtual void move(double, double);
    virtual void move(QPointF);
    virtual void moveCenter(double x, double y);
    virtual void moveCenter2RelPos(double x, double y);
    virtual void move2RelPos(double x, double y);
    virtual void move2RelPos(QPointF p);
    virtual void setRelPos();
    virtual void setRelPos(const QPointF &p);
    virtual void setDockPos();
    virtual void reposition();

    virtual QRectF
    getBBoxSizeWithChildren(); // return size of BBox including children
};

#endif
