#ifndef HEADING_CONTAINER_H
#define HEADING_CONTAINER_H

#include "container.h"

class HeadingObj;

class HeadingContainer : public Container {
  public:
    HeadingContainer (QGraphicsItem *parent = NULL);
    virtual ~HeadingContainer();
    virtual void init();

    void setText(const QString &);
    virtual QString getName();

    virtual void reposition();

  protected:
    HeadingObj *headingObj; 
};

#endif
