#ifndef CONTAINER_H
#define CONTAINER_H

#include <QGraphicsRectItem>

class MapObj;

class Container : public QGraphicsRectItem {
  public:
    enum ContentType {UndefinedContent, MapObject, Containers};
    enum ContainerType {Undefined, Branch, Heading};
    enum LayoutType {Horizontal, Vertical};
    enum HorizontalAlignment {Top, Middle, Bottom}; // FIXME-2 used?
    enum VerticalAlignment  {Left, Center, Right};  // FIXME-2 used?

    Container (QGraphicsItem *parent = NULL);
    virtual ~Container();
    virtual void init();
    virtual void copy(Container *);

    void setContentType(const ContentType &ctype);
    ContentType getContentType();
    void setContent(MapObj* mapObj);

    ContainerType containerType();

    void setLayoutType(const LayoutType &ltype);

    void addContainer(Container *c);

    void reposition();

    void setName(const QString &n);
    QString getName();

  protected:
    ContentType contentType;
    ContainerType type;
    MapObj *contentObj; //! Content object, e.g. HeadingObj or FlagRowObj

    QString name;

    LayoutType layout;
};

#endif
