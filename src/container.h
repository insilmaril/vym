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
        FloatingContent,
        InnerContent,
        Children, 
        Branch, 
        Heading
    };

    /*! Orientation relative to parent container */
    enum Orientation {
        UndefinedOrientation,
        LeftOfParent,
        RightOfParent
    };
    enum OrientationMode {
        Auto,
        Manual
    };

    /*! Alignment of children containers */
    enum LayoutType {Horizontal, Vertical, FloatingBounded, FloatingFree};
    enum HorizontalDirection {LeftToRight, RightToLeft};
    enum VerticalAlignment {AlignedLeft, AlignedCentered, AlignedRight};

    Container (QGraphicsItem *parent = nullptr);
    virtual ~Container();
    virtual void copy(Container*);
    virtual void init();

    void setType(const ContainerType &t);
    ContainerType containerType();

    void setName(const QString &n);
    virtual QString getName();

    virtual QString info (const QString &prefix = "");

    void setOrientationMode(const OrientationMode &);
    OrientationMode getOrientationMode();

    void setOrientation(const Orientation &);
    Orientation getOrientation();

    virtual void setLayoutType(const LayoutType &ltype);
    LayoutType getLayoutType();
    bool isFloating();          //! returns true, if parent container has Floating layout
    bool hasFloatingLayout();   //! returns true, if this container has Floating layout

    void setHorizontalDirection(const HorizontalDirection &hdir);
    HorizontalDirection getHorizontalDirection();

    void setVerticalAlignment(const VerticalAlignment &a);


    void addContainer(Container *c);
    Container* parentContainer();

    void setOrgPos();
    QPointF orgPos();
    
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);
    virtual void reposition();

  protected:
    ContainerType type;

    QPointF originalPos;    //! Save position before move for undo
    QString name;

    OrientationMode orientationMode;
    Orientation orientation;

    LayoutType layout;
    HorizontalDirection horizontalDirection;
    VerticalAlignment verticalAlignment;
};

#endif
