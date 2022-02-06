#ifndef CONTAINER_H
#define CONTAINER_H

#include <QGraphicsRectItem>

class BranchContainer;
class MapObj;

class Container : public QGraphicsRectItem {
  friend class BranchContainer;
  public:
    /*! Type of this container */
    enum ContainerType {
        Undefined, 
        TmpParent,
        InnerContent,
        Children, 
        Branch, 
        Heading
    };

    /*! How are children containers and boundaries organized? */
    enum BoundsType {BoundedStacked, BoundedFloating, FreeFloating};

    /*! Alignment of children containers */
    enum LayoutType {Horizontal, Vertical, BFloat};
    enum HorizontalDirection {LeftToRight, RightToLeft};
    enum VerticalAlignment {Left, Centered, Right};

    Container (QGraphicsItem *parent = NULL);
    virtual ~Container();
    virtual void init();

    void setType(const ContainerType &t);
    ContainerType containerType();

    void setName(const QString &n);
    virtual QString getName();

    virtual QString info (const QString &prefix = "");

    void setLayoutType(const LayoutType &ltype);
    LayoutType getLayoutType();

    void setHorizontalDirection(const HorizontalDirection &hdir);
    HorizontalDirection getHorizontalDirection();

    void setVerticalAlignment(const VerticalAlignment &a);

    void addContainer(Container *c);
    Container* parentContainer();

    
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);
    virtual void reposition();

  protected:
    ContainerType type;

    QPointF ct;    // Translation of inner content due to floating children
    QString name;

    LayoutType layout;
    HorizontalDirection horizontalDirection;
    VerticalAlignment verticalAlignment;
};

#endif
