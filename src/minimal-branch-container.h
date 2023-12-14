#ifndef MINIMAL_BRANCH_CONTAINER_H
#define MINIMAL_BRANCH_CONTAINER_H

#include <QBrush>

// #include "branch-container.h"
#include "container.h"

class MinimalBranchContainer : public Container {
  public:
    /*! Orientation relative to parent branch container */
    enum Orientation {
        UndefinedOrientation,
        LeftOfParent,
        RightOfParent
    };

    MinimalBranchContainer ();
    virtual ~MinimalBranchContainer();
    virtual void init();

    void setOrientation(const Orientation &);
    Orientation getOrientation();

  protected:
    BranchContainer *tmpLinkedParentContainer;
    BranchContainer *originalParentBranchContainer;

  public:
    void setTemporaryLinked(BranchContainer *tpc);
    void unsetTemporaryLinked();
    bool isTemporaryLinked();

  public:
    int childrenCount();    //! Sum of branch and image children

    int branchCount();

    /*! branchesContainer exists only, if there are children branches
     *
     *  branchesContainer and linkSpaceContainer are children of innerContainer.
     *  The linkSpaceContainer is existing, only if a !Floating layout is used AND 
     *  there is a branchesContainer 
     */
    bool hasFloatingBranchesLayout(); //! Checks, if children branches are or should be floating
    bool hasFloatingImagesLayout(); //! Checks, if children images are or should be floating
    void addToBranchesContainer(Container *c);
    Container* getBranchesContainer();

    int imageCount();
    void createImagesContainer();
    void addToImagesContainer(Container *c);
    Container* getImagesContainer();

    // Convenience functions to access children
    QList <BranchContainer*> childBranches();
    QList <ImageContainer*> childImages();


    void setLayout(const Container::Layout &l);

    bool imagesContainerAutoLayout;
    void setImagesContainerLayout(const Container::Layout &l);
    Container::Layout getImagesContainerLayout();

    bool branchesContainerAutoLayout;
    void setBranchesContainerLayout(const Container::Layout &l);
    Container::Layout getBranchesContainerLayout();
    void setBranchesContainerHorizontalAlignment(const Container::HorizontalAlignment &valign);

  private:
    void updateBranchesContainerLayout();

  public:
    void reposition();

  protected:
    // Save layout, alignment and brush of children containers 
    // even before containers are created on demand
    Container::Layout imagesContainerLayout;
    Container::Layout branchesContainerLayout;
    Container::HorizontalAlignment branchesContainerHorizontalAlignment;

    Container *branchesContainer;       // Container with children branches
    Container *imagesContainer;         // Container with children images

    Orientation orientation;

  private:
    bool originalFloating;                      //! Save, if floating before linked temporary   // FIXME-2 needed?

};

#endif
