#ifndef BRANCH_CONTAINER_H
#define BRANCH_CONTAINER_H

#include <QBrush>

#include "container.h"

class BranchItem;
class HeadingContainer;
class LinkContainer;

class BranchContainer : public Container {
  public:
    /*! Orientation relative to parent branch container */
    enum Orientation {
        UndefinedOrientation,
        LeftOfParent,
        RightOfParent
    };

    BranchContainer (QGraphicsScene *scene, QGraphicsItem *parent = NULL, BranchItem *bi = NULL);
    virtual ~BranchContainer();
    virtual void init();

    BranchContainer* parentBranchContainer();

    void setBranchItem(BranchItem *);
    BranchItem *getBranchItem() const;

    virtual QString getName();

    void setOrientation(const Orientation &);
    void setOriginalOrientation();
    Orientation getOriginalOrientation();
    Orientation getOrientation();
    QPointF getOriginalParentPos();

    void setRealScenePos(const QPointF &);  //! Move ornaments container to scenePos
    QPointF getRealScenePos();              //! scenePos of ornamentsContainer
    void setRealRelPos(const QPointF &);  //! Move ornaments container to relative pos
    QPointF getRealRelPos();              //! relPos of ornamentsContainer to parent oC


    bool isOriginalFloating();

  private:
    bool temporaryLinked;   //! True, while moved as child of tmpParentContainer and linked temporary

  public:
    void setTemporaryLinked();
    void unsetTemporaryLinked();
    bool isTemporaryLinked();

    int childrenCount();    //! Sum of branch and image children

    int branchCount();

    /*! branchesContainer exists only, if there are children branches
     *
     *  branchesContainer and linkSpaceContainer are children of innerContainer.
     *  The linkSpaceContainer is existing, only if a !Floating layout is used AND 
     *  there is a branchesContainer 
     */
    bool hasFloatingBranchesLayout(); //! Checks, if children branches are or should be floating
    void createBranchesContainer();
    void addToBranchesContainer(Container *c, bool keepScenePos = false);
    Container* getBranchesContainer();
    void updateBranchesContainer();    //! Remove unused containers and add needed ones


    int imageCount();
    void createImagesContainer();
    void addToImagesContainer(Container *c, bool keepScenePos = false);
    Container* getImagesContainer();

    HeadingContainer* getHeadingContainer();
    LinkContainer* getLinkContainer();

    // Convenience functions to access children
    QList <BranchContainer*> childBranches();
    QList <ImageContainer*> childImages();

    /*! Get suggestion where new child could be positioned (scene coord) */
    QPointF getPositionHintNewChild(Container*);

    /*! Get suggestion where a relinked child could be positioned (scene coord) */
    QPointF getPositionHintRelink(Container*, int d_pos = 0, const QPointF & p_scene = QPointF());

    /*! Update "upwards" links in LinkContainer */
    void updateUpLink();

    virtual void setLayout(const Layout &l);
    virtual void switchLayout(const Layout &l);

    virtual void setBranchesContainerLayout(const Layout &l);
    virtual void setBranchesContainerHorizontalAlignment(const HorizontalAlignment &valign);
    virtual void setBranchesContainerBrush(const QBrush &b);

    QRectF getHeadingRect();  //! Return rectangle of HeadingContainer in absolute coordinates

    void setRotationHeading(const int &);
    int getRotationHeading();
    void setRotationInnerContent(const int &);
    int getRotationInnerContent();


    bool isInClickBox(const QPointF &p);

    void updateVisuals();

    void reposition();

  protected:
    static qreal linkWidth;
    BranchItem *branchItem; //! Crossreference to "parent" BranchItem 
    
    // Save layout, alignment and brush of branchesContainer 
    // even before container is created on demand
    Layout branchesContainerLayout;
    HorizontalAlignment branchesContainerHorizontalAlignment;
    QBrush branchesContainerBrush;

    HeadingContainer *headingContainer; // Heading of this branch
    HeadingContainer *linkSpaceContainer; // space for downLinks
    LinkContainer *linkContainer;       // uplink to parent
    Container *branchesContainer;       // Container with children branches
    Container *imagesContainer;         // Container with children images
    Container *ornamentsContainer;      // Flags and heading
    Container *innerContainer;          // Ornaments (see above) and children branches

  private:
    Orientation orientation;
    Orientation originalOrientation;            //! Save orientation before move for undo
    bool originalFloating;                      //! Save, if floating before linked temporary
    QPointF originalParentPos;  // FIXME-0 Currently used in ME to determine orientation during move: scene coord of orig, parent

};

#endif
