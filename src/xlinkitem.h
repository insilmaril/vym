#ifndef XLINKITEM_H
#define XLINKITEM_H

class BranchItem;
class QGraphicsScene;
class XLinkObj;

#include "mapitem.h"
#include "xlink.h"

/*! \brief xlinks are used to draw arbitrary connections between branches
 * (BranchObj) in the map. */

/////////////////////////////////////////////////////////////////////////////

class XLinkItem : public MapItem {
  public:
    enum XLinkState { undefinedXLink, initXLink, activeXLink, deleteXLink };

    XLinkItem(TreeItem *parent = NULL);
    ~XLinkItem();
    void init();
    void clear();
    void setLink(Link *);
    Link *getLink();
    void updateXLink();
    MapObj *getMO();
    void setSelection();
    BranchItem *getPartnerBranch();

  private:
    Link *link;
};

#endif
