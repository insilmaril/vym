#ifndef BRANCH_CONTAINER_H
#define BRANCH_CONTAINER_H

#include "container.h"

class BranchItem;
class HeadingContainer;

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
    Orientation getOrientation();

  private:
    bool temporaryLinked;   //! True, while moved as child of tmpParentContainer and linked temporary
  public:
    void setTemporaryLinked(bool);
    bool isTemporaryLinked();

    void addToBranchesContainer(Container *c, bool keepScenePos = false);
    void addToImagesContainer(Container *c, bool keepScenePos = false);
    Container* getBranchesContainer();
    Container* getImagesContainer();
    HeadingContainer* getHeadingContainer();

    // Convenience functions to access children
    QList <BranchContainer*> childBranches();
    QList <ImageContainer*> childImages();

    /*! Get suggestion where new child could be positioned in scene coord */
    QPointF getPositionHintNewChild(Container*);

    /*! Get suggestion where a relinked child could be positioned in scene coord */
    QPointF getPositionHintRelink(Container*, int d_pos = 0, const QPointF & p_scene = QPointF());

    virtual void setLayoutType(const LayoutType &ltype);

    QRectF getHeadingRect();  //! Return rectangle of HeadingContainer in absolute coordinates

    bool isInClickBox(const QPointF &p);

    void updateVisuals();

    void reposition();

  protected:
    static qreal linkWidth;
    BranchItem *branchItem; //! Crossreference to "parent" BranchItem 
    HeadingContainer *headingContainer;
    Container *branchesContainer;
    Container *imagesContainer;
    Container *innerContainer;

  private:
    Orientation orientation;
    Orientation originalOrientation;//! Save orientation before move for undo

};

#endif
