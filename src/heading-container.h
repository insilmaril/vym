#ifndef HEADING_CONTAINER_H
#define HEADING_CONTAINER_H

#include "container.h"

class BranchItem;

class HeadingContainer : public Container { //FIXME-0   branchItem really required? only to get current heading...
  public:
    HeadingContainer (QGraphicsItem *parent = NULL, BranchItem *bi = NULL);
    virtual ~HeadingContainer();
    virtual void init();
    virtual void copy(Container *);

    void setBranchItem(BranchItem *);
    BranchItem *getBranchItem() const;

  protected:
    BranchItem *branchItem; //! Crossreference to "parent" BranchItem 
};

#endif
