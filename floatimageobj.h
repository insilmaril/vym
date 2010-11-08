#ifndef FLOATIMAGEOBJ_H
#define FLOATIMAGEOBJ_H

#include "floatobj.h"
//Added by qt3to4:
#include <QPixmap>

class TreeItem;
/*! \brief A pixmap which can be positioned freely as FlagObj  on the map.  */


/////////////////////////////////////////////////////////////////////////////
class FloatImageObj:public FloatObj {
public:
    FloatImageObj (QGraphicsScene*,TreeItem *ti=NULL);
    ~FloatImageObj ();
    virtual void init ();
    virtual void copy (FloatImageObj*);
    virtual void setZValue (const int&);
    virtual int z();

    virtual void load (const QPixmap &);
    virtual bool load (const QString &);
    virtual void save (const QString &, const QString&);
    virtual void setOriginalFilename(const QString &);
    virtual QString getOriginalFilename();
    virtual void setVisibility(bool);	    // set vis. for w
    virtual void move (double x,double y);
    virtual void move (QPointF);
    virtual void positionBBox();
    virtual void calcBBoxSize();
    virtual QRectF getBBoxSizeWithChildren();	// return size of BBox including children  
    virtual void calcBBoxSizeWithChildren();	// calc size of  BBox including children recursivly

protected:
    ImageObj *icon;
    QString filetype;
    QString filename;
    QString originalFilename;
};

#endif
