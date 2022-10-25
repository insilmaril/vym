#ifndef XLINKOBJ_H
#define XLINKOBJ_H

#include <QPen>

#include "arrowobj.h"
#include "branch-container.h"
#include "mapobj.h"
#include "xlink.h"

class BranchObj;
class BranchItem;

/*! \brief xlinks are used to draw arbitrary connections between branches
 * (BranchObj) in the map. */

/////////////////////////////////////////////////////////////////////////////
class XLinkObj : public MapObj {
  public:
    enum SelectionType { Empty, Path, C0, C1 };
    XLinkObj(Link*);
    XLinkObj(QGraphicsItem *, Link *l);
    virtual ~XLinkObj();
    virtual void init();
    virtual QPointF getAbsPos();
    void setStyleBegin(const QString &s);
    void setStyleBegin(ArrowObj::OrnamentStyle os);
    ArrowObj::OrnamentStyle getStyleBegin();
    void setStyleEnd(const QString &s);
    void setStyleEnd(ArrowObj::OrnamentStyle os);
    ArrowObj::OrnamentStyle getStyleEnd();
    QPointF getBeginPos();
    QPointF getEndPos();
    virtual void move(QPointF p);
    virtual void setEnd(QPointF);
    void setSelection(int cp);
    void setSelection(SelectionType s);
    void updateXLink();
    void setVisibility(bool);
    void setVisibility();
    void initC0();
    void setC0(const QPointF &p);
    QPointF getC0();
    void initC1();
    void setC1(const QPointF &p);
    QPointF getC1();
    void setSelectedCtrlPoint(const QPointF &);
    QPointF getSelectedCtrlPoint();

    int ctrlPointInClickBox(const QPointF &p);
    SelectionType couldSelect(const QPointF &);
    bool isInClickBox(const QPointF &p);
    QPainterPath getClickPath();

  private:
    enum StateVis { Hidden, OnlyBegin, OnlyEnd, Full, FullShowControls };
    StateVis stateVis;
    static int arrowSize;
    static int clickBorder;
    static int pointRadius;
    static int d_control;
    QPainterPath clickPath;
    QGraphicsPolygonItem *poly; // Arrowhead, when one end is not visible
    ArrowObj *pointerBegin;     // Arrowhead
    ArrowObj *pointerEnd;       // Arrowhead
    QGraphicsPathItem *path;

    QPointF beginPos;
    QPointF endPos;
    BranchContainer::Orientation beginOrient;
    BranchContainer::Orientation endOrient;

    // Controlpoints for Bezier path
    QPointF c0, c1;
    QGraphicsEllipseItem *ctrl_p0;
    QGraphicsEllipseItem *ctrl_p1;

    SelectionType curSelection;

    BranchItem *visBranch; // the "visible" part of a partially scrolled li
    Link *link;
};

#endif
