#ifndef XLINKOBJ_H
#define XLINKOBJ_H

#include <QPen>
#include "linkablemapobj.h"
#include "mapobj.h"
#include "xlink.h"

class BranchObj;
class BranchItem;

/*! \brief xlinks are used to draw arbitrary connections between branches (BranchObj) in the map. */

/////////////////////////////////////////////////////////////////////////////
class XLinkObj:public MapObj {
public:
    enum CurrentSelection {Unselected, Path, C0, C1};
    XLinkObj (QGraphicsItem*, Link* l );
    virtual ~XLinkObj ();
    virtual void init ();
    virtual QPointF getAbsPos();
    QPointF getBeginPos();
    QPointF getEndPos();
    virtual void move (QPointF p);
    virtual void setEnd (QPointF);
    void setSelection (int cp);
    void setSelection (CurrentSelection s);
    void updateXLink();
    void positionBBox();
    void calcBBoxSize();
    void setVisibility (bool);
    void setVisibility ();
    void setC0 (const QPointF &p);
    QPointF getC0();
    void setC1 (const QPointF &p);
    QPointF getC1();
    bool isInClickBox (const QPointF &p);
    int ctrlPointInClickBox (const QPointF &p);
    QPainterPath getClickPath();

private:
    enum StateVis {Hidden,Part,Full,FullShowControls};
    StateVis stateVis;
    static int arrowSize;
    static int clickBorder;
    static int pointRadius;
    QPainterPath clickPath;
    QGraphicsPolygonItem *poly;	    // Arrowhead, when one end is not visible
    QGraphicsPathItem *path;
    
    QPointF beginPos;
    QPointF   endPos;
    QPointF c0,c1;		    // Controlpoints for Bezier path
    LinkableMapObj::Orientation beginOrient;
    LinkableMapObj::Orientation   endOrient;
    QGraphicsEllipseItem *ctrl_p0;
    QGraphicsEllipseItem *ctrl_p1;
    QGraphicsLineItem *ctrl_l0;
    QGraphicsLineItem *ctrl_l1;

    CurrentSelection curSelection;

    BranchItem *visBranch;  // the "visible" part of a partially scrolled li
    Link *link;
};

#endif
