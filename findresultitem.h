#ifndef FINDRESULTITEM_H
#define FINDRESULTITEM_H

#include <QList>
#include <QVariant>
#include <QVector>

class TreeItem;
class VymModel;

class FindResultItem
{
public:
    FindResultItem(const QVector<QVariant> &data, FindResultItem *parent = 0);
    ~FindResultItem();

    FindResultItem *child(int number);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    int row() const;
    bool insertChildren(int position, int count, int columns);
    bool insertColumns(int position, int columns);
    FindResultItem *parent();
    bool removeChildren(int position, int count);
    bool removeColumns(int position, int columns);
    int childNumber() const;
    bool setData(int column, const QVariant &value);
    void setOriginal (TreeItem *ti);
    int getOriginalID();
    void setOriginalIndex(int i);
    int getOriginalIndex ();
    VymModel* getOrgModel();

private:
    QList<FindResultItem*> childItems;
    QVector<QVariant> itemData;
    FindResultItem *parentItem;

    int orgID;
    int orgIndex;
    VymModel *orgModel;
};

#endif
