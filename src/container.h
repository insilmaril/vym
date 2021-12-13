#ifndef CONTAINER_H
#define CONTAINER_H

#include <QGraphicsRectItem>

class MapObj;

class Container : public QGraphicsRectItem {
  public:
    enum ContainerType {Undefined, Collection, Branch, Heading};
    enum LayoutType {Horizontal, Vertical};
    enum HorizontalAlignment {Top, Middle, Bottom}; // FIXME-2 used?
    enum HorizontalDirection {LeftToRight, RightToLeft};
    enum VerticalAlignment  {Left, Center, Right};  // FIXME-2 used?

    Container (QGraphicsItem *parent = NULL);
    virtual ~Container();
    virtual void init();

    void setContent(MapObj* mapObj);

    ContainerType containerType();

    void setLayoutType(const LayoutType &ltype);
    void setHorizontalDirection(const HorizontalDirection &hdir);

    void addContainer(Container *c);

    void reposition();

    void setName(const QString &n);
    QString getName();

  protected:
    ContainerType type;
    MapObj *contentObj; //! Content object, e.g. HeadingObj or FlagRowObj

    QString name;

    LayoutType layout;
    HorizontalDirection horizontalDirection;
};

#endif
