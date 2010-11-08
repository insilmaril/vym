#include <QtGui>

#include "attributeitem.h"
#include "branchitem.h"
#include "imageitem.h"
#include "treeitem.h"
#include "treemodel.h"
#include "xlinkitem.h"

TreeModel::TreeModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    QList<QVariant> rootData;
    rootData << "Heading" << "Type";
    rootItem = new BranchItem(rootData);
}

TreeModel::~TreeModel()
{
    //qDebug()<<"Destr TreeModel  this="<<this;
    delete rootItem;
}

QVariant TreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    TreeItem *item = getItem (index);

    if (role != Qt::DisplayRole)
    {
	if (role == Qt::ForegroundRole ) 
	    return item->getHeadingColor();
	//if (role == Qt::BackgroundRole )  // does not look nice
	//  return item->getBackgroundColor();
	return QVariant();
    }	


    return item->data(index.column());
}

Qt::ItemFlags TreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant TreeModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem->data(section);

    return QVariant();
}

QModelIndex TreeModel::index (TreeItem* ti)
{
    if (!ti->parent())
	return QModelIndex();
    else    
	return createIndex (ti->row(),0,ti);
}

QModelIndex TreeModel::index(int row, int column, const QModelIndex &parent)
            const
{
    // Make sure to return invalid index for invalid values (see modeltest)
    if (row<0 || column<0) return QModelIndex();
    if (column!=0) return QModelIndex();

    TreeItem *parentItem;

    if (!parent.isValid())
    {
        parentItem = rootItem;
	/*
	cout << "TM::index()  no parent?! xxx\n";
	cout << "   row="<<row<<"  col="<<column<<endl;
	cout << "   parent.internal="<< parent.internalPointer()<<endl;
	*/
	// Somehow index is requested where parentIndex is invalid.
	// what's happening here...?
	// Check if Qt examples also return index of rootIem then...

    }	
    else
        parentItem = getItem (parent);

    TreeItem *childItem = parentItem->child(row);
    //cout << "TM::index  parentItem="<<parentItem<<"  childItem="<<childItem<<"  row="<<row<<" col="<<column<<endl;
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex TreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    //FIXME-3 cout << "TM::parent  ri="<<rootItem<< "  row="<<index.row()<<"  col="<<index.column()<<endl;
    TreeItem *ti= getItem (index);
    //cout << "            ti="<<ti<<endl;
    //cout << "               "<<ti->getHeadingStd()<<endl;
    TreeItem *parentItem = ti->parent();
    //cout << "            pi="<<parentItem<<endl;

    //cout << "TreeModel::parent  ti="<<ti<<" "<<ti->getHeading().toStdString()<<"  pi="<<parentItem<<"  "<<endl;
    if (parentItem == rootItem)
        return QModelIndex();

/*
    if (!parentItem)
        return QModelIndex();	// FIXME-3 do this to avoid segfault, but why?
				// see also my question on qt-interest in march
*/
    return createIndex(parentItem->childNumber(), 0, parentItem);
}

int TreeModel::rowCount(const QModelIndex &parent) const
{
    TreeItem *parentItem;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = getItem (parent);

    return parentItem->childCount();
}

int TreeModel::columnCount(const QModelIndex &parent) const
{
    int c;
    if (parent.isValid())
    {
        c= getItem (parent)->columnCount();
	//cout << "TM::colCount  c="<<c<<"  parent="<<getItem (parent)<<endl;	
    }
    else
    {
        c= rootItem->columnCount();
	//cout << "TM::colCount  c="<<c<<"  parent=invalid"<<endl;  
    }
    return c;
}

BranchItem* TreeModel::nextBranch (BranchItem* &current, BranchItem* &previous, bool deepLevelsFirst, BranchItem *start)    
{
    // Walk through map beginning at current with previous==0
    // Start at root, if current==NULL
    if (!current) current=(BranchItem*)rootItem;

    // Are we just beginning to walk the map?
    if (!previous)
    {
	if (!start) start=current;
	previous=current;
	current=current->getFirstBranch();
	return current;
    }
    if (deepLevelsFirst)
    {
	// Going up or down (deeper)?
	if (current->depth() > previous->depth() )
	{   
	    // Coming from above
	    // Trying  to go down deeper
	    if (current->branchCount() >0 )
	    {
		previous=current;
		current=current->getFirstBranch();
		return current;
	    }	
	    // turn around and go up again
	    BranchItem *bi=current;
	    current=previous;
	    previous=bi;
	}   

	// Coming from below
	// Trying to go down again to siblings

	BranchItem *sibling=current->getBranchNum (previous->num()+1);

	if (sibling)
	{   
	    // Found sibling of previous, go there
	    previous=current;
	    current=sibling;
	    return current;
	} 

	// If we only needed to go through subtree, we are done now
	if (start==current) return NULL;

	// Go up and try to find siblings of current
	previous=current;
	current=(BranchItem*)current->parent();

	// Check if we still can go somewhere
	if (!current) return current;
	
	while (current && current->depth() < previous->depth() )
	    current=nextBranch (current,previous,true,start);
	    
	return current;

    } else
    {
/*FIXME-3
	cout << "TM::nextBranch shallow\n"; 
	std::string ch="()"; if (current) ch=current->getHeadingStd();
	std::string ph="()"; if (previous) ph=previous->getHeadingStd();
	cout << "  cur="<<ch << " prev="<<ph<<endl;
*/

	// Try to find sibling with same depth
	BranchItem *sibling=current->parent()->getBranchNum (current->num()+1);
	if (sibling)
	{   
	    // Found sibling of previous, go there
	    previous=current;
	    current=sibling;
	    return current;
	}  else
	{   
	    // Try to find next branch with same depth or greater
	    

	    current=NULL;
	    return current;
	}


	/*
    while (ix.isValid())
    {
	TreeItem *ti=model->getItem (ix);
	cout << "  level="<<level<<"  ix=";
	if (ti) cout << ti->getHeadingStd();
	row=ix.row();
	col=ix.column();
	if (! treeEditor->isExpanded(ix))
	    cout <<"  expand!";
	else	
	    cout <<"  is expanded.";
	cout <<endl;
	ix=ix.sibling(row+1,col);
    }
    */

    }
}

bool TreeModel::removeRows ( int row, int count, const QModelIndex & parent)
{
    int last=row+count-1;
    TreeItem *pi;
    if (parent.isValid())
	pi=getItem (parent);
    else
	pi=rootItem;
    TreeItem *ti;

    //cout << "TM::removeRows  pi="<<pi<<"  row="<<row<<"  count="<<count<<endl;
    for (int i=row; i<=last; i++)
    {
	ti=pi->getChildNum (row);
	pi->removeChild (row);	// does not delete object!
	delete ti;
    }
    return true;
}

TreeItem *TreeModel::getItem(const QModelIndex &index) const
{
    if (index.isValid()) {
	TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
        if (item) return item;
    }
    return NULL;
}

BranchItem *TreeModel::getRootItem()
{
    return rootItem;
}

