#ifndef CONTAINER_H
#define CONTAINER_H

#include <QGraphicsRectItem>

class MapObj;

class Container : public QGraphicsRectItem {
  public:
    /*! Type of this container */
    enum ContainerType {Undefined, Collection, Branch, Heading};

    /*! How are children containers and boundaries organized? */
    enum BoundsType {BoundedStacked, BoundedFloating, FreeFloating};

    /*! Alignment of children containers */
    enum LayoutType {Horizontal, Vertical};
    enum HorizontalDirection {LeftToRight, RightToLeft};
    enum VerticalAlignment {Left, Centered, Right};

    Container (QGraphicsItem *parent = NULL);
    virtual ~Container();
    virtual void init();

    ContainerType containerType();

    void setLayoutType(const LayoutType &ltype);
    void setHorizontalDirection(const HorizontalDirection &hdir);
    void setVerticalAlignment(const VerticalAlignment &a);
    void setBoundsType(const BoundsType &btype);

    void addContainer(Container *c);

    
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);
    virtual void reposition();

  protected:

    ContainerType type;
    BoundsType boundsType;

    QString name;

    LayoutType layout;
    HorizontalDirection horizontalDirection;
    VerticalAlignment verticalAlignment;
};

#endif
