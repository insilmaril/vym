#ifndef BRANCH_CONTAINER_H
#define BRANCH_CONTAINER_H

#include "container.h"

class BranchItem;
class HeadingContainer;

class BranchContainer : public Container {
  public:
    BranchContainer (QGraphicsScene *scene, QGraphicsItem *parent = NULL, BranchItem *bi = NULL);
    virtual ~BranchContainer();
    virtual void init();

    void setBranchItem(BranchItem *);
    BranchItem *getBranchItem() const;

    virtual QString getName();

    void addToBranchesContainer(Container *c, bool keepScenePos = false);
    void addToImagesContainer(Container *c, bool keepScenePos = false);
    Container* getBranchesContainer();
    Container* getImagesContainer();
    HeadingContainer* getHeadingContainer();

    QPointF getChildrenPosHint(Container*);   //! Suggestion where new children could be positioned

    virtual void setLayoutType(const LayoutType &ltype);

    QRectF getHeadingRect();  //! Return rectangle of HeadingContainer in absolute coordinates

    bool isInClickBox(const QPointF &p);

    void updateVisuals();

    void reposition();

  protected:
    BranchItem *branchItem; //! Crossreference to "parent" BranchItem 
    HeadingContainer *headingContainer;
    Container *branchesContainer;
    Container *imagesContainer;
    Container *innerContainer;
};

#endif
