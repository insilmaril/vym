#ifndef CONTAINER_H
#define CONTAINER_H

#include <QGraphicsRectItem>

#include "animpoint.h"

class BranchContainer;
class ImageContainer;
#define dZ_BBOX 0 // testing
#define dZ_SELBOX 5
#define dZ_FRAME_LOW 10
#define dZ_LINK 20
#define dZ_XLINK 40
#define dZ_FLOATIMG 70
#define dZ_ICON 80
#define dZ_TEXT 90
#define dZ_DEPTH 100
#define Z_SNOW 2000
#define Z_INIT 9999
#define Z_LINEEDIT 10000

class Container : public QGraphicsRectItem {
  friend class BranchContainer;
  friend class ImageContainer;
  public:
    /*! Type of this container */
    enum ContainerType {
        Branch,
        BranchesContainer,
        FlagCont,
        FlagRowCont,
        FloatingContent,
        Heading,
        Image,
        ImagesContainer,
        InnerContent,
        Link,
        OrnamentsContainer,
        OuterContainer,
        TmpParent,
        UndefinedType
    };

    /*! Alignment of children containers */
    enum Layout {UndefinedLayout, Horizontal, Vertical, BoundingFloats, FloatingBounded, FloatingFree};
    enum HorizontalDirection {LeftToRight, RightToLeft};
    enum HorizontalAlignment {AlignedLeft, AlignedCentered, AlignedRight};

    Container (QGraphicsItem *parent = nullptr);
    virtual ~Container();
    virtual void copy(Container*);
    virtual void init();

    void setType(const ContainerType &t);
    ContainerType containerType();

    void setName(const QString &n);
    virtual QString getName();

    virtual QString info (const QString &prefix = "");
    int containerDepth();
    QString ind();

    void setLayout(const Layout &ltype);

    Layout getLayout();
    static QString getLayoutString(const Layout &);
    static Layout getLayoutFromString(const QString &s);
    QString getLayoutString();

    bool isFloating();          //! returns true, if parent container has Floating layout
    bool hasFloatingLayout();   //! returns true, if this container has Floating layout

    void setCentralContainer(Container *);   //! Set the given container to be the central one
    bool isCentralContainer();               //! Returns true, if I am the central container
    void setMinimumWidth();         //! Minimum dimensions of container. Used for linkSpaceContainer
    qreal getMinimumWidth();

    void setHorizontalDirection(const HorizontalDirection &hdir);
    HorizontalDirection getHorizontalDirection();

    void setHorizontalAlignment(const HorizontalAlignment &a);

    virtual bool isVisibleContainer();
    virtual void setVisibility(bool);

    void addContainer(Container *c);
    Container* parentContainer();

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
    ContainerType type;

    bool visible;

    QPointF originalPos;    //! Save position before move for undo
    AnimPoint animatedPos;  //! animated position to let e.g. a branch "snap back"

    bool centralContainer;  //! true, if center of container should be in origin of a set of containers 
    QString name;

    Layout layout;

    qreal minimumWidth;

    HorizontalDirection horizontalDirection;
    HorizontalAlignment horizontalAlignment;
};

#endif
