#ifndef CONTAINER_H
#define CONTAINER_H

#include <QGraphicsRectItem>

class MapObj;

class Container : public QGraphicsRectItem {
  public:
    enum ContainerType {Undefined, Collection, Branch, Heading};
    enum LayoutType {Horizontal, Vertical};
    enum HorizontalDirection {LeftToRight, RightToLeft};

    Container (QGraphicsItem *parent = NULL);
    virtual ~Container();
    virtual void init();

    ContainerType containerType();

    void setLayoutType(const LayoutType &ltype);
    void setHorizontalDirection(const HorizontalDirection &hdir);

    void addContainer(Container *c);

    virtual void reposition();

  protected:

    ContainerType type;

    QString name;

    LayoutType layout;
    HorizontalDirection horizontalDirection;
};

#endif
