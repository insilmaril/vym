#ifndef SELECTABLE_CONTAINER_H
#define SELECTABLE_CONTAINER_H

#include <QBrush>

class BranchContainer;
class Container;

class SelectableContainer {
  friend class BranchContainer;
  friend class BranchContainerBase;
  friend class ImageContainer;

  public:
    /*! States related to moving around */
    enum MovingState {
        NotMoving,
        Moving,
        TemporaryLinked
    };

    SelectableContainer();

    virtual void select(
            Container *container,
            const QPen &,
            const QBrush &);
    void unselect();
    bool isSelected();

  protected:
    MovingState movingStateInt;
    BranchContainer *tmpLinkedParentContainer;
    BranchContainer *originalParentBranchContainer;

  public:
    void setMovingState(const MovingState &, BranchContainer *tpc = nullptr);
    MovingState movingState();

  protected:
    Container *selectionContainer;
};

#endif
