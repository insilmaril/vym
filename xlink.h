#ifndef LINK_H
#define LINK_H

#include <QColor>
#include <QPen>

#include "xmlobj.h"

class QPointF;
class QGraphicsScene;
class QString;

class BranchItem;
class MapObj;
class LinkableMapObj;
class VymModel;
class XLinkItem;
class XLinkObj;


class Link:public XMLObj
{
public:
    enum XLinkState {undefinedXLink,initXLink,activeXLink,deleteXLink};	
    enum LinkType {Linear, Bezier};

    Link (VymModel *m);
    virtual ~Link();
    virtual void init ();
    void setBeginBranch (BranchItem*);
    BranchItem* getBeginBranch();
    void setEndBranch   (BranchItem*);
    void setEndPoint(QPointF);
    BranchItem* getEndBranch();
    void setBeginLinkItem (XLinkItem*);
    XLinkItem* getBeginLinkItem();
    void setEndLinkItem (XLinkItem*);
    XLinkItem* getEndLinkItem ();
    XLinkItem* getOtherEnd (XLinkItem*);
    void setPen (const QPen &p);
    QPen getPen();
    void setLinkType (const QString &s);
    bool activate ();		
    void deactivate ();		
    XLinkState getState();
    void removeXLinkItem (XLinkItem *xli);
    void updateLink();
    QString saveToDir ();
    XLinkObj* getXLinkObj();
    XLinkObj* createMapObj();
    MapObj* getMO();

private:
    XLinkState xLinkState;  // init during drawing or active
    LinkType type;
    QPen pen;

    XLinkObj *xlo;
    VymModel *model;

    BranchItem *beginBranch;
    BranchItem *endBranch;
    XLinkItem *beginLinkItem;
    XLinkItem *endLinkItem;
};


#endif

