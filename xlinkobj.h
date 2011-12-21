#ifndef XLINKOBJ_H
#define XLINKOBJ_H

#include <QPen>
#include "mapobj.h"
#include "xlink.h"

class BranchObj;
class BranchItem;

/*! \brief xlinks are used to draw arbitrary connections between branches (BranchObj) in the map. */

#define clickBorder 8

/////////////////////////////////////////////////////////////////////////////
class XLinkObj:public MapObj {
public:
    XLinkObj (QGraphicsItem*, Link* l );
    virtual ~XLinkObj ();
    virtual void init ();
    virtual void setEnd (QPointF);
    void updateXLink();
    void positionBBox();
    void calcBBoxSize();
    void setVisibility (bool);
    void setVisibility ();

private:
    static int arrowSize;
    QPen pen;
    QGraphicsLineItem *line;
    QGraphicsPolygonItem *poly;
    QPointF beginPos;
    QPointF   endPos;

    BranchItem *visBranch;  // the "visible" part of a partially scrolled li
    Link *link;
};

#endif
