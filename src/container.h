#ifndef CONTAINER_H
#define CONTAINER_H

#include <QGraphicsRectItem>

class TreeItem;

class Container : public QGraphicsRectItem {
  public:
    enum ContentType {Undefined, MapObj, Containers};
    enum LayoutType {Horizontal, Vertical};
    enum HorizontalAlignment {Top, Middle, Bottom};
    enum VerticalAlignment  {Left, Center, Right};

    Container (QGraphicsItem *parent = NULL, TreeItem *ti = NULL);
    virtual ~Container();
    virtual void init();
    virtual void copy(Container *);

    void setContentType(const ContentType &ctype);
    ContentType contentType();

    void setLayoutType(const LayoutType &ltype);

    virtual void setTreeItem(TreeItem *);
    virtual TreeItem *getTreeItem() const;

    void addContainer(Container *c);

    void reposition();

    void setName(const QString &n);
    QString getName();

  protected:
    ContentType contType;

    TreeItem *treeItem; //! Crossrefence to treemodel 
    QString name;

    QList <Container*> childrenList;

    LayoutType layout;

};

#endif
