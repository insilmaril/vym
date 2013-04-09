#ifndef LINKABLEMAPOBJ_H
#define LINKABLEMAPOBJ_H

#include "animpoint.h"
#include "noteobj.h"
#include "headingobj.h"
#include "flagrowobj.h"

#define MAX_DEPTH 999

class VymModel;
class TreeItem;

/*! \brief This class adds links to MapObj 

The links are connecting the branches (BranchObj) and images (FloatImageObj) in the map.
*/

class LinkableMapObj:public MapObj {
public:
    /*! Orientation of an object depends on the position relative to the parent */
    enum Orientation {
	UndefinedOrientation, //!< Undefined
	LeftOfCenter,		//!< Object is left of center
	RightOfCenter		//!< Object is right of center
    };

    /*! Various drawing styles for links */
    enum Style {
	UndefinedStyle,	//!< Undefined
	Line,		//!< Straight line
	Parabel,	//!< Parabel
	PolyLine,	//!< Polygon (thick line)
	PolyParabel	//!< Thick parabel
    };

    /*! Vertical position of link in object */
    enum Position {
	Middle, //!< Link is drawn in the middle of object
	Bottom  //!< Link is drawn at bottom of object
    };


    /*! Hint if link should use the default link color or the color of heading */
    enum ColorHint {
	DefaultColor,	//!< Link uses the default color
	HeadingColor	//!< Link uses the color of heading
    };

    LinkableMapObj ();
    LinkableMapObj (QGraphicsItem*, TreeItem *ti=NULL);
    virtual ~LinkableMapObj ();
protected:
    virtual void init ();
    virtual void createBottomLine();
public:
    virtual void delLink();
    virtual void copy (LinkableMapObj*);

    void setChildObj (LinkableMapObj*);
    virtual void setParObj (LinkableMapObj*);
    virtual void setParObjTmp (LinkableMapObj*,QPointF,int);	// Only for moving Obj around
    virtual void unsetParObjTmp();			// reuse original ParObj
    virtual bool hasParObjTmp();

    virtual void setUseRelPos (const bool&);
    virtual bool getUseRelPos();
    virtual void setRelPos();		    // set relPos to current parentPos
    virtual void setRelPos(const QPointF&); 
    virtual QPointF getRelPos();

    virtual qreal getTopPad();
    virtual qreal getLeftPad();
    virtual qreal getRightPad();
    Style getDefLinkStyle(TreeItem *parent);
    void setLinkStyle(Style);            
    Style getLinkStyle();

    void setLinkPos (Position);
    Position getLinkPos ();

    virtual void setLinkColor();	    // sets color according to colorhint, overloaded
    virtual void setLinkColor(QColor);
    QColor getLinkColor();
    virtual void setVisibility (bool);
    virtual void setOrientation();
    virtual void updateVisibility();	    //! hides/unhides link depending on selection

    /*! update parPos, childRefPos 
	depending on pos
	redraw link with given style */
    virtual void updateLinkGeometry();	    

    virtual void setDockPos()=0;	    // sets childRefPos and parPos
    QPointF getChildRefPos();		    // returns pos where children dock
    QPointF getParPos();                    // returns pos where parents dock
    Orientation getOrientation();	    // get orientation

    virtual void reposition();
    virtual void requestReposition();	    // do reposition after next user event
    virtual void forceReposition();	    // to force a reposition now (outside
					    // of mapeditor e.g. in noteeditor
    virtual bool repositionRequested();

    virtual void calcBBoxSizeWithChildren()=0;// calc size of  BBox including children recursivly

protected:
    void parabel(QPolygonF &,double,double,double,double);  // Create Parabel connecting two points

    QPointF childRefPos;
    QPointF floatRefPos;
    QPointF parPos;
    bool link2ParPos;		    // While moving around, sometimes link to parent

    Orientation orientation;     
    qreal linkwidth;		    // width of a link
    QRectF bboxTotal;		    // bounding box including children

    LinkableMapObj* parObj;	
    LinkableMapObj* parObjTmpBuf;   // temporary buffer the original parent
    bool tmpParent;

    int thickness_start;	    // for StylePoly*	
    Style style;		    // Current style
    Position linkpos;		    // Link at bottom of object or middle of height
    QColor linkcolor;               // Link color
    QPen pen;
    QGraphicsLineItem* l;           // line style
    QGraphicsPolygonItem* p;	    // poly styles
    int arcsegs;                    // arc: number of segments
    QList <QGraphicsLineItem*> segment; // a part of e.g. the parabel
    QPolygonF pa0;		    // For drawing of PolyParabel and PolyLine
    QPolygonF pa1;		    // For drawing of PolyParabel 
    QPolygonF pa2;		    // For drawing of PolyParabel   

    QGraphicsLineItem* bottomline;  // on bottom of BBox
    bool useBottomline;		    //! Hint if bottomline should be used
    qreal bottomlineY;              // vertical offset of dockpos to pos

    bool repositionRequest;	    // 

    qreal topPad, botPad,
	leftPad, rightPad;          // padding within bbox

    QPointF  relPos;		    // position relative to childRefPos of parent
    bool useRelPos;

};
#endif
