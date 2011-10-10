#include <iostream>
#include <QStringList>

#include "attributeitem.h"
#include "branchobj.h"
#include "branchitem.h"
#include "treeitem.h"
#include "vymmodel.h"
#include "xlinkitem.h"
#include "xlinkobj.h"

using namespace std;

extern FlagRow* standardFlagsMaster;

uint TreeItem::idLast=0;    // Create instance

TreeItem::TreeItem()
{
    //cout << "Constr. TI  this="<<this<<endl;
    init();
    itemData.clear();
    rootItem=this;
    parentItem=NULL;
}

TreeItem::TreeItem(const QList<QVariant> &data, TreeItem *parent)
{
    //cout << "Constructor TreeItem this="<<this<<"  parent="<<parent<<endl;
    init();
    parentItem = parent;
    itemData = data;
    
    rootItem=this;
    if (parentItem )
	rootItem=parentItem->rootItem;
}

TreeItem::~TreeItem()
{
    //qDebug()<<"Destr TreeItem this="<<this<<"  childcount="<<childItems.count();
    /*
    qDeleteAll(childItems); // Call delete  //FIXME-3 use this instead of below
    qDebug()<<"  ok1";
    childItems.clear();	    // and clear list
    qDebug()<<"  ok2";use
    */

    //qDebug()<<"                   ="<<getHeading();
    TreeItem *ti;
    while (!childItems.isEmpty())
    {
	//qDebug()<<"    ="<<ti->getHeading();
	ti=childItems.takeFirst();
	//qDebug()<<"  ti="<<ti;
	delete ti;
    }
}


void TreeItem::init()
{
    model=NULL;

    // Assign ID  
    idLast++;
    id=idLast;

    branchOffset=0;
    branchCounter=0;

    imageOffset=0;
    imageCounter=0;

    attributeCounter=0;
    attributeOffset=0;

    xlinkCounter=0;
    xlinkOffset=0;

    target=false;

    note.setNote(""); 
    // note.setFontHint (textEditor->getFontHintDefault() );	//FIXME-3
    // isNoteInEditor=false;

    hidden=false;
    hideExport=false;

    headingColor=Qt::black;
    backgroundColor=Qt::transparent;

    standardFlags.setMasterRow (standardFlagsMaster);
}


void TreeItem::setModel (VymModel *m)
{
    model=m;
}

VymModel* TreeItem::getModel ()
{
    return model;
}

int TreeItem::getRowNumAppend (TreeItem *item)
{   
    switch (item->type)
    {
	case Attribute: return attributeOffset + attributeCounter;
	case XLink: return xlinkOffset + xlinkCounter;
	case Image: return imageOffset + imageCounter;
	case MapCenter: return branchOffset + branchCounter;
	case Branch: return branchOffset + branchCounter;
	default: return -1;
    }
}

void TreeItem::appendChild(TreeItem *item)
{
    item->parentItem=this;
    item->rootItem=rootItem;
    item->setModel (model);

    if (item->type == Attribute)
    {
	// attribute are on top of list
	childItems.insert (attributeCounter,item);
	attributeCounter++;
	xlinkOffset++;
	imageOffset++;
	branchOffset++;
    }

    if (item->type == XLink)
    {
	childItems.insert (xlinkCounter+xlinkOffset,item);
	xlinkCounter++;
	imageOffset++;
	branchOffset++;
    }

    if (item->type == Image)
    {
	childItems.insert (imageCounter+imageOffset,item);
	imageCounter++;
	branchOffset++;
    }

    if (item->isBranchLikeType())
    {
	// branches are on bottom of list
	childItems.append(item);
	branchCounter++;

	// Set correct type	//FIXME-3 DUP in constr branchitem
	if (this==rootItem)
	    item->setType(MapCenter);
	else
	    item->setType (Branch);
    }
}

void TreeItem::removeChild(int row)
{
    if (row<0 || row > childItems.size()-1)
	qWarning ("TreeItem::removeChild tried to remove non existing item?!\n");
    else
    {
	if (childItems.at(row)->type==Attribute)
	{
	    attributeCounter--;
	    xlinkOffset--;
	    imageOffset--;
	    branchOffset--;
	}   
	if (childItems.at(row)->type==XLink)
	{
	    xlinkCounter--;
	    imageOffset--;
	    branchOffset--;
	}   
	if (childItems.at(row)->type==Image)
	{
	    imageCounter--;
	    branchOffset--;
	}   
	if (childItems.at(row)->isBranchLikeType())
	    branchCounter--;

	childItems.removeAt (row);
    }
}

TreeItem *TreeItem::child(int row)
{
    return childItems.value(row);
}

int TreeItem::childCount() const
{
    return childItems.count();
}

int TreeItem::childNumber() const
{
    if (parentItem)
        return parentItem->childItems.indexOf(const_cast<TreeItem*>(this));

    return 0;
}

int TreeItem::columnCount() const
{
    return 1;
}

int TreeItem::branchCount() const
{
    return branchCounter;
}

int TreeItem::imageCount() const
{
    return imageCounter; 
}

int TreeItem::xlinkCount() const
{
    return xlinkCounter; 
}

int TreeItem::attributeCount() const 
{
    return attributeCounter; 
}

int TreeItem::row() const
{
    if (parentItem)
        return parentItem->childItems.indexOf(const_cast<TreeItem*>(this));

    cout << "TI::row() pI=NULL this="<<this<<"  ***************\n";
    return 0;
}

int TreeItem::depth() 
{
    // Rootitem d=-1
    // MapCenter d=0
    int d=-2;
    TreeItem *ti=this;
    while (ti!=NULL)
    {
	ti=ti->parent();
	d++;
    }
    return d;
}

TreeItem *TreeItem::parent()
{
    //cout << "TI::parent of "<<getHeadingStd()<<"  is "<<parentItem<<endl;
    return parentItem;
}

int TreeItem::childNum()
{
    return parentItem->childItems.indexOf (this);
}

int TreeItem::num()
{
    if (!parentItem) return -1;
    return parentItem->num (this);
}

int TreeItem::num (TreeItem *item)
{
    if (!item) return -1;
    if (!childItems.contains(item)) return -1;
    switch (item->getType())
    {
	case MapCenter: return childItems.indexOf (item) - branchOffset;
	case Branch: return childItems.indexOf (item) - branchOffset;
	case Image: return childItems.indexOf (item) - imageOffset;
	case Attribute: return childItems.indexOf (item) - attributeOffset;
	case XLink: return childItems.indexOf (item) - xlinkOffset;
	default: return -1;
    }
}
void TreeItem::setType(const Type t)
{
    type=t;
    itemData[1]=getTypeName();
}

TreeItem::Type TreeItem::getType()
{
    if (type==Branch && depth()==0) return MapCenter;	//FIXME-3 should not be necesssary
    return type;
}

bool TreeItem::isBranchLikeType() const
{
    if (type==Branch ||type==MapCenter) return true;
    return false;
}

QString TreeItem::getTypeName()
{
    switch (type)
    {
	case Undefined: return QString ("Undefined");
	case MapCenter: return QString ("MapCenter");
	case Branch: return QString ("Branch");
	case Image: return QString ("Image");
	case Attribute: return QString ("Attribute");
	case XLink: return QString ("XLink");
	default: return QString ("TreeItem::getTypeName no typename defined?!");
    }
}


QVariant TreeItem::data(int column) const
{
    return itemData.value(column);
}

void TreeItem::setHeading (const QString s)
{
    itemData[0]=s;
    //qDebug()<<"TI::setHeading this="<<this<<"  "<<s;
}

QString TreeItem::getHeading () const
{
    return itemData[0].toString();
}

std::string TreeItem::getHeadingStd () const
{
    return itemData[0].toString().toStdString();
}

#include "noteobj.h"
QString TreeItem::getHeadingPlain() const   //FIXME-3 create own TextObj instead of recreating from ASCII note every time
{
    NoteObj no(itemData[0].toString());
    QString t=no.getNoteASCII();

    // strip beginning and tailing WS
    return t.trimmed();
}

void TreeItem::setHeadingColor (QColor color)
{
    headingColor=color;
}

QColor TreeItem::getHeadingColor ()
{
    return headingColor;
}

void TreeItem::setBackgroundColor (QColor color)
{
    backgroundColor=color;
}

QColor TreeItem::getBackgroundColor() 
{
    return backgroundColor;
}

void TreeItem::setURL (const QString &u)
{
    url=u;
    if (!url.isEmpty())
    {
	if (url.contains ("bugzilla.novell.com"))
	{
	    systemFlags.activate ("system-url-bugzilla-novell");
	    if (systemFlags.isActive ("system-url"))
		systemFlags.deactivate ("system-url");
	} else
	{
	    systemFlags.activate ("system-url");
	    if (systemFlags.isActive ("system-url-bugzilla-novell"))
		systemFlags.deactivate ("system-url-bugzilla-novell");
	}
    }
    else
    {
	if (systemFlags.isActive ("system-url"))
	    systemFlags.deactivate ("system-url");
	if (systemFlags.isActive ("system-url-bugzilla-novell"))
	    systemFlags.deactivate ("system-url-bugzilla-novell");
    }
}

QString TreeItem::getURL ()
{
    return url;
}

void TreeItem::setVymLink (const QString &vl)
{
    if (!vl.isEmpty())
    {
	// We need the relative (from loading) 
	// or absolute path (from User event)
	// and build the absolute path.
	// Note: If we have relative, use path of
	// current map to build absolute path
	QDir d(vl);
	if (!d.path().startsWith ("/"))
	{
	    QString p=model->getDestPath();
	    int i=p.lastIndexOf("/",-1);
	    d.setPath(p.left(i)+"/"+vl);
	    d.makeAbsolute();
	}
	vymLink=d.path();
	systemFlags.activate("system-vymLink");
    }	
    else    
    {
	vymLink.clear();
	systemFlags.deactivate("system-vymLink");
    }	
}

QString TreeItem::getVymLink ()
{
    return vymLink;
}

void TreeItem::toggleTarget ()
{
    systemFlags.toggle ("system-target");
    target= systemFlags.isActive("system-target");
    model->emitDataHasChanged(this);	// FIXME-3 better call from VM?
}

bool TreeItem::isTarget ()
{
    return target;
}

void TreeItem::setNote(const QString &s)
{
    NoteObj n;
    n.setNote(s);
    setNoteObj (n);
}

void TreeItem::clearNote()
{
    note.clear();
    systemFlags.deactivate ("system-note");
}

void TreeItem::setNoteObj(const NoteObj &n){
    note=n;
    if (!note.isEmpty() && !systemFlags.isActive ("system-note"))
	systemFlags.activate ("system-note");
    if (note.isEmpty() && systemFlags.isActive ("system-note"))
	systemFlags.deactivate ("system-note");
}

QString TreeItem::getNote()
{
    return note.getNote();
}

bool TreeItem::hasEmptyNote()
{
    return note.isEmpty();
}

NoteObj TreeItem::getNoteObj()
{
    return note;
}

QString TreeItem::getNoteASCII(const QString &indent, const int &width)
{
    return note.getNoteASCII(indent,width);
}

QString TreeItem::getNoteASCII()
{
    return note.getNoteASCII();
}

QString TreeItem::getNoteOpenDoc()
{
    return note.getNoteOpenDoc();
}

void TreeItem::activateStandardFlag (const QString &name)
{
    standardFlags.activate (name);
    model->emitDataHasChanged(this);
}

void TreeItem::deactivateStandardFlag (const QString &name)
{
    standardFlags.deactivate (name);
    model->emitDataHasChanged(this);
}

void TreeItem::deactivateAllStandardFlags ()
{
    standardFlags.deactivateAll ();
    model->emitDataHasChanged(this);
}

void TreeItem::toggleStandardFlag(const QString &name, FlagRow *master)
{
    standardFlags.toggle (name,master);
    model->emitDataHasChanged(this);	// FIXME-3 better call from VM?
}

void TreeItem::toggleSystemFlag(const QString &name, FlagRow *master)
{
    systemFlags.toggle (name,master);
    model->emitDataHasChanged(this);	// FIXME-3 better call from VM?
}

bool TreeItem::isActiveStandardFlag (const QString &name)
{
    return standardFlags.isActive (name);
}

bool TreeItem::isActiveSystemFlag (const QString &name)
{
    return systemFlags.isActive (name);
}

QStringList TreeItem::activeStandardFlagNames ()
{
    return standardFlags.activeFlagNames();
}

FlagRow* TreeItem::getStandardFlagRow()
{
    return &standardFlags;
}

QStringList TreeItem::activeSystemFlagNames ()
{
    return systemFlags.activeFlagNames();
}

bool TreeItem::canMoveDown()
{
    switch (type)
    {
	case Undefined: return false;
	case MapCenter: 
	case Branch: 
	    if (!parentItem) return false;
	    if (parentItem->num (this) < parentItem->branchCount()-1)
		return true;
	    else
		return false;
	    break;  
	case Image: return false;
	default: return false;
    }
}

bool TreeItem::canMoveUp()
{
    switch (type)
    {
	case MapCenter: 
	case Branch: 
	    if (!parentItem) return false;
	    if (parentItem->num (this) > 0)
		return true;
	    else
		return false;
	    break;  
	default: return false;
    }
}

uint TreeItem::getID()
{
    return id;
}

TreeItem* TreeItem::getChildNum(const int &n)
{
    if (n>=0 && n<childItems.count() )
	return childItems.at(n);
    else
	return NULL;
}

BranchItem* TreeItem::getFirstBranch()
{
    if (branchCounter>0)
	return getBranchNum (0);
    else
	return NULL;
}

BranchItem* TreeItem::getLastBranch()
{
    if (branchCounter>0)
	return getBranchNum (branchCounter-1);
    else
	return NULL;
}

ImageItem* TreeItem::getFirstImage()
{
    if (imageCounter>0)
	return getImageNum (imageCounter-1);
    else
	return NULL;
}

ImageItem* TreeItem::getLastImage()
{
    if (imageCounter>0)
	return getImageNum (imageCounter-1);
    else
	return NULL;
}

BranchItem* TreeItem::getNextBranch(BranchItem *currentBranch)
{
    if (!currentBranch) return NULL;
    int n=num (currentBranch)+1;
    if (n<branchCounter)
	return getBranchNum (branchOffset + n);
    else
	return NULL;
}


BranchItem* TreeItem::getBranchNum(const int &n)
{
    if (n>=0 && n<branchCounter)
	return (BranchItem*)getChildNum (branchOffset + n);
    else
	return NULL;
}

BranchObj* TreeItem::getBranchObjNum(const int &n)
{
    if (n>=0 && n<branchCounter)
    {
	BranchItem *bi=getBranchNum(n);
	if (bi)
	{
	    BranchObj *bo=(BranchObj*)(bi->getLMO());
	    if (bo)
		return bo;
	    else
		qDebug()<<"TI::getBONum bo=NULL";
	}
    } 
    return NULL;
}

ImageItem* TreeItem::getImageNum (const int &n)
{
    if (n>=0 && n<imageCounter)
	return (ImageItem*)getChildNum (imageOffset + n);
    else
	return NULL;
}

FloatImageObj* TreeItem::getImageObjNum (const int &n)	// FIXME-5 what about SVGs later?
{
    if (imageCounter>0 )
	return (FloatImageObj*)(getImageNum(n)->getLMO());
    else
	return NULL;
}

AttributeItem* TreeItem::getAttributeNum (const int &n)
{
    if (n>=0 && n<attributeCounter)
	return (AttributeItem*)getChildNum (attributeOffset + n);
    else
	return NULL;
}

XLinkItem* TreeItem::getXLinkItemNum (const int &n) 
{
    if (n>=0 && n<xlinkCounter )
	return (XLinkItem*)getChildNum (xlinkOffset +n);
    else
	return NULL;
}


XLinkObj* TreeItem::getXLinkObjNum (const int &n)   
{
    if (xlinkCounter>0 )
    {
	XLinkItem *xli=getXLinkItemNum (n);
	if (xli)
	{
	    Link *l=xli->getLink();
	    if (l) return l->getXLinkObj();
	}
    }
    return NULL;
}


void TreeItem::setHideTmp (HideTmpMode mode)  //FIXME-3	update visibility in derived objects
{
    if (type==Image || type==Branch || type==MapCenter)
//	((ImageItem*)this)->updateVisibility();
    {
	//LinkableMapObj* lmo=((MapItem*)this)->getLMO();

	if (mode==HideExport && (hideExport || hasHiddenExportParent() ) ) // FIXME-3  try to avoid calling hasScrolledParent repeatedly

	    // Hide stuff according to hideExport flag and parents
	    hidden=true;
	else
	    // Do not hide, but still take care of scrolled status
	    hidden=false;
	updateVisibility();
	// And take care of my children
	for (int i=0; i<branchCount(); ++i)
	    getBranchNum(i)->setHideTmp (mode);	
    }
}

bool TreeItem::hasHiddenExportParent()
{
    // Calls parents recursivly to
    // find out, if we or parents are temp. hidden

    if (hidden || hideExport) return true;

    if (parentItem) 
	return parentItem->hasHiddenExportParent();
    else
	return false;
}


void TreeItem::setHideInExport(bool b) 
{
    if (type==MapCenter ||type==Branch || type==Image)
    {
	hideExport=b;
	if (b)
	    systemFlags.activate("system-hideInExport");
	else	
	    systemFlags.deactivate("system-hideInExport");
    }
}   

bool TreeItem::hideInExport()
{
    return hideExport;
}   

void TreeItem::updateVisibility()
{
    // overloaded in derived objects
}   

bool TreeItem::isHidden()
{
    return hidden;
}   

QString TreeItem::getGeneralAttr()  
{
    QString s;
    if (hideExport)
	 s+=attribut("hideInExport","true");
    if (!url.isEmpty())
	s+=attribut ("url",url);
    if (!vymLink.isEmpty())
	s+=attribut ("vymLink",convertToRel (model->getDestPath(),vymLink));	

    if (target)
	s+=attribut ("localTarget","true");
    return s;	
}


