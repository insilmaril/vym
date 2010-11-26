#ifndef MAPOBJ_H
#define MAPOBJ_H

#include <QGraphicsScene>
#include <QGraphicsItem>

#include "xmlobj.h"

#define dZ_BBOX         0   // testing
#define dZ_DEPTH      100
#define dZ_FRAME_LOW   10	
#define dZ_LINK        20
#define dZ_FRAME_HIGH  30	
#define dZ_XLINK       40
#define dZ_SELBOX      60
#define dZ_FLOATIMG    70
#define dZ_ICON        80
#define dZ_TEXT        90
#define  Z_INIT      9999
#define  Z_LINEEDIT 10000 

class ConvexPolygon;

#include "treeitem.h"

/*! \brief Base class for all objects visible on a map
*/

class MapObj:public XMLObj {
public:
    MapObj ();
    MapObj (QGraphicsScene *scene,TreeItem *ti=NULL);
    MapObj (MapObj*);
    virtual ~MapObj ();
    virtual void init ();
    virtual void copy (MapObj*);

    virtual void setTreeItem(TreeItem *);
    virtual TreeItem* getTreeItem() const;

    virtual QGraphicsScene* getScene();
    virtual qreal x();
    virtual qreal y();
    virtual qreal width();
    virtual qreal height();
    virtual QPointF getAbsPos();
    virtual QString getPos();			//! Return position as string (x,y)
    virtual void move (double x,double y);      //! move to absolute Position
    virtual void move (QPointF p);
    virtual void moveBy (double x,double y);    //! move to relative Position
    virtual QRectF getBBox();			//! returns bounding box
    virtual ConvexPolygon getBoundingPolygon();	//! return bounding convex polygon
    virtual QRectF getClickBox();		//! returns box to click
    virtual bool isInClickBox (const QPointF &p);   //! Checks if p is in clickBox
    virtual QSizeF getSize();			//! returns size of bounding box
    virtual bool isVisibleObj();
    virtual void setVisibility(bool);
    virtual void positionBBox()=0;       
    virtual void calcBBoxSize()=0;
protected:  
    QGraphicsScene* scene;
    QRectF bbox;		    // bounding box of MO itself
    QRectF clickBox;		    // area where mouseclicks are found
    QPointF absPos;		    // Position on canvas
    bool visible;

    TreeItem *treeItem;		    //! Crossrefence to treemodel

    QGraphicsPolygonItem *pi;	//FIXME-3 testing only
};

#endif
