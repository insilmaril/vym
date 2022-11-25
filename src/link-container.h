#ifndef LINK_CONTAINER_H
#define LINK_CONTAINER_H

#include <QPen>

#include "container.h"

/*! \brief This container class provides links and is used in BranchContainer and ImageContainer

The links are connecting the branch and image containers in a map.
*/

class LinkContainer : public Container {
  public:
    /*! Various drawing styles for links */
    enum Style {
        NoLink,         //!< No visible link
        Line,           //!< Straight line
        PolyLine,       //!< Polygon (thick line)
        Parabel,        //!< Parabel
        PolyParabel,    //!< Thick parabel
        Undefined
    };

    /*! Hint if link should use the default link color or the color of heading
     */
    enum ColorHint {
        DefaultColor, //!< Link uses the default color
        HeadingColor  //!< Link uses the color of heading
    };

    LinkContainer();
    virtual ~LinkContainer();

  protected:
    void init();

  public:
    void createBottomLine();
    void deleteBottomLine();
    bool hasBottomLine();
    void delLink();
    void copy(LinkContainer *);

    void setLinkStyle(Style);
    Style getLinkStyle();
    static Style styleFromString(const QString &);
    static QString styleString(const Style &);

    void setLinkColorHint(ColorHint);
    ColorHint getLinkColorHint();

    void setLinkColor(QColor);
    QColor getLinkColor();
    virtual void setVisibility(bool);
    void updateVisibility(); //! hides/unhides link depending on selection

    /*! update parPos, childRefPos depending on pos redraw link with given style */
    void updateLinkGeometry();

    void setUpLinkPosParent(const QPointF&);  // Upwards link pos, parents end (local coord)
    void setUpLinkPosSelf(const QPointF&);    // Upwards own link pos (local coord)
    void setDownLinkPos(const QPointF&);      // Upwards own link pos (local coord)

    void reposition();

  protected:
    void parabel(QPolygonF &, qreal, qreal, qreal, qreal); // Create Parabel connecting two points

    QPointF upLinkPosParent;
    QPointF upLinkPosSelf;
    QPointF downLinkPos;

    int thickness_start; // for StylePoly*
    Style style;         // Current style
    QColor linkcolor;    // Link color
    ColorHint colorHint;
    QPen pen;
    QGraphicsLineItem *l;               // line style
    QGraphicsPolygonItem *p;            // poly styles
    int arcsegs;                        // arc: number of segments
    QList<QGraphicsLineItem *> segments;// a part of e.g. the parabel
    QPolygonF pa0; // For drawing of PolyParabel and PolyLine
    QPolygonF pa1; // For drawing of PolyParabel
    QPolygonF pa2; // For drawing of PolyParabel

    QGraphicsLineItem *bottomLine; // on bottom of BBox
};
#endif
