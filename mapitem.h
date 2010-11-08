#ifndef MAPITEM_H
#define MAPITEM_H

#include <QPointF>

#include "treeitem.h"

class LinkableMapObj;

/*! /brief MapItem is used to store information of MapObj and inherited
   classes.
 
    This is done even while no QGraphicsView is availabe. This is useful
    if e.g. on a small device like a cellphone the full map is not used,
    but just a treeview instead.
*/

class MapItem:public TreeItem
{
public:
    enum PositionMode {Unused,Absolute,Relative};
protected:
    QPointF pos;
    PositionMode posMode;

public:
    MapItem();
    MapItem(const QList<QVariant> &data, TreeItem *parent = 0);

    void init();

    /*! Overloaded from TreeItem. Used to set parObj in LinkableMapObj */
    virtual void appendChild (TreeItem *item);

    /*! Used to save relative position while map is not in QGraphicsView */
    virtual void setRelPos(const QPointF&); 

    /*! Used to save absolute position while map is not in QGraphicsView */
    virtual void setAbsPos(const QPointF&); 

    /*! Tell object to use e.g. absolute positioning for mapcenter. 
	Defaulst is MapItem::Unused */
    void setPositionMode (PositionMode mode);
    PositionMode getPositionMode ();


protected:
    bool hideLinkUnselected;
public:
    /*! Hide link if item is not selected */
    virtual void setHideLinkUnselected(bool);

    /*! Check if link is hidden for unselected items */
    virtual bool getHideLinkUnselected();

    virtual QString getMapAttr();   //! Get attributes for saving as XML

    virtual QRectF getBBoxURLFlag();//! get bbox of url flag
    virtual QRectF getBBoxFlag   (const QString &fname);    //! get bbox of flag


protected:
    LinkableMapObj *lmo;
public:
    /*! Returns pointer to related LinkableMapObj in QGraphicsView */
    virtual LinkableMapObj* getLMO();

    /*! Set pointer to related LinkableMapObj in QGraphicsView */
    virtual void setLMO (LinkableMapObj*);

    /*! Initialize LinkableMapObj with data in MapItem */
    virtual void initLMO();

};


#endif
