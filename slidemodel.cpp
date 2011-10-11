#include "slideitem.h"

#include "slidemodel.h"

SlideModel::SlideModel( QObject *parent)
    : QAbstractItemModel(parent)
{
    QVector<QVariant> rootData;
    rootData << "Slide";
    rootItem = new SlideItem(rootData);
}

SlideModel::~SlideModel()
{
    delete rootItem;
}

void SlideModel::clear()
{
    if (rootItem->childCount()>0)
	removeRows (0,rowCount (QModelIndex ()));
}

int SlideModel::columnCount(const QModelIndex & /* parent */) const
{
    return rootItem->columnCount();
}

QVariant SlideModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();

    SlideItem *item = getItem(index);

    return item->data(index.column());
}

Qt::ItemFlags SlideModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant SlideModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem->data(section);

    return QVariant();
}

QModelIndex SlideModel::index (SlideItem *fri)
{
    if (!fri->parent())
	return QModelIndex();
    else    
	return createIndex (fri->row(),0,fri);
}


QModelIndex SlideModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid() && parent.column() != 0)
        return QModelIndex();

    SlideItem *parentItem = getItem(parent);

    SlideItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

bool SlideModel::insertColumns(int position, int columns, const QModelIndex &parent)
{
    bool success;

    beginInsertColumns(parent, position, position + columns - 1);
    success = rootItem->insertColumns(position, columns);
    endInsertColumns();

    return success;
}

bool SlideModel::insertRows(int position, int rows, const QModelIndex &parent)
{
    SlideItem *parentItem = getItem(parent);
    bool success;

    beginInsertRows(parent, position, position + rows - 1);
    success = parentItem->insertChildren(position, rows, rootItem->columnCount());
    endInsertRows();

    return success;
}

QModelIndex SlideModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    SlideItem *childItem = getItem(index);
    SlideItem *parentItem = childItem->parent();

    if (parentItem == rootItem)
        return QModelIndex();

    return createIndex(parentItem->childNumber(), 0, parentItem);
}

bool SlideModel::removeColumns(int position, int columns, const QModelIndex &parent)
{
    bool success;

    beginRemoveColumns(parent, position, position + columns - 1);
    success = rootItem->removeColumns(position, columns);
    endRemoveColumns();

    if (rootItem->columnCount() == 0)
        removeRows(0, rowCount());

    return success;
}

bool SlideModel::removeRows(int position, int rows, const QModelIndex &parent)
{
    SlideItem *parentItem = getItem(parent);
    bool success = true;

    beginRemoveRows(parent, position, position + rows - 1);
    success = parentItem->removeChildren(position, rows);
    endRemoveRows();

    return success;
}

int SlideModel::rowCount(const QModelIndex &parent) const
{
    SlideItem *parentItem = getItem(parent);

    return parentItem->childCount();
}

bool SlideModel::setData(const QModelIndex &index, const QVariant &value,
                        int role)
{
    if (role != Qt::EditRole)
        return false;

    SlideItem *item = getItem(index);
    bool result = item->setData(index.column(), value);

    if (result)
        emit dataChanged(index, index);

    return result;
}

bool SlideModel::setHeaderData(int section, Qt::Orientation orientation,
                              const QVariant &value, int role)
{
    if (role != Qt::EditRole || orientation != Qt::Horizontal)
        return false;

    bool result = rootItem->setData(section, value);

    if (result)
        emit headerDataChanged(orientation, section, section);

    return result;
}

SlideItem* SlideModel::getItem(const QModelIndex &index) const
{
    if (index.isValid()) {
        SlideItem *item = static_cast<SlideItem*>(index.internalPointer());
        if (item) return item;
    }
    return rootItem;
}

SlideItem*  SlideModel::addItem (const QString &s)
{
    SlideItem *ni=NULL;

    QModelIndex parix (index (rootItem));
    
    emit (layoutAboutToBeChanged() );

    int n=rowCount (parix);
    beginInsertRows (parix,n,n);
    if (rootItem->insertChildren (n,1,0) )
    {
	QModelIndex ix=index(n,0,QModelIndex());
	setData (ix,QVariant(s),Qt::EditRole);
	ni=getItem(ix);
	//ni->setOriginal (ti);
    }
    endInsertRows ();

    emit (layoutChanged() );
    
    return ni;
}

/*
SlideItem*  SlideModel::addSubItem (SlideItem *parent,const QString &s, TreeItem *pi, int i)
{
    SlideItem *ni=NULL;
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
	    ni->setTreeItem (pi);
	    //ni->setOriginalIndex (i);
	}
	endInsertRows ();
	emit (layoutChanged() );
    }
    return ni;
}
*/

void SlideModel::setSearchString( const QString &s)
{
    searchString=s;
}

QString SlideModel::getSearchString()
{
    return searchString;
}

void SlideModel::setSearchFlags( QTextDocument::FindFlags f)
{
    searchFlags=f;
}

QTextDocument::FindFlags SlideModel::getSearchFlags()
{
    return searchFlags;
}

