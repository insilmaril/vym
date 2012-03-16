#ifndef SLIDEITEM_H
#define SLIDEITEM_H

#include <QEasingCurve>
#include <QList>
#include <QVariant>
#include <QVector>

#include "xmlobj.h"

class TreeItem;
class SlideModel;

class SlideItem : public XMLObj 
{
public:
    SlideItem(const QVector<QVariant> &data, SlideItem *parent = 0, SlideModel *sm = 0 );
    ~SlideItem();
    SlideModel* getModel(); 
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
    void setDuration (const int &);
    int getDuration();
    void setEasingCurve (const QEasingCurve &);
    QEasingCurve getEasingCurve();
    QString saveToDir();

private:
    SlideModel *model;
    QList<SlideItem*> childItems;
    QVector<QVariant> itemData;
    SlideItem *parentItem;

    int treeItemID;
    qreal zoomFactor;
    qreal rotationAngle;
    int duration;
    QEasingCurve easingCurve;
};

#endif
