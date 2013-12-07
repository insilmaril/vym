#ifndef ARROWOBJ_H
#define ARROWOBJ_H

#include "mapobj.h"


/*! \brief arrows are used to indicate partially hidden ends of xlinks and
    also the ends of xlinks. 
*/

/////////////////////////////////////////////////////////////////////////////
class ArrowObj:public MapObj {
public:
    enum OrnamentStyle {None, HeadFull, HeadOutline, Foot};
    ArrowObj (MapObj* parent);
    virtual ~ArrowObj ();
    virtual void init ();
    void setColor (QColor c);
    QColor getColor();
    void setArrowSize(qreal r);
    qreal getArrowSize();
    void setFixedLength(int i);
    int  getFixedLength();
    void setVisibility (bool b);
    void setEndPoint (QPointF p);
    QPointF getEndPoint ();
    void setOrnamentStyleBegin (OrnamentStyle os);
    OrnamentStyle getOrnamentStyleBegin ();
    void setOrnamentStyleEnd (OrnamentStyle os);
    OrnamentStyle getOrnamentStyleEnd ();

private:
    QColor color;
    qreal arrowSize;
    int fixedLength;
    QGraphicsPolygonItem *arrowEnd;	    
    QGraphicsPolygonItem *arrowBegin;	    
    QGraphicsLineItem *line;
    QPointF endPoint;

    OrnamentStyle headStyle;
    OrnamentStyle footStyle;

};

#endif
