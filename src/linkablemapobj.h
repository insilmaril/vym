#ifndef LINKABLEMAPOBJ_H
#define LINKABLEMAPOBJ_H

#define MAX_DEPTH 999

#include "mapobj.h"

class LinkableMapObj : public MapObj {
  public:
    /*! Orientation of an object depends on the position relative to the parent
     */
    enum Orientation {
        UndefinedOrientation, //!< Undefined
        LeftOfCenter,         //!< Object is left of center
        RightOfCenter         //!< Object is right of center
    };

    /*! Various drawing styles for links */
    enum Style {
        UndefinedStyle, //!< Undefined
        Line,           //!< Straight line
        Parabel,        //!< Parabel
        PolyLine,       //!< Polygon (thick line)
        PolyParabel     //!< Thick parabel
    };

    /*! Vertical position of link in object */
    enum Position {
        Middle, //!< Link is drawn in the middle of object
        Bottom  //!< Link is drawn at bottom of object
    };

    /*! Hint if link should use the default link color or the color of heading
     */
    enum ColorHint {
        DefaultColor, //!< Link uses the default color
        HeadingColor  //!< Link uses the color of heading
    };

    LinkableMapObj();
    LinkableMapObj(QGraphicsItem *, TreeItem *ti = NULL);
    virtual ~LinkableMapObj();

  protected:
    QPointF childRefPos;
    QPointF floatRefPos;
    QPointF parPos;
    bool link2ParPos; // While moving around, sometimes link to parent

    Orientation orientation;
    qreal linkwidth;  // width of a link
    QRectF bboxTotal; // bounding box including children

    LinkableMapObj *parObj;
    LinkableMapObj *parObjTmpBuf; // temporary buffer the original parent
    bool tmpParent;

    int thickness_start; // for StylePoly*
    Style style;         // Current style
    Position linkpos;    // Link at bottom of object or middle of height
    QGraphicsLineItem *l;               // line style
    QGraphicsPolygonItem *p;            // poly styles
    int arcsegs;                        // arc: number of segments
    QList<QGraphicsLineItem *> segment; // a part of e.g. the parabel
    QPolygonF pa0; // For drawing of PolyParabel and PolyLine
    QPolygonF pa1; // For drawing of PolyParabel
    QPolygonF pa2; // For drawing of PolyParabel

    QGraphicsLineItem *bottomline; // on bottom of BBox
    bool useBottomline;            //! Hint if bottomline should be used
    qreal bottomlineY;             // vertical offset of dockpos to pos

    bool repositionRequest; //

    qreal topPad, botPad, leftPad, rightPad; // padding within bbox

    QPointF relPos; // position relative to childRefPos of parent
    bool useRelPos;
};
#endif
