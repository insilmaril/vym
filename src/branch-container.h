#ifndef BRANCH_CONTAINER_H
#define BRANCH_CONTAINER_H

#include <QBrush>

#include "container.h"
#include "mapdesign.h"
#include "selectable-container.h"

class BranchItem;
class FlagRowContainer;
class HeadingContainer;
class LinkContainer;

class LinkObj;

class BranchContainer : public BranchContainerBase, public SelectableContainer {
  public:
    BranchContainer(
            QGraphicsScene *scene,
            BranchItem *bi = nullptr);
    virtual ~BranchContainer();
    virtual void init();

    BranchContainer* parentBranchContainer();

    void setBranchItem(BranchItem *);
    BranchItem *getBranchItem() const;

    virtual QString getName();

    void setOriginalOrientation();
    Orientation getOriginalOrientation();
    QPointF getOriginalParentPos();

    void setOriginalScenePos();              //! Saves current scene position for later restoring

    void updateVisibilityOfChildren();    // consider scroll state for branchesCont and imagesCont

    void setScrolled(bool b);
    void setScrollOpacity(qreal a);
    qreal getScrollOpacity();
  private:
    qreal scrollOpacity;

  public:
    bool isOriginalFloating();

    void addToBranchesContainer(Container *c);

  private:
    void updateImagesContainer();       //! Remove unused containers and add needed ones
    void createOuterContainer();        //! Used if only images have FloatingBounded layout
    void deleteOuterContainer();

  public:
    void updateChildrenStructure();     //! Depending on layouts of children, rearrange structure
    void createImagesContainer();
    void addToImagesContainer(Container *c);

    HeadingContainer* getHeadingContainer();
    LinkContainer* getLinkContainer();
    LinkObj* getLink();
    void linkTo(BranchContainer *);

    /*! Get suggestion where new child could be positioned (scene coord) */
    QPointF getPositionHintNewChild(Container*);

    /*! Get suggestion where a relinked child could be positioned (scene coord) */
    QPointF getPositionHintRelink(Container*, int d_pos = 0, const QPointF & p_scene = QPointF());

    /*! Get positions for links depending on frameType and orientation*/
    QPointF downLinkPos();
    QPointF downLinkPos(const Orientation &orientationChild);
    QPointF upLinkPos(const Orientation &orientationChild);

    /*! Update "upwards" links in LinkContainer */
    void updateUpLink();

    void setLayout(const Layout &l);

    bool imagesContainerAutoLayout;
    void setImagesContainerLayout(const Layout &l);
    Container::Layout getImagesContainerLayout();
    bool hasFloatingImagesLayout(); //! Checks, if children images are or should be floating

    bool branchesContainerAutoLayout;
    void setBranchesContainerLayout(const Layout &l);
    Container::Layout getBranchesContainerLayout();
    bool hasFloatingBranchesLayout(); //! Checks, if children branches are or should be floating
    void setBranchesContainerHorizontalAlignment(const HorizontalAlignment &valign);
    void setBranchesContainerBrush(const QBrush &b);

    QRectF headingRect();    //! Return rectangle of HeadingContainer in absolute coordinates

    void setRotationHeading(const int &);
    int rotationHeading();
    int rotationHeadingInScene();

    void setRotationSubtree(const int &);
    int rotationSubtree();
  protected:
    qreal rotationHeadingInt;
    qreal rotationSubtreeInt;

  public:

    QUuid findFlagByPos(const QPointF &p);
    bool isInClickBox(const QPointF &p);
    QRectF getBBoxURLFlag();

    virtual void select();  // Overloads SelectableContainer::select

  private:
    bool autoDesignInnerFrame;
    bool autoDesignOuterFrame;

  public:
    // FrameContainer interfaces
    bool frameAutoDesign(const bool &useInnerFrame);
    void setFrameAutoDesign(const bool &userInnerFrame, const bool &);
    FrameContainer::FrameType frameType(const bool &useInnerFrame);
    QString frameTypeString(const bool &useInnerFrame);
    void setFrameType(const bool &useInnerFrame, const FrameContainer::FrameType &);
    void setFrameType(const bool &useInnerFrame, const QString &);

    int framePadding(const bool &useInnerFrame = true);
    void setFramePadding(const bool &useInnerFrame, const int &);
    int framePenWidth(const bool &useInnerFrame);
    void setFramePenWidth(const bool &useInnerFrame, const int &);
    QColor framePenColor(const bool &useInnerFrame);
    void setFramePenColor(const bool &useInnerFrame, const QColor &);
    QColor frameBrushColor(const bool &useInnerFrame);
    void setFrameBrushColor(const bool &useInnerFrame, const QColor&);

    QString saveFrame();

  private:
    void updateBranchesContainerLayout();

  public:
    /*! Update styles (frame, links, fonts, colors, ...) */
    void updateStyles(const MapDesign::UpdateMode &);

    /*! Update flags and heading */
    void updateVisuals();

    void reposition();

  protected:
    static qreal linkWidth;
    BranchItem *branchItem; //! Crossreference to "parent" BranchItem 

    // Uplink to parent
    LinkObj *upLink;

    // Save layout, alignment and brush of children containers 
    // even before containers are created on demand
    Layout imagesContainerLayout;
    Layout branchesContainerLayout;
    HorizontalAlignment branchesContainerHorizontalAlignment;
    QBrush branchesContainerBrush;

    FrameContainer *innerFrame;         // Frame container around ornamentsContainer
    FrameContainer *outerFrame;         // Frame container around whole BranchContainer
    HeadingContainer *headingContainer; // Heading of this branch
    HeadingContainer *linkSpaceContainer; // space for downLinks
    LinkContainer *linkContainer;       // uplink to parent
    Container *listContainer;           // Container for bullet point lists, if used
    HeadingContainer *bulletPointContainer;  // if lists are used, contains bulletpoint
    Container *ornamentsContainer;      // Flags and heading
    Container *innerContainer;          // Ornaments (see above) and children branches
    Container *outerContainer;          // Used only with FloatingBounded images and vertical branches

    FlagRowContainer *standardFlagRowContainer;
    FlagRowContainer *systemFlagRowContainer;

  private:
    Orientation originalOrientation;            //! Save orientation before move for undo
    bool originalFloating;                      //! Save, if floating before linked temporary
    QPointF originalParentPos;                  // used in ME to determine orientation during move: scene coord of orig, parent

};

#endif
