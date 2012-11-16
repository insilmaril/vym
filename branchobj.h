#ifndef BRANCHOBJ_H
#define BRANCHOBJ_H

#include "floatimageobj.h"
#include "linkablemapobj.h"
#include "ornamentedobj.h"
#include "xlinkobj.h"

/*! \brief A branch visible in the map */

/////////////////////////////////////////////////////////////////////////////
class BranchObj:public OrnamentedObj {
public:
    /*! New branches will get use same color for heading as parent */
    enum BranchModification {NewBranch, MovedBranch};

    BranchObj (QGraphicsItem *parent=NULL,TreeItem *ti=NULL);
    ~BranchObj ();
    virtual void init ();
    virtual void copy (BranchObj*);
    void clear();

    virtual void setParObjTmp (LinkableMapObj*,QPointF,int);// Only for moving Obj around
    virtual void unsetParObjTmp();	    // reuse original ParObj

    virtual void setVisibility(bool,int);   // set visibility
    virtual void setVisibility(bool);	    // set vis. for w
    virtual void setLinkColor();	    // set the color of link

    virtual void positionContents();
    virtual void move (double x,double y);
    virtual void move (QPointF);
    virtual void moveBy (double x,double y);
    virtual void moveBy (QPointF);

    virtual void positionBBox();
    virtual void calcBBoxSize();
    virtual void setDockPos();
    
    virtual void updateData();	//! Update represantatio of heading, flags, etc.

public:	
    virtual void setDefAttr (BranchModification, bool keepFrame=false);	// set default attributes (frame, font, size, ...)

    virtual void alignRelativeTo(const QPointF, bool alignSelf=false );
    virtual void reposition();
    virtual void unsetAllRepositionRequests();

    virtual QRectF getTotalBBox();	// return size of BBox including children  
    virtual ConvexPolygon getBoundingPolygon();
    virtual void calcBBoxSizeWithChildren();	// calc size of  BBox including children recursivly

    virtual void setAnimation(const AnimPoint &ap);
    virtual void stopAnimation();
    virtual bool animate();

protected:
    AnimPoint anim;
};


#endif

