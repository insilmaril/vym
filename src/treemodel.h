#ifndef TREEMODEL_H
#define TREEMODEL_H

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>

class BranchItem;
class TreeItem;
class Link;
class LinkableMapObj;

class TreeModel : public QAbstractItemModel {
    Q_OBJECT

  public:
    TreeModel(QObject *parent = 0);
    ~TreeModel();

    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const;
    QModelIndex index(TreeItem *ti);
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    void nextBranch(BranchItem *&current, BranchItem *&previous,
                    bool deepLevelsFirst = false, BranchItem *start = NULL);

    bool removeRows(int row, int count,
                    const QModelIndex &parent = QModelIndex());

    TreeItem *getItem(const QModelIndex &index) const;
    BranchItem *getRootItem();

    virtual int xlinkCount();
    virtual Link *getXLinkNum(const int &n);

  protected:
    BranchItem *rootItem;

    QList<Link *> xlinks;
    QList<uint> deleteLaterIDs;
};

#endif
