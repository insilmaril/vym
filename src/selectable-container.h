#ifndef SELECTABLE_H
#define SELECTABLE_H

//#include "branch-container.h"
//#include "container.h"
#include "minimal-branch-container.h"

class BranchContainer;

class SelectableContainer : public MinimalBranchContainer {
  friend class BranchContainer;
  public:
    virtual void select(
            Container *container,
            const QPen &,
            const QBrush &);
    void unselect();
    bool isSelected();

  protected:
    Container *selectionContainer;
};

#endif
