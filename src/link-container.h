#ifndef LINK_CONTAINER_H
#define LINK_CONTAINER_H

#include "container.h"

class LinkObj;

/*! \brief This container class is parent of LinkObjs pointing to children branches and images.
 * The parenting is required for correct z-ordering, so that links go below frames.
*/

class LinkContainer : public Container {
  public:
    LinkContainer();
    virtual ~LinkContainer();

  protected:
    void init();

  public:
    void addLink(LinkObj*);
    void reposition();
};
#endif
