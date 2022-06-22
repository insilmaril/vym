#ifndef LINKABLE_CONTAINER_H
#define LINKABLE_CONTAINER_H

#include <QPen>

#include "container.h"

/*! \brief This class adds links to a container

The links are connecting the branch and image containers in a map.
*/

class LinkableContainer : public Container {
  public:
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

    LinkableContainer();
    LinkableContainer(QGraphicsItem *parent);
    virtual ~LinkableContainer();

  protected:
    virtual void init();
    virtual void createBottomLine();

  public:
    virtual void delLink();
    virtual void copy(LinkableContainer *);

    void setLinkStyle(Style);
    Style getLinkStyle();

    void setLinkPos(Position);
    Position getLinkPos();

    virtual void setLinkColor(QColor);
    QColor getLinkColor();
    virtual void setVisibility(bool);
    virtual void updateVisibility(); //! hides/unhides link depending on selection

    /*! update parPos, childRefPos depending on pos redraw link with given style */
    virtual void updateLinkGeometry();

    virtual void setDockPos() = 0; // sets childRefPos and parPos
    QPointF getChildRefPos();      // returns pos where children dock
    QPointF getFloatRefPos();      // returns pos where floats dock
    QPointF getParPos();           // returns pos where parents dock

  protected:
    void parabel(QPolygonF &, qreal, qreal, qreal, qreal); // Create Parabel connecting two points

    QPointF childRefPos;
    QPointF floatRefPos;
    QPointF parPos;
    bool link2ParPos; // While moving around, sometimes link to parent
    LinkableContainer *parentContainer;    // FIXME-0 needed?

    qreal linkwidth;  // width of a link

    int thickness_start; // for StylePoly*
    Style style;         // Current style
    Position linkpos;    // Link at bottom of object or middle of height
    QColor linkcolor;    // Link color
    QPen pen;
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
};
#endif
