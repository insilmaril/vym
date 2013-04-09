#ifndef FLOATIMAGEOBJ_H
#define FLOATIMAGEOBJ_H

#include "floatobj.h"
#include <QPixmap>

class TreeItem;
/*! \brief A pixmap which can be positioned freely on the map.  */


/////////////////////////////////////////////////////////////////////////////
class FloatImageObj:public FloatObj {
public:
    FloatImageObj (QGraphicsItem*,TreeItem *ti=NULL);
    ~FloatImageObj ();
    virtual void copy (FloatImageObj*);
    virtual void setZValue (const int&);
    virtual int z();

    virtual void load (const QImage &);
    virtual void setParObj (QGraphicsItem*);
    virtual void setVisibility(bool);	    // set vis. for w
    virtual void moveCenter (double x,double y);
    virtual void move (double x,double y);
    virtual void move (QPointF);
    virtual void positionBBox();
    virtual void calcBBoxSize();
    virtual QRectF getBBoxSizeWithChildren();	// return size of BBox including children  
    virtual void calcBBoxSizeWithChildren();	// calc size of  BBox including children recursivly

protected:
    ImageObj *icon;
};

#endif
