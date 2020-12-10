#ifndef ORNAMENTEDOBJ_H
#define ORNAMENTEDOBJ_H

#include "frameobj.h"
#include "linkablemapobj.h"

class TreeItem;

/*! \brief Adds various ornaments and data to the class LinkableMapObj

The ornaments are:
    - frame
    - note
    - references
    - flags
    - standard flags
    - system flags
 */
//    - attributes (key/value pairs)  

class OrnamentedObj:public LinkableMapObj {
public:	
    OrnamentedObj (QGraphicsItem* parent, TreeItem *ti=NULL);
    virtual ~OrnamentedObj ();
    virtual void init ();
    virtual void copy (OrnamentedObj*);

    virtual void setLinkColor();	// sets color according to colorhint, overloaded
    virtual void setColor(QColor);	// set the color of text and link
    QColor getColor ();		    // get color of heading
    QRectF getBBoxHeading();

    virtual void setRotation (const qreal &a);
    virtual FrameObj* getFrame();
    virtual FrameObj::FrameType getFrameType ();
    virtual QString getFrameTypeName ();
    virtual void setFrameType (const FrameObj::FrameType &);
    virtual void setFrameType (const QString &);
    virtual void setFramePadding (const int &);
    virtual int  getFramePadding ();
    virtual void setFrameBorderWidth(const int &);
    virtual int  getFrameBorderWidth ();
    virtual void setFramePenColor (QColor);
    virtual QColor getFramePenColor ();
    virtual void setFrameBrushColor (QColor);
    virtual QColor getFrameBrushColor ();
    virtual void setFrameIncludeChildren (bool);
    virtual bool getFrameIncludeChildren ();
    virtual QRectF getOrnamentsBBox();

    virtual void positionContents();
    virtual void move   (double,double);
    virtual void move   (QPointF);
    virtual void moveBy (double,double);
    virtual void moveBy (QPointF);
    virtual void move2RelPos (QPointF);	    // move relativly to parent^
    virtual void move2RelPos (double,double);

    virtual QUuid findSystemFlagUidByPos (const QPointF &p);
    virtual QRectF getBBoxSystemFlagByUid (const QUuid &u);

protected:
    HeadingObj *heading;	    // Heading
    FlagRowObj *systemFlagRowObj;   // System Flags
    FlagRowObj *standardFlagRowObj; // Standard Flags
    FrameObj *frame;		// frame around object
    QRectF ornamentsBBox;	// bbox of flags and heading
};

#endif
