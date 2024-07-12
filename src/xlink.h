#ifndef LINK_H
#define LINK_H

#include <QColor>
#include <QPen>
#include <QUuid>

#include "scripting.h"

#include "xmlobj.h"
#include "xlink-wrapper.h"

class QPointF;
class QGraphicsScene;
class QString;

class BranchItem;
class VymModel;
class XLinkItem;
class XLinkObj;

class Link : public XMLObj {
  public:
    enum XLinkState { undefinedXLink, initXLink, activeXLink, deleteXLink };
    enum LinkType { Linear, Bezier };

    Link(VymModel *m);
    virtual ~Link();
    virtual void init();
    void setUuid(const QString &id);
    QUuid getUuid();
    VymModel *getModel();
    XLinkWrapper *xlinkWrapper();
    void setBeginBranch(BranchItem *);
    BranchItem *getBeginBranch();
    void setEndBranch(BranchItem *);
    void setEndPoint(QPointF);
    BranchItem *getEndBranch();
    void setBeginXLinkItem(XLinkItem *);
    XLinkItem *beginXLinkItem();
    void setEndXLinkItem(XLinkItem *);
    XLinkItem *endXLinkItem();
    XLinkItem *getOtherEnd(XLinkItem *);
    void setPen(const QPen &p);
    QPen getPen();
    void setLinkType(const QString &s);
    void setStyleBegin(const QString &s);
    QString getStyleBeginString();
    void setStyleEnd(const QString &s);
    QString getStyleEndString();
    bool activate();
    XLinkState getState();
    void updateLink();
    QString saveToDir();
    XLinkObj *getXLinkObj();
    XLinkObj *createXLinkObj();

  private:
    QUuid uuid;

    XLinkState xLinkState; // init during drawing or active
    LinkType type;
    QPen pen;

    XLinkObj *xlo;
    VymModel *model;

    BranchItem *beginBranch;
    BranchItem *endBranch;
    XLinkItem *beginXLinkItemInt;
    XLinkItem *endXLinkItemInt;

    XLinkWrapper *xlinkWrapperInt;
};

#endif
