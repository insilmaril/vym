#include <QtGui>

#include "findresultitem.h"
#include "findresultmodel.h"
#include "settings.h"
#include "treeitem.h"

extern Settings settings;

FindResultModel::FindResultModel( QObject *parent)
    : QAbstractItemModel(parent)
{
    QVector<QVariant> rootData;
    rootData << "Heading";
    rootItem = new FindResultItem(rootData);
    showParentsLevel = ("/satellite/findResults/showParentsLevel", 1);
}

FindResultModel::~FindResultModel()
{
    delete rootItem;
}

void FindResultModel::clear()
{
    if (rootItem->childCount()>0)
	removeRows (0,rowCount (QModelIndex ()));
}

int FindResultModel::columnCount(const QModelIndex & /* parent */) const
{
    return rootItem->columnCount();
}

QVariant FindResultModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();

    FindResultItem *item = getItem(index);

    return item->data(index.column());
}

Qt::ItemFlags FindResultModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant FindResultModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem->data(section);

    return QVariant();
}

QModelIndex FindResultModel::index (FindResultItem *fri)
{
    if (!fri->parent())
	return QModelIndex();
    else    
	return createIndex (fri->row(),0,fri);
}


QModelIndex FindResultModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid() && parent.column() != 0)
        return QModelIndex();

    FindResultItem *parentItem = getItem(parent);

    FindResultItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

bool FindResultModel::insertColumns(int position, int columns, const QModelIndex &parent)
{
    bool success;

    beginInsertColumns(parent, position, position + columns - 1);
    success = rootItem->insertColumns(position, columns);
    endInsertColumns();

    return success;
}

bool FindResultModel::insertRows(int position, int rows, const QModelIndex &parent)
{
    FindResultItem *parentItem = getItem(parent);
    bool success;

    beginInsertRows(parent, position, position + rows - 1);
    success = parentItem->insertChildren(position, rows, rootItem->columnCount());
    endInsertRows();

    return success;
}

QModelIndex FindResultModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    FindResultItem *childItem = getItem(index);
    FindResultItem *parentItem = childItem->parent();

    if (parentItem == rootItem)
        return QModelIndex();

    return createIndex(parentItem->childNumber(), 0, parentItem);
}

bool FindResultModel::removeColumns(int position, int columns, const QModelIndex &parent)
{
    bool success;

    beginRemoveColumns(parent, position, position + columns - 1);
    success = rootItem->removeColumns(position, columns);
    endRemoveColumns();

    if (rootItem->columnCount() == 0)
        removeRows(0, rowCount());

    return success;
}

bool FindResultModel::removeRows(int position, int rows, const QModelIndex &parent)
{
    FindResultItem *parentItem = getItem(parent);
    bool success = true;

    beginRemoveRows(parent, position, position + rows - 1);
    success = parentItem->removeChildren(position, rows);
    endRemoveRows();

    return success;
}

int FindResultModel::rowCount(const QModelIndex &parent) const
{
    FindResultItem *parentItem = getItem(parent);

    return parentItem->childCount();
}

bool FindResultModel::setData(const QModelIndex &index, const QVariant &value,
                        int role)
{
    if (role != Qt::EditRole)
        return false;

    FindResultItem *item = getItem(index);
    bool result = item->setData(index.column(), value);

    if (result)
        emit dataChanged(index, index);

    return result;
}

bool FindResultModel::setHeaderData(int section, Qt::Orientation orientation,
                              const QVariant &value, int role)
{
    if (role != Qt::EditRole || orientation != Qt::Horizontal)
        return false;

    bool result = rootItem->setData(section, value);

    if (result)
        emit headerDataChanged(orientation, section, section);

    return result;
}

FindResultItem* FindResultModel::getItem(const QModelIndex &index) const
{
    if (index.isValid()) {
        FindResultItem *item = static_cast<FindResultItem*>(index.internalPointer());
        if (item) return item;
    }
    return rootItem;
}

FindResultItem*  FindResultModel::addItem (TreeItem *ti)
{
    FindResultItem *ni=NULL;
    if (ti)
    {
	QModelIndex parix (index (rootItem));
	
	emit (layoutAboutToBeChanged() );

	int n=rowCount (parix);
	beginInsertRows (parix,n,n);
	if (rootItem->insertChildren (n,1,0) )
        {
            QString h=ti->getHeadingPlainWithParents( showParentsLevel );
            QModelIndex ix=index(n,0,QModelIndex());
            setData (ix,QVariant(h),Qt::EditRole);
            ni=getItem(ix);
            ni->setOriginal (ti);
        }
	endInsertRows ();

	emit (layoutChanged() );
    }
    return ni;
}

FindResultItem*  FindResultModel::addSubItem (FindResultItem *parent,const QString &s, TreeItem *pi, int i)
{
    FindResultItem *ni=NULL;
    if (pi && parent)
    {
	QModelIndex parix ( index (parent));
	
	emit (layoutAboutToBeChanged() );

	int n=rowCount (parix);
	beginInsertRows (parix,n,n);

	QModelIndex ix;
	if (parent->insertChildren (n,1,0))
	{
	    ix=index(n,0,parix);
	    setData (ix,QVariant(s),Qt::EditRole);
	    ni=getItem(ix);
	    ni->setOriginal (pi);
	    ni->setOriginalIndex (i);
	}
	endInsertRows ();
	emit (layoutChanged() );
    }
    return ni;
}

void FindResultModel::setSearchString( const QString &s)
{
    searchString=s;
}

QString FindResultModel::getSearchString()
{
    return searchString;
}

void FindResultModel::setSearchFlags( QTextDocument::FindFlags f)
{
    searchFlags=f;
}

QTextDocument::FindFlags FindResultModel::getSearchFlags()
{
    return searchFlags;
}

void FindResultModel::setShowParentsLevel(uint i)
{
    showParentsLevel = i;
    settings.setValue("/findResults/showParentsLevel", showParentsLevel);
}

uint FindResultModel::getShowParentsLevel()
{
    return showParentsLevel;
}

