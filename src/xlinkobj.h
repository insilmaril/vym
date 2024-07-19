#ifndef XLINKOBJ_H
#define XLINKOBJ_H

#include <QPen>

#include "arrowobj.h"
#include "branch-container.h"
#include "mapobj.h"

class BranchObj;
class BranchItem;
class XLink;

/*! \brief xlinks are used to draw arbitrary connections between branches
 * (BranchObj) in the map. */

/////////////////////////////////////////////////////////////////////////////
class XLinkObj : public MapObj {
  public:
    enum SelectionType { Empty, Path, C0, C1 };
    XLinkObj(XLink*);
    XLinkObj(QGraphicsItem *, XLink *l);
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
    virtual void setEnd(QPointF);
    void setSelectionType(SelectionType s);
    void updateGeometry();
    void updateVisibility();
    void setVisibility(bool);
    void initC0();
    void setC0(const QPointF &p);
    QPointF getC0();
    void initC1();
    void setC1(const QPointF &p);
    QPointF getC1();
    void setSelectedCtrlPoint(const QPointF &);
    QRectF boundingRect();

    SelectionType couldSelect(const QPointF &);
    void select(const QPen &pen, const QBrush &brush);
    void unselect();

  private:
    enum StateVis { Hidden, OnlyBegin, OnlyEnd, Full, FullShowControls };
    StateVis stateVis;
    static int arrowSize;
    static int clickBorder;
    static int pointRadius;
    static int d_control;
    QPainterPath clickPath;
    QGraphicsPolygonItem *poly; // Arrowhead, when one end is not visible
    ArrowObj *beginArrow;     // Arrowhead
    ArrowObj *endArrow;       // Arrowhead
    QGraphicsPathItem *path;

    QPointF beginPos;
    QPointF endPos;
    BranchContainer::Orientation beginOrient;
    BranchContainer::Orientation endOrient;

    // Controlpoints for Bezier path
    QPointF c0, c1;
    QGraphicsEllipseItem *c0_ellipse;
    QGraphicsEllipseItem *c1_ellipse;
    QGraphicsEllipseItem *selection_ellipse;

    SelectionType selectionTypeInt;

    BranchItem *visBranch; // the "visible" part of a partially scrolled li
    XLink *xlink;
};

#endif
