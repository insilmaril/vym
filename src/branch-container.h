#ifndef BRANCH_CONTAINER_H
#define BRANCH_CONTAINER_H

#include "container.h"

class BranchItem;
class HeadingContainer;

class BranchContainer : public Container {
  public:
    BranchContainer (QGraphicsScene *scene, QGraphicsItem *parent = NULL, BranchItem *bi = NULL);
    virtual ~BranchContainer();
    virtual void init();

    void setBranchItem(BranchItem *);
    BranchItem *getBranchItem() const;

    void addToChildrenContainer(Container *c);
    Container* getChildrenContainer();

    QRectF getHeadingRect();  //! Return rectangle of HeadingContainer in absolute coordinates

    void setTmpParentContainer(BranchItem* dstBI, QPointF mousePos, int offset);
    void unsetTmpParentContainer(QPointF absPos = QPointF());

    bool isInClickBox(const QPointF &p);

    void updateVisuals();

    void reposition();

  protected:
    BranchItem *branchItem; //! Crossreference to "parent" BranchItem 
    HeadingContainer *headingContainer;
    Container *childrenContainer;
};

#endif
