#ifndef BRANCH_CONTAINER_BASE_H
#define BRANCH_CONTAINER_BASE_H

#include <QBrush>

#include "container.h"

class BranchContainerBase : public Container {
  public:
    /*! Orientation relative to parent branch container */
    enum Orientation {
        UndefinedOrientation,
        LeftOfParent,
        RightOfParent
    };

    /*! States related to moving around */
    enum MovingState {
        NotMoving,
        Moving,
        TemporaryLinked
    };

    BranchContainerBase ();
    virtual void init();

    void setOrientation(const Orientation &);
    Orientation getOrientation();


  protected:
    MovingState movingStateInt;
    BranchContainer *tmpLinkedParentContainer;
    BranchContainer *originalParentBranchContainer;

  public:
    void setMovingState(const MovingState &, BranchContainer *tpc = nullptr);
    MovingState movingState();

  public:
    int childrenCount();    //! Sum of branch and image children

    int branchCount();

    /*! branchesContainer exists only, if there are children branches
     *
     *  branchesContainer and linkSpaceContainer are children of innerContainer.
     *  The linkSpaceContainer is existing, only if a !Floating layout is used AND 
     *  there is a branchesContainer 
     */
    virtual void addToBranchesContainer(Container *c);
    Container* getBranchesContainer();

    int imageCount();
    virtual void createImagesContainer();
    virtual void addToImagesContainer(Container *c);
    Container* getImagesContainer();

    // Convenience functions to access children
    QList <BranchContainer*> childBranches();
    QList <ImageContainer*> childImages();

  public:
    virtual void reposition();

  protected:
    Container::HorizontalAlignment branchesContainerHorizontalAlignment;

    Container *branchesContainer;       // Container with children branches
    Container *imagesContainer;         // Container with children images

    Orientation orientation;

  private:
    bool originalFloating;                      //! Save, if floating before linked temporary   // FIXME-2 needed?

};

#endif
