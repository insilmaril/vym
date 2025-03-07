#ifndef BRANCH_CONTAINER_BASE_H
#define BRANCH_CONTAINER_BASE_H


#include "container.h"
#include "selectable-container.h"

class BranchContainerBase : public Container, public SelectableContainer {
  friend class SelectableContainer;

  public:
    /*! Orientation relative to parent branch container */
    enum Orientation {
        UndefinedOrientation,
        LeftOfParent,
        RightOfParent
    };

    BranchContainerBase ();
    virtual void init();

    void setOrientation(const Orientation &);
    Orientation getOrientation();

  public:
    int childrenCount();    //! Sum of branch and image children

    int branchCount();

    /*! branchesContainer exists only, if there are children branches
     *
     *  branchesContainer and linkSpaceContainer are children of innerContainer.
     *  The linkSpaceContainer is existing, only if a !Floating layout is used AND 
     *  there is a branchesContainer 
     */
    virtual void addToBranchesContainer(BranchContainer *);
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
    Container *imagesAndBranchesContainer;  // Container, which contains both of above in special cases

    Orientation orientation;
};

#endif
