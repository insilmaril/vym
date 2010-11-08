#ifndef FINDRESULTMODEL_H
#define FINDRESULTMODEL_H

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QTextDocument>
#include <QVariant>

class FindResultItem;
class TreeItem;

class FindResultModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    FindResultModel( QObject *parent = 0);
    ~FindResultModel();
    void clear();

    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const;

    QModelIndex index (FindResultItem *fri);
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    Qt::ItemFlags flags(const QModelIndex &index) const;
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole);
    bool setHeaderData(int section, Qt::Orientation orientation,
                       const QVariant &value, int role = Qt::EditRole);

    bool insertColumns(int position, int columns,
                       const QModelIndex &parent = QModelIndex());
    bool removeColumns(int position, int columns,
                       const QModelIndex &parent = QModelIndex());
    bool insertRows(int position, int rows,
                    const QModelIndex &parent = QModelIndex());
    bool removeRows(int position, int rows,
                    const QModelIndex &parent = QModelIndex());

    FindResultItem* getItem(const QModelIndex &index) const;
    FindResultItem* findTreeItem (TreeItem *ti);

    FindResultItem* addItem (TreeItem *ti);
    FindResultItem* addSubItem (FindResultItem *parent,const QString &s, TreeItem *pi, int i);

    void setSearchString( const QString &s);
    QString getSearchString();
    void setSearchFlags( QTextDocument::FindFlags f);
    QTextDocument::FindFlags getSearchFlags();

private:

    FindResultItem *rootItem;

    QString searchString;
    QTextDocument::FindFlags searchFlags;
};

#endif
