#ifndef LINKOBJ_H
#define LINKOBJ_H

#include "mapobj.h"

#include <QPen>

/*! \brief This class provides links to draw linkx from ImageContainers and OrnamentsContainers to their parent BranchContainers
*/

class QGraphicsLineItem;
class QGraphicsPolygonItem;

class LinkObj : public MapObj {
  public:
    /*! Various drawing styles for links */
    enum Style {
        NoLink,         //!< No visible link
        Line,           //!< Straight line
        PolyLine,       //!< Polygon (thick line)
        Parabel,        //!< Parabel
        PolyParabel,    //!< Thick parabel
        ListDash,       //!< Item list, no real links drawn
        Undefined
    };

    /*! Hint if link should use the default link color or the color of heading
     */
    enum ColorHint {
        DefaultColor, //!< Link uses the default color
        HeadingColor  //!< Link uses the color of heading
    };

    LinkObj(QGraphicsItem *parent = nullptr);
    virtual ~LinkObj();

  protected:
    void init();

  public:
    void createBottomLine();
    void deleteBottomLine();
    bool hasBottomLine();
    void delLink();
    void copy(LinkObj *);

    void setLinkStyle(Style);
    Style getLinkStyle();
    static Style styleFromString(const QString &);
    static QString styleString(int);

    void setLinkColorHint(ColorHint);
    ColorHint getLinkColorHint();

    void setLinkColor(QColor);
    QColor getLinkColor();

    /*! update parPos, childRefPos depending on pos redraw link with given style */
    void updateLinkGeometry();

    void setUpLinkPosParent(const QPointF&);  // Upwards link pos, parents end (local coord)
    void setUpLinkPosSelf(const QPointF&);    // Upwards own link pos (local coord)
    void setDownLinkPos(const QPointF&);      // Upwards own link pos (local coord)
    void setBulletPointPos(const QPointF&);   // Center of bullet point, if List* is used (local coord)

    void reposition();

  protected:
    void parabel(QPolygonF &, qreal, qreal, qreal, qreal); // Create Parabel connecting two points

    QPointF upLinkPosParent;
    QPointF upLinkPosSelf;
    QPointF downLinkPos;
    QPointF bulletPointPos;

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
