#ifndef SELECTABLE_H
#define SELECTABLE_H

#include "frame-container.h"

class SelectableContainer:public Container {
  friend class BranchContainer;
  public:
    virtual void select(Container *container,const QColor &color);
    void unselect();
    bool isSelected();

  protected:
    Container *selectionContainer;
};

#endif
