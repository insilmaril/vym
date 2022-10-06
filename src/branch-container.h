#ifndef BRANCH_CONTAINER_H
#define BRANCH_CONTAINER_H

#include <QBrush>

#include "container.h"
#include "frame-container.h"

class BranchItem;
class FlagRowContainer;
// class FrameContainer;
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

    BranchContainer (
            QGraphicsScene *scene,
            QGraphicsItem *parent = nullptr,
            BranchItem *bi = nullptr);
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

    void setScrollOpacity(qreal a);
    qreal getScrollOpacity();
  private:
    qreal scrollOpacity;

  public:
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

  private:
    void updateBranchesContainer();    //! Remove unused containers and add needed ones
    void createOuterContainer();        //! Used if only images have FloatingBounded layout
    void deleteOuterContainer();
    void updateChildrenStructure();     //! Depending on layouts of children, rearrange structure

  public:
    void showStructure();     // Print structure for debugging
    int imageCount();
    void createImagesContainer();
    void addToImagesContainer(Container *c, bool keepScenePos = false);
    Container* getImagesContainer();

    HeadingContainer* getHeadingContainer();
    LinkContainer* getLinkContainer();

    FrameContainer* createFrameContainer();
    FrameContainer* getFrameContainer();
    void deleteFrameContainer();

    // Convenience functions to access children
    QList <BranchContainer*> childBranches();
    QList <ImageContainer*> childImages();

    /*! Get suggestion where new child could be positioned (scene coord) */
    QPointF getPositionHintNewChild(Container*);

    /*! Get suggestion where a relinked child could be positioned (scene coord) */
    QPointF getPositionHintRelink(Container*, int d_pos = 0, const QPointF & p_scene = QPointF());

    /*! Update "upwards" links in LinkContainer */
    void updateUpLink();

    void setLayout(const Layout &l);

    bool imagesContainerAutoLayout;
    void setImagesContainerLayout(const Layout &l);
    Container::Layout getImagesContainerLayout();

    bool branchesContainerAutoLayout;
    void setBranchesContainerLayout(const Layout &l);
    Container::Layout getBranchesContainerLayout();
    void setBranchesContainerHorizontalAlignment(const HorizontalAlignment &valign);
    void setBranchesContainerBrush(const QBrush &b);

    QRectF getHeadingRect();  //! Return rectangle of HeadingContainer in absolute coordinates

    void setRotationHeading(const int &);
    int getRotationHeading();
    void setRotationContent(const int &);
    int getRotationInnerContent();

    QUuid findFlagByPos(const QPointF &p);
    bool isInClickBox(const QPointF &p);

    void updateVisuals();

    Container::Layout getDefaultBranchesContainerLayout();
    Container::Layout getDefaultImagesContainerLayout();

    void reposition();

  protected:
    static qreal linkWidth;
    BranchItem *branchItem; //! Crossreference to "parent" BranchItem 

    // Save layout, alignment and brush of children containers 
    // even before containers are created on demand
    Layout imagesContainerLayout;
    Layout branchesContainerLayout;
    HorizontalAlignment branchesContainerHorizontalAlignment;
    QBrush branchesContainerBrush;

    HeadingContainer *headingContainer; // Heading of this branch
    HeadingContainer *linkSpaceContainer; // space for downLinks
    LinkContainer *linkContainer;       // uplink to parent
    FrameContainer *frameContainer;
    Container *branchesContainer;       // Container with children branches
    Container *imagesContainer;         // Container with children images
    Container *ornamentsContainer;      // Flags and heading
    Container *innerContainer;          // Ornaments (see above) and children branches
    Container *outerContainer;          // Used only with FloatingBounded images and vertical branches

    FlagRowContainer *standardFlagRowContainer;
    FlagRowContainer *systemFlagRowContainer;

  private:
    Orientation orientation;
    Orientation originalOrientation;            //! Save orientation before move for undo
    bool originalFloating;                      //! Save, if floating before linked temporary
    QPointF originalParentPos;                  // used in ME to determine orientation during move: scene coord of orig, parent

};

#endif
