#ifndef LINKABLE_CONTAINER_H
#define LINKABLE_CONTAINER_H

#include "linkobj.h"

class BranchContainer;
class ImageContainer;

class LinkableContainer
{
  friend class BranchContainer;  
  friend class ImageContainer;
  public:
    LinkableContainer();
    virtual ~LinkableContainer();

/*

  public:
    // FIXME needed? LinkContainer* getLinkContainer();
    LinkObj* getLink();
    void linkTo(BranchContainer *);

    //! Get suggestion where new child could be positioned (scene coord)
    QPointF getPositionHintNewChild(Container*);
*/

    LinkObj* getLink();

    //! Set hints where to place links between items
    void setUpLinkPosHint(const LinkObj::PosHint &);
    void setDownLinkPosHint(const LinkObj::PosHint &);

/*
    //! Get scene positions for links depending on frameType and orientation
    QPointF downLinkPos();
    QPointF downLinkPos(const Orientation &orientationChild);   // FIXME Orientation could move from BC_BASE to LinkableContainer
*/

  protected:
    LinkObj::PosHint upLinkPosHintInt;
    LinkObj::PosHint downLinkPosHintInt;

/*
  public:
    //! Update "upwards" links in LinkContainer
    void updateUpLink();
*/
  protected:
    // Uplink to parent
    LinkObj *upLink;

/*
    // Save layout, alignment and brush of children containers 
    // even before containers are created on demand
    LinkContainer *linkContainer;
*/
};

#endif
