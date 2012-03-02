#ifndef SLIDEMODEL_H
#define SLIDEMODEL_H

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QTextDocument>
#include <QVariant>

#include "xmlobj.h"

class QItemSelectionModel;
class SlideItem;
class TreeItem;
class VymModel;

class SlideModel : public QAbstractItemModel, XMLObj
{
    Q_OBJECT

public:
    SlideModel( VymModel *vm);
    ~SlideModel();
    void clear();
    
    VymModel* getVymModel();
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const;

    QModelIndex index (SlideItem *fri);
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;

    int count();
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

    SlideItem* addSlide ( SlideItem *dst=NULL, int n=-1);
    void deleteSlide (SlideItem *si);
    bool relinkSlide (SlideItem *si, SlideItem *dst, int pos);

    SlideItem* getItem (const QModelIndex &index) const;
    SlideItem* getSlide (int n); 

    QString saveToDir ();

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
    VymModel *vymModel;	// needed for saveToDir

private:
    SlideItem *rootItem;

    QString searchString;
    QTextDocument::FindFlags searchFlags;
};

#endif
