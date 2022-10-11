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
    QPointF pos;    // FIXME-2 should be removed and position retrieved directly from container - if needed after all

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

  public:
    /*! Return path to highlight currently selected items in scene coordinates */
    virtual QPainterPath getSelectionPath();

    /*! Return position to edit headings in scene coordinates */
    virtual QPointF getEditPosition();
};

#endif
