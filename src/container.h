#ifndef CONTAINER_H
#define CONTAINER_H

#include <QGraphicsRectItem>

class MapObj;

class Container : public QGraphicsRectItem {
  public:
    /*! Type of this container */
    enum ContainerType {Undefined, Collection, Branch, Heading};

    /*! How should this container be considered in bounding boxes of parent? */
    enum BoundsType {Bounded, BoundedFloat, FreeFloat};

    /*! Alignment of children containers */
    enum LayoutType {Horizontal, Vertical};
    enum HorizontalDirection {LeftToRight, RightToLeft};

    Container (QGraphicsItem *parent = NULL);
    virtual ~Container();
    virtual void init();

    ContainerType containerType();

    void setLayoutType(const LayoutType &ltype);
    void setHorizontalDirection(const HorizontalDirection &hdir);

    void addContainer(Container *c);

    
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);
    virtual void reposition();

  protected:

    ContainerType type;
    BoundsType boundsType;

    QString name;

    LayoutType layout;
    HorizontalDirection horizontalDirection;
};

#endif
