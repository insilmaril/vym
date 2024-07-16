#ifndef XLINKITEM_H
#define XLINKITEM_H

#include "mapitem.h"

class BranchItem;
class QGraphicsScene;
class XLink;
class XLinkObj;
class XLinkWrapper;

// #include "xlink.h"

/*! \brief xlinks are used to draw arbitrary connections between branches
 * (BranchObj) in the map. */

// #include "xlink.h"

/////////////////////////////////////////////////////////////////////////////

class XLinkItem : public MapItem {
  public:
    enum XLinkState { undefinedXLink, initXLink, activeXLink, deleteXLink };

    XLinkItem(TreeItem *parent = nullptr);
    ~XLinkItem();
    void init();
    void setXLink(XLink *);
    XLink* getXLink();
    void updateXLink();
    XLinkObj *getXLinkObj();
    QColor headingColor();
    void setSelectionType();
    BranchItem *getPartnerBranch();

  private:
    XLink *xlinkInt;
};

#endif
