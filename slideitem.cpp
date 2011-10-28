#include <QStringList>

#include "treeitem.h"
#include "vymmodel.h"

#include "slideitem.h"

SlideItem::SlideItem(const QVector<QVariant> &data, SlideItem *parent)
{
    parentItem = parent;
    itemData = data;
    treeItemID=-1;
    zoomFactor=-1;
    if (data.isEmpty()) itemData.append(QVariant("empty"));
}

SlideItem::~SlideItem()
{
    qDeleteAll(childItems);
}

SlideItem *SlideItem::child(int number)
{
    return childItems.value(number);
}

int SlideItem::childCount() const
{
    return childItems.count();
}

int SlideItem::childNumber() const
{
    if (parentItem)
        return parentItem->childItems.indexOf(const_cast<SlideItem*>(this));

    return 0;
}

int SlideItem::columnCount() const
{
    return itemData.count();
}

QVariant SlideItem::data(int column) const
{
    return itemData.value(column);
}

int SlideItem::row() const
{
    if (parentItem)
        return parentItem->childItems.indexOf(const_cast<SlideItem*>(this));

    return 0;
}

void SlideItem::insertItem (int pos, SlideItem *si)
{
    if (pos<0) pos=0;
    if (pos>childItems.count() ) pos=childItems.count();
    childItems.insert (pos,si);
    si->parentItem=this;
}

void SlideItem::removeItem (int pos)
{
    // Remove, but don't delete (needed for relinking)
    if (pos<0 || pos > childItems.size()-1)
	qWarning ("TreeItem::removeChild tried to remove non existing item?!");
    else
	childItems.removeAt (pos);
}

bool SlideItem::insertChildren(int position, int count, int columns)
{
    if (position < 0 || position > childItems.size())
        return false;

    for (int row = 0; row < count; ++row) {
        QVector<QVariant> data(columns);
        SlideItem *item = new SlideItem(data, this);
        childItems.insert(position, item);
    }

    return true;
}

bool SlideItem::insertColumns(int position, int columns)
{
    if (position < 0 || position > itemData.size())
        return false;

    for (int column = 0; column < columns; ++column)
        itemData.insert(position, QVariant());

    foreach (SlideItem *child, childItems)
        child->insertColumns(position, columns);

    return true;
}

SlideItem *SlideItem::parent()
{
    return parentItem;
}

bool SlideItem::removeChildren(int position, int count)
{
    if (position < 0 || position + count > childItems.size())
        return false;

    for (int row = 0; row < count; ++row)
        delete childItems.takeAt(position);

    return true;
}

bool SlideItem::removeColumns(int position, int columns)
{
    if (position < 0 || position + columns > itemData.size())
        return false;

    for (int column = 0; column < columns; ++column)
        itemData.remove(position);

    foreach (SlideItem *child, childItems)
        child->removeColumns(position, columns);

    return true;
}

bool SlideItem::setData(int column, const QVariant &value)
{
    if (column < 0 || column >= itemData.size())
        return false;

    itemData[column] = value;
    return true;
}

void SlideItem::setTreeItem (TreeItem *ti)
{
    if (ti) treeItemID=ti->getID();
}

int SlideItem::getTreeItemID()
{
    return treeItemID;
}

void SlideItem::setZoomFactor (const qreal &zf)
{
    zoomFactor=zf;
}

qreal SlideItem::getZoomFactor()
{
    return zoomFactor;
}

void SlideItem::setRotationAngle (const qreal &zf)
{
    rotationAngle=zf;
}

qreal SlideItem::getRotationAngle()
{
    return rotationAngle;
}

