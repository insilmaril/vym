#ifndef CONTAINER_H
#define CONTAINER_H

#include <QGraphicsRectItem>

#include "animpoint.h"

class BranchContainer;
class ImageContainer;

class Container : public QGraphicsRectItem {
  friend class BranchContainer;
  friend class ImageContainer;
  friend class SelectableContainer;
  public:
    /*! Type of this container */
    enum ContainerType {
        Branch,
        BranchesContainer,
        FlagCont,
        FlagRowCont,
        Frame,
        Heading,
        Image,
        ImagesContainer,
        InnerContent,
        Link,
        OrnamentsContainer,
        OuterContainer,
        Selection,
        TmpParent,
        UndefinedType
    };

    /*! Alignment of children containers */
    enum Layout {UndefinedLayout, Horizontal, Vertical, BoundingFloats, FloatingBounded, FloatingFree};
    enum HorizontalDirection {LeftToRight, RightToLeft};
    enum VerticalAlignment {AlignedLeft, AlignedCentered, AlignedRight};

    Container ();
    virtual ~Container();
    virtual void copy(Container*);
    virtual void init();

    void setContainerType(const ContainerType &t);
    ContainerType getContainerType();

    enum {Type = UserType + 1};
    int type() const override;

    void setName(const QString &n);
    virtual QString getName();

    virtual QString info (const QString &prefix = "");
    int containerDepth();
    QString ind();

    // Convenience coordinates
    QPointF leftCenter();
    QPointF rightCenter();
    QPointF topCenter();
    QPointF bottomCenter();
    QPointF center();
    QPointF bottomLeft();
    QPointF bottomRight();
    QPointF topLeft();
    QPointF topRight();

    void setLayout(const Layout &ltype);

    Layout getLayout();
    static QString getLayoutString(const Layout &);
    static Layout getLayoutFromString(const QString &s);
    QString getLayoutString();

    bool isFloating();          //! returns true, if parent container has Floating layout
    bool hasFloatingLayout();   //! returns true, if this container has Floating layout

    void setCentralContainer(Container *);   //! Set the given container to be the central one
    void setMinimumWidth();         //! Minimum dimensions of container. Used for linkSpaceContainer
    qreal getMinimumWidth();

    void setHorizontalDirection(const HorizontalDirection &hdir);
    HorizontalDirection getHorizontalDirection();

    void setVerticalAlignment(const VerticalAlignment &a);

    virtual bool isVisibleContainer();
    virtual void setVisibility(bool);

    void addContainer(Container *c);
    Container* parentContainer();
    QList <Container*> childContainers();

    /*! Save original position in current parent items coordinates before temporary relinking
     *  to tmpParentContainer while moving around
     */

    virtual void setPos(QPointF p);         //! Set position, or animated position
    virtual void setPos(qreal x, qreal y);  //! Overloaded for convenience

    void setOriginalPos();                   //! Saves current position for later restoring
    QPointF getOriginalPos();

    virtual void setAnimation(const AnimPoint &ap);
    virtual void stopAnimation();
    virtual bool animate();
    virtual bool isAnimated();

  public:
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);
    virtual void reposition();

  protected:
    ContainerType containerType;

    bool visible;
    bool overlay;

    QPointF originalPos;    //! Save position before move for undo
    AnimPoint animatedPos;  //! animated position to let e.g. a branch "snap back"

    Container* centralContainer;  //! Center of this container should be in origin of a set of containers 
    QString name;

    Layout layout;

    qreal minimumWidth;

    HorizontalDirection horizontalDirection;
    VerticalAlignment verticalAlignment;
};

#endif
