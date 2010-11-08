#ifndef FLOATOBJ_H
#define FLOATOBJ_H

#include "ornamentedobj.h"

/*! \brief Base class for objects floating in the map, which means they can be positioned freely. */


/////////////////////////////////////////////////////////////////////////////
class FloatObj:public OrnamentedObj {
public:
    FloatObj (QGraphicsScene*,TreeItem *ti=NULL);
    ~FloatObj ();
    virtual void init ();
    virtual void copy (FloatObj*);
    virtual bool load (const QString&)=0;
    virtual void setZValue(const int&);	    // set zPlane
    virtual int zValue();

    virtual void move (double,double);
    virtual void move (QPointF);
    virtual void setDockPos();
    virtual void reposition();
					    
    virtual QRectF getBBoxSizeWithChildren();	// return size of BBox including children  

protected:
    int zPlane;
};

#endif
