#ifndef CONTAINER_H
#define CONTAINER_H

#include <QGraphicsRectItem>

class BranchContainer;
class MapObj;

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

    void setOrientation(const Orientation &);
    Orientation getOrientation();

    virtual void setLayoutType(const LayoutType &ltype);
    LayoutType getLayoutType();
    bool isFloating();          //! returns true, if parent container has Floating layout
    bool hasFloatingLayout();   //! returns true, if this container has Floating layout

    /*! Floating content can cause translating this container, this is used
     *  for InnerContainer. But mapCenters need to keep the position of
     *  InnerContainer and heading, and move the parent BranchContainer
     *  instead in opposite direction, when total bbox of
     *  floating children changes
     */
    void setMovableByFloats(bool);  

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

    Orientation orientation;

    LayoutType layout;
    bool movableByFloats;

    HorizontalDirection horizontalDirection;
    VerticalAlignment verticalAlignment;
};

#endif
