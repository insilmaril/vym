#ifndef MAPITEM_H
#define MAPITEM_H

#include <QPointF>

#include "treeitem.h"

class Container;
/*! /brief MapItem is used to maintain geometrical information of images and branches
 *  resp. their containers
*/

class MapItem : public TreeItem {
  public:
    MapItem(TreeItem *parent = nullptr);

    void init();

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
};

#endif
