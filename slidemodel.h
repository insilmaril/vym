#ifndef SLIDEMODEL_H
#define SLIDEMODEL_H

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QTextDocument>
#include <QVariant>

class QItemSelectionModel;
class SlideItem;
class TreeItem;

class SlideModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    SlideModel( QObject *parent = 0);
    ~SlideModel();
    void clear();

    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const;

    QModelIndex index (SlideItem *fri);
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

    SlideItem* addItem (SlideItem *dst=NULL, int n=-1);
    void deleteItem (SlideItem *si);
    bool relinkItem ( SlideItem *si, SlideItem *dst, int pos);

    SlideItem* getItem(const QModelIndex &index) const;

    void setSearchString( const QString &s);
    QString getSearchString();
    void setSearchFlags( QTextDocument::FindFlags f);
    QTextDocument::FindFlags getSearchFlags();

// Selection related
public:
    void setSelectionModel(QItemSelectionModel *);
    QItemSelectionModel* getSelectionModel();
    QModelIndex getSelectedIndex();
    SlideItem* getSelectedItem();
private:
    QItemSelectionModel *selModel;

private:
    SlideItem *rootItem;

    QString searchString;
    QTextDocument::FindFlags searchFlags;
};

#endif
