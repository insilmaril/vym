#ifndef SELECTABLE_H
#define SELECTABLE_H

//#include "branch-container.h"
//#include "container.h"
#include "minimal-branch-container.h"

class BranchContainer;

class SelectableContainer {  // FIXME-0 required to inhert Contaier? Star inheritance in BC?
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
