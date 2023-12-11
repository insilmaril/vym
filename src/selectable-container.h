#ifndef SELECTABLE_H
#define SELECTABLE_H

//#include "branch-container.h"
//#include "container.h"
#include "minimal-branch-container.h"

class BranchContainer;

class SelectableContainer {
  friend class BranchContainer;
  public:
    SelectableContainer();
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
