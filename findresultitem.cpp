#include <QStringList>

#include "findresultitem.h"
#include "treeitem.h"
#include "vymmodel.h"

FindResultItem::FindResultItem(const QVector<QVariant> &data, FindResultItem *parent)
{
    parentItem = parent;
    itemData = data;
    orgID=-1;
    orgIndex=-1;
    orgModel=NULL;
    if (data.isEmpty()) itemData.append(QVariant("empty"));
}

FindResultItem::~FindResultItem()
{
    qDeleteAll(childItems);
}

FindResultItem *FindResultItem::child(int number)
{
    return childItems.value(number);
}

int FindResultItem::childCount() const
{
    return childItems.count();
}

int FindResultItem::childNumber() const
{
    if (parentItem)
        return parentItem->childItems.indexOf(const_cast<FindResultItem*>(this));

    return 0;
}

int FindResultItem::columnCount() const
{
    return itemData.count();
}

QVariant FindResultItem::data(int column) const
{
    return itemData.value(column);
}

int FindResultItem::row() const
{
    if (parentItem)
        return parentItem->childItems.indexOf(const_cast<FindResultItem*>(this));

    return 0;
}

bool FindResultItem::insertChildren(int position, int count, int columns)
{
    if (position < 0 || position > childItems.size())
        return false;

    for (int row = 0; row < count; ++row) {
        QVector<QVariant> data(columns);
        FindResultItem *item = new FindResultItem(data, this);
        childItems.insert(position, item);
    }

    return true;
}

bool FindResultItem::insertColumns(int position, int columns)
{
    if (position < 0 || position > itemData.size())
        return false;

    for (int column = 0; column < columns; ++column)
        itemData.insert(position, QVariant());

    foreach (FindResultItem *child, childItems)
        child->insertColumns(position, columns);

    return true;
}

FindResultItem *FindResultItem::parent()
{
    return parentItem;
}

bool FindResultItem::removeChildren(int position, int count)
{
    if (position < 0 || position + count > childItems.size())
        return false;

    for (int row = 0; row < count; ++row)
        delete childItems.takeAt(position);

    return true;
}

bool FindResultItem::removeColumns(int position, int columns)
{
    if (position < 0 || position + columns > itemData.size())
        return false;

    for (int column = 0; column < columns; ++column)
        itemData.remove(position);

    foreach (FindResultItem *child, childItems)
        child->removeColumns(position, columns);

    return true;
}

bool FindResultItem::setData(int column, const QVariant &value)
{
    if (column < 0 || column >= itemData.size())
        return false;

    itemData[column] = value;
    return true;
}

void FindResultItem::setOriginal (TreeItem *ti)
{
    orgModel=ti->getModel();
    orgID=ti->getID();
}

int FindResultItem::getOriginalID()
{
    return orgID;
}

void FindResultItem::setOriginalIndex(int i)
{
    orgIndex=i;
}

int FindResultItem::getOriginalIndex()
{
    return orgIndex;
}

VymModel* FindResultItem::getOrgModel()
{
    return orgModel;
}

