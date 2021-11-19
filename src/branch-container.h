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

    void updateVisuals();

  protected:
    BranchItem *branchItem; //! Crossreference to "parent" BranchItem 
    HeadingContainer *headingContainer;
    Container *childrenContainer;
};

#endif
