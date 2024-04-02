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

    XLinkItem(TreeItem *parent = nullptr);
    ~XLinkItem();
    void init();
    void setLink(Link *);
    Link *getLink();
    void updateXLink();
    XLinkObj *getXLinkObj();
    QColor headingColor();
    void setSelection();
    BranchItem *getPartnerBranch();

  private:
    Link *link;
};

#endif
