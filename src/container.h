#ifndef CONTAINER_H
#define CONTAINER_H

#include <QGraphicsRectItem>

class TreeItem;

class Container : public QGraphicsRectItem {
  public:
    Container (QGraphicsItem *parent = NULL, TreeItem *ti = NULL);
    virtual ~Container();
    virtual void init();
    virtual void copy(Container *);

    virtual void setTreeItem(TreeItem *);
    virtual TreeItem *getTreeItem() const;

    void addContainer(Container *c);

    void reposition();

    void setName(const QString &n);
    QString getName();

  protected:
    TreeItem *treeItem; //! Crossrefence to treemodel 
    QString name;

    QList <Container*> childrenList;
};

#endif
