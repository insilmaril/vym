#ifndef ARROWOBJ_H
#define ARROWOBJ_H

#include "mapobj.h"

#include <QPen>

/*! \brief arrows are used to indicate partially hidden ends of xlinks and
    also the ends of xlinks. 
*/

/////////////////////////////////////////////////////////////////////////////

class ArrowObj:public MapObj {
public:
    enum OrnamentStyle {None, HeadFull, Foot};
    ArrowObj (MapObj* parent);
    virtual ~ArrowObj ();
    virtual void init ();
    void setPen( QPen pen );
    QPen getPen();
    void setArrowSize(qreal r);
    qreal getArrowSize();
    void setFixedLength(int i);
    int  getFixedLength();
    void setUseFixedLength(bool b);
    bool getUseFixedLength();
    void show();
    void hide();
    void setVisibility (bool b);
    void setEndPoint (QPointF p);
    QPointF getEndPoint ();
    void setStyleBegin (OrnamentStyle os);
    void setStyleBegin (const QString &s);
    OrnamentStyle getStyleBegin ();
    void setStyleEnd (const QString &s);
    void setStyleEnd (OrnamentStyle os);
    OrnamentStyle getStyleEnd ();
    static QString styleToString(const OrnamentStyle &os);

private:
    QPen pen;
    qreal arrowSize;
    int fixedLength;
    bool useFixedLength;
    QGraphicsPolygonItem *arrowEnd;	    
    QGraphicsPolygonItem *arrowBegin;	    
    QGraphicsLineItem *line;
    QPointF endPoint;

    OrnamentStyle styleBegin;
    OrnamentStyle styleEnd;
};

#endif
