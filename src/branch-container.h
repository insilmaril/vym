#ifndef BRANCH_CONTAINER_H
#define BRANCH_CONTAINER_H

#include "container.h"

class BranchItem;

class BranchContainer : public Container {
  public:
    BranchContainer (QGraphicsItem *parent = NULL, BranchItem *bi = NULL);
    virtual ~BranchContainer();
    virtual void init();
    virtual void copy(Container *);

    void setBranchItem(BranchItem *);
    BranchItem *getBranchItem() const;

  protected:
    BranchItem *branchItem; //! Crossreference to "parent" BranchItem 
};

#endif
