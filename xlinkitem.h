#ifndef XLINKITEM_H
#define XLINKITEM_H

class BranchItem;
class QGraphicsScene;
class XLinkObj;

#include "mapitem.h"
#include "xlink.h"

/*! \brief xlinks are used to draw arbitrary connections between branches (BranchObj) in the map. */

/////////////////////////////////////////////////////////////////////////////

class XLinkItem:public MapItem {
public:
    enum XLinkState {undefinedXLink,initXLink,activeXLink,deleteXLink};	

    XLinkItem (const QList<QVariant> &data, TreeItem *parent=NULL);
    virtual ~XLinkItem ();
    virtual void init ();
    void setLink (Link*);
    Link* getLink ();
    void updateXLink();
    MapObj* getMO();
    BranchItem* getPartnerBranch ();


private:
    Link *link;
};

#endif
