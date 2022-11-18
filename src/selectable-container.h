#ifndef SELECTABLE_H
#define SELECTABLE_H

#include "frame-container.h"

class SelectableContainer:public FrameContainer {
  friend class BranchContainer;
  public:
    virtual void select(Container *c);
    void unselect();
    bool isSelected();

  protected:
    Container *selectionContainer;
};

#endif
