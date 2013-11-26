#ifndef ARROWOBJ_H
#define ARROWOBJ_H

#include "mapobj.h"


/*! \brief arrows are used to indicate partially hidden ends of xlinks or also the ends of xlinks. */

/////////////////////////////////////////////////////////////////////////////
class ArrowObj:public MapObj {
public:
    enum LengthStyle {ZeroLength, FixedLength, FullLength};
    enum OrnamentStyle {None, HeadFull, HeadOutline, Foot};
    ArrowObj (MapObj* parent);
    virtual ~ArrowObj ();
    virtual void init ();
    void setVisibility (bool b);
    void setEndPoint (QPointF p);
    QPointF getEndPoint ();
    void setLengthStyle(LengthStyle l);
    LengthStyle getLengthStyle();
    void setOrnamentStyleBegin (OrnamentStyle os);
    OrnamentStyle getOrnamentStyleBegin ();
    void setOrnamentStyleEnd (OrnamentStyle os);
    OrnamentStyle getOrnamentStyleEnd ();

private:
    QGraphicsPolygonItem *polyBegin;	    
    QGraphicsPolygonItem *polyEnd;	    
    QGraphicsLineItem *line;
    QPointF endPoint;

    LengthStyle lengthStyle;
    OrnamentStyle beginStyle;
    OrnamentStyle endStyle;

};

#endif
