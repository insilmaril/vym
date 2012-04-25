#ifndef XLINKOBJ_H
#define XLINKOBJ_H

#include <QPen>
#include "mapobj.h"
#include "xlink.h"

class BranchObj;
class BranchItem;

/*! \brief xlinks are used to draw arbitrary connections between branches (BranchObj) in the map. */

/////////////////////////////////////////////////////////////////////////////
class XLinkObj:public MapObj {
public:
    enum CurrentSelection {Unselected, Path, C1, C2};
    XLinkObj (QGraphicsItem*, Link* l );
    virtual ~XLinkObj ();
    virtual void init ();
    virtual QPointF getAbsPos();
    virtual void move (QPointF p);
    virtual void setEnd (QPointF);
    void setSelection (CurrentSelection s);
    void updateXLink();
    void positionBBox();
    void calcBBoxSize();
    void setVisibility (bool);
    void setVisibility ();
    void setC1 (const QPointF &p);
    QPointF getC1();
    void setC2 (const QPointF &p);
    QPointF getC2();
    bool isInClickBox (const QPointF &p);
    QPainterPath getClickPath();

private:
    static int arrowSize;
    static int clickBorder;
    static int pointRadius;
    QPainterPath clickPath;
    QGraphicsPolygonItem *poly;	    // Arrowhead, when one end is not visible
    QGraphicsPathItem *path;
    
    QPointF beginPos;
    QPointF   endPos;
    QPointF c1,c2;		    // Controlpoints for bezier curve
    QGraphicsEllipseItem *ctrl_p1;
    QGraphicsEllipseItem *ctrl_p2;
    QGraphicsLineItem *ctrl_l1;
    QGraphicsLineItem *ctrl_l2;

    CurrentSelection curSelection;

    BranchItem *visBranch;  // the "visible" part of a partially scrolled li
    Link *link;
};

#endif
