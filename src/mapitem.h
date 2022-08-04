#ifndef MAPITEM_H
#define MAPITEM_H

#include <QPointF>

#include "treeitem.h"

class Container;
class MapObj;

/*! /brief MapItem is used to maintain geometrical information of images and branches
 *  resp. their containers
*/

class MapItem : public TreeItem {
  protected:
    QPointF pos;    // FIXME-2 should be removed and retrieved directly from container

  public:
    MapItem(TreeItem *parent = nullptr);

    void init();

    /*! Overloaded from TreeItem. Used to set parObj in LinkableMapObj */
    virtual void appendChild(TreeItem *item);

    /*! Overloaded in BranchItem and ImageItem to retrieve the related container */
    virtual Container* getContainer();

    /*! Used to save position while map is not in QGraphicsView */
    virtual void setPos(const QPointF &);

  protected:
    bool hideLinkUnselected;

  public:
    /*! Hide link if item is not selected */
    virtual void setHideLinkUnselected(bool);

    /*! Check if link is hidden for unselected items */
    virtual bool getHideLinkUnselected();

    virtual QString getPosAttr();       //! Get position attributes shared by Images and Branches
    virtual QString getLinkableAttr();  //! Get attributes shared by Images and Branches

    virtual QRectF getBBoxURLFlag(); //! g   // FIXME-2 Refactor to use container layoutset bbox of url flag

  protected:
    MapObj *mo;
    qreal angle;    // FIXME-2 should be removed and retrieved directly from container

  public:
    /*! Return path to highlight currently selected items in scene coordinates */
    virtual QPainterPath getSelectionPath();

    /*! Return position to edit headings in scene coordinates */
    virtual QPointF getEditPosition();
};

#endif
