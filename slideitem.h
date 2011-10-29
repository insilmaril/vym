#ifndef SLIDEITEM_H
#define SLIDEITEM_H

#include <QList>
#include <QVariant>
#include <QVector>

class TreeItem;
class VymModel;

class SlideItem 
{
public:
    SlideItem(const QVector<QVariant> &data, SlideItem *parent = 0);
    ~SlideItem();

    SlideItem *child(int number);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    int row() const;
    void insertItem (int pos, SlideItem *si);
    void removeItem (int pos);
    bool insertChildren(int position, int count, int columns);
    bool insertColumns(int position, int columns);
    SlideItem* parent();
    bool removeChildren(int position, int count);
    bool removeColumns(int position, int columns);
    int childNumber() const;
    bool setData(int column, const QVariant &value);
    void setName (const QString &n);
    QString getName ();
    void setTreeItem (TreeItem *ti);
    int getTreeItemID();
    void setZoomFactor(const qreal &);
    qreal getZoomFactor ();
    void setRotationAngle(const qreal &);
    qreal getRotationAngle ();

private:
    QList<SlideItem*> childItems;
    QVector<QVariant> itemData;
    SlideItem *parentItem;

    int treeItemID;
    qreal zoomFactor;
    qreal rotationAngle;
};

#endif
