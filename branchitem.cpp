#include "branchitem.h"

#include "attributeitem.h"
#include "branchobj.h"
#include "task.h"
#include "taskmodel.h"
#include "vymmodel.h"
#include "xlink.h"
#include "xlinkitem.h"

extern TaskModel *taskModel;

//#include <QDir>

BranchItem::BranchItem(const QList<QVariant> &data, TreeItem *parent):MapItem (data,parent)
{
    //qDebug()<< "Constr. BranchItem this="<<this;

    // Set type if parent is known yet 
    // if not, type is set in insertBranch or TreeItem::appendChild
    if (parent==rootItem)
	setType (MapCenter);
    else
	setType (Branch);

    scrolled=false;
    tmpUnscrolled=false;

    includeImagesVer=false;
    includeImagesHor=false;
    includeChildren=false;
     
    lastSelectedBranchNum=-1;
    lastSelectedBranchNumAlt=-1;

    task=NULL;
}

BranchItem::~BranchItem()
{
    //qDebug()<< "Destr. BranchItem  this="<<this<<"  "<<getHeading();
    if (lmo) 
    {
	delete lmo;
	lmo=NULL;
    }
    clear();
}

void BranchItem::clear()
{
    if (task) taskModel->deleteTask (task);
}

void BranchItem::copy (BranchItem *other)  // FIXME-3 lacks most of data...
{
    scrolled=other->scrolled;
    tmpUnscrolled=other->tmpUnscrolled;
}

BranchItem* BranchItem::parentBranch ()
{
    return (BranchItem*) parentItem;
}

void BranchItem::insertBranch (int pos, BranchItem *branch)
{
    if (pos<0) pos=0;
    if (pos>branchCounter) pos=branchCounter;
    childItems.insert(pos+branchOffset,branch);
    branch->parentItem=this;
    branch->rootItem=rootItem;
    branch->setModel (model);
    if (parentItem==rootItem)
	setType (MapCenter);
    else
	setType (Branch);


    if (branchCounter==0)
	branchOffset=childItems.count()-1;
    branchCounter++;
}

QString BranchItem::saveToDir (const QString &tmpdir,const QString &prefix, const QPointF& offset, QList <Link*> &tmpLinks ) //FIXME-3 Check if everything is saved...
{
    // Cloudy stuff can be hidden during exports
    if (hidden) return QString();

    QString s,a;
    BranchObj *bo=(BranchObj*)lmo;

    // Update of note is usually done while unselecting a branch
    
    QString scrolledAttr;
    if (scrolled) 
	scrolledAttr=attribut ("scrolled","yes");
    else
	scrolledAttr="";

    // save area, if not scrolled   // FIXME-5 not needed if HTML is rewritten...
				    // also we should check if _any_ of parents is scrolled
    QString areaAttr;
    if (lmo && parentItem->isBranchLikeType() && !((BranchItem*)parentItem)->isScrolled() )
    {
	qreal x=lmo->getAbsPos().x();
	qreal y=lmo->getAbsPos().y();
	areaAttr=
	    attribut("x1",QString().setNum(x-offset.x())) +
	    attribut("y1",QString().setNum(y-offset.y())) +
	    attribut("x2",QString().setNum(x+lmo->width()-offset.x())) +
	    attribut("y2",QString().setNum(y+lmo->height()-offset.y()));

    } else
	areaAttr="";
    
    QString elementName;
    if (parentItem==rootItem)
	elementName="mapcenter";
    else    
	elementName="branch";

    s=beginElement (elementName
	+getMapAttr()
	+getGeneralAttr()
	+scrolledAttr 
	+getIncludeImageAttr() 
	);
    incIndent();

    // save heading
    s+=valueElement("heading", getHeading(),
	attribut ("textColor",QColor( bo->getColor()).name()));

    // Save frame  //FIXME-4 not saved if there is no LMO
    if (lmo && ((OrnamentedObj*)lmo)->getFrame()->getFrameType()!=FrameObj::NoFrame) 
	s+=((OrnamentedObj*)lmo)->getFrame()->saveToDir ();

    // Save rotation
    if (lmo && ((OrnamentedObj*)lmo)->getRotation() !=0 )
	s+=attribut ("rotation",QString().setNum (((OrnamentedObj*)lmo)->getRotation() ) );

    // save names of flags set
    s+=standardFlags.saveToDir(tmpdir,prefix,0);
    
    // Save Images
    for (int i=0; i<imageCount(); ++i)
	s+=getImageNum(i)->saveToDir (tmpdir,prefix);

    // save attributes
    for (int i=0; i<attributeCount(); ++i)
	s+=getAttributeNum(i)->getDataXML();

    // save note
    if (!note.isEmpty() )
	s+=note.saveToDir();
    
    // save task
    if (task)
	s+=task->saveToDir();

    // Save branches
    int i=0;
    TreeItem *ti=getBranchNum(i);
    while (ti)
    {
	s+=getBranchNum(i)->saveToDir(tmpdir,prefix,offset,tmpLinks);
	i++;
	ti=getBranchNum(i);
    }	

    // Mark Links for save
    for (int i=0; i<xlinkCount(); ++i)
    {
	Link *l=getXLinkItemNum (i)->getLink();
	if (l && !tmpLinks.contains (l)) tmpLinks.append (l);
    }
    decIndent();
    s+=endElement   (elementName);
    return s;
}

void BranchItem::updateVisibility() 
{
    // Needed to hide relinked branch, if parent is scrolled
    if (lmo)
    {
	if (hasScrolledParent(this) || hidden)
	    lmo->setVisibility (false);
	else	
	    lmo->setVisibility (true);
    }
}

void BranchItem::setHeadingColor (QColor color)
{
    TreeItem::setHeadingColor (color);
    if (lmo) ((BranchObj*)lmo)->setColor (color);
}

void BranchItem::updateTaskFlag()
{
    systemFlags.deactivateGroup ("system-tasks");
    if (task)
    {
	switch (task->getStatus() ) 
	{
	    case Task::NotStarted: 
		systemFlags.activate("system-task-new");
		break;
	    case Task::WIP: 
		systemFlags.activate("system-task-wip");
		break;
	    case Task::Finished: 
		systemFlags.activate("system-task-finished");
		break;
	}
    } 
}

void BranchItem::setTask(Task *t) 
{
    task=t;
    updateTaskFlag();
}

Task* BranchItem::getTask()
{
    return task;
}

void BranchItem::unScroll()
{
    if (tmpUnscrolled) resetTmpUnscroll();
    if (scrolled) toggleScroll();
}

bool BranchItem::toggleScroll()	
{
    BranchObj *bo=NULL;
    if (scrolled)
    {
	scrolled=false;
	systemFlags.deactivate("system-scrolledright");
	if (branchCounter>0)
	{
	    for (int i=0;i<branchCounter;++i)
	    {
		bo=(BranchObj*)(getBranchNum(i)->getLMO());
		if (bo) bo->setVisibility(true);
	    }
	}
    } else
    {
	scrolled=true;
	systemFlags.activate("system-scrolledright");
	if (branchCounter>0)
	{
	    for (int i=0;i<branchCounter;++i)
	    {
		bo=(BranchObj*)(getBranchNum(i)->getLMO());
		if (bo) bo->setVisibility(false);
	    }
	}
    }
    return true;
}

bool BranchItem::isScrolled()
{
    return scrolled;
}

bool BranchItem::hasScrolledParent(BranchItem *start)
{
    // Calls parents recursivly to
    // find out, if we are scrolled at all.
    // But ignore myself, just look at parents.

    if (this !=start && scrolled) return true;

    BranchItem* bi=(BranchItem*)parentItem;
    if (bi && bi!=rootItem ) 
	return bi->hasScrolledParent(start);
    else
	return false;
}

bool BranchItem::tmpUnscroll(BranchItem *start)
{
    bool result=false;

    if (!start) start=this;

    // Unscroll parent (recursivly)
    BranchItem * pi=(BranchItem*)parentItem;
    if (pi && pi->isBranchLikeType() ) result=pi->tmpUnscroll(start);
	
    // Unscroll myself
    if (start !=this && scrolled)
    {
	tmpUnscrolled=true;
	systemFlags.activate("system-tmpUnscrolledRight");
	toggleScroll();
	model->emitDataHasChanged (this); 
	result=true;
    }	
    return result;
}

bool BranchItem::resetTmpUnscroll()
{
    bool result=false;

    // Unscroll parent (recursivly)
    BranchItem * pi=(BranchItem*)parentItem;
    if (pi && pi->isBranchLikeType() ) result=pi->resetTmpUnscroll();
	
    // Unscroll myself
    if (tmpUnscrolled)
    {
	tmpUnscrolled=false;
	systemFlags.deactivate("system-tmpUnscrolledRight");
	toggleScroll();
	model->emitDataHasChanged (this);
	result=true;
    }	
    return result;
}

void BranchItem::sortChildren(bool inverse) //FIXME-4 optimize by not using moveUp/Down
{
    int childCount=branchCounter; 
    int curChildIndex;
    bool madeChanges=false;
    do
    {
	madeChanges=false;
	for(curChildIndex=1;curChildIndex<childCount;curChildIndex++)
	{
	    BranchItem* curChild =getBranchNum(curChildIndex);
	    BranchItem* prevChild=getBranchNum(curChildIndex-1);
	    if (inverse)
	    {
		if (prevChild->getHeading().compare(curChild->getHeading())<0)
		{
		    model->moveUp (curChild);
		    madeChanges=true;
		}   
	    } else  
		if (prevChild->getHeading().compare(curChild->getHeading())>0)
		{
		    model->moveUp (curChild);
		    madeChanges=true;
		}   
	} 
    }while(madeChanges);
}

void BranchItem::setIncludeImagesVer(bool b)
{
    includeImagesVer=b;
}

bool BranchItem::getIncludeImagesVer()
{
    return includeImagesVer;
}

void BranchItem::setIncludeImagesHor(bool b)
{
    includeImagesHor=b;
}

bool BranchItem::getIncludeImagesHor()
{
    return includeImagesHor;
}

QString BranchItem::getIncludeImageAttr()
{
    QString a;
    if (includeImagesVer)
	a=attribut ("incImgV","true");
    if (includeImagesHor)
	a+=attribut ("incImgH","true");
    return a;	
}

void BranchItem::setFrameIncludeChildren(bool b)
{
    includeChildren=b;	// FIXME-4 ugly: same information stored in FrameObj
    BranchObj *bo=getBranchObj();
    if (bo) bo->getFrame()->setFrameIncludeChildren(b);
}

bool BranchItem::getFrameIncludeChildren()
{
    BranchObj *bo=getBranchObj();
    if (bo) 
	return bo->getFrame()->getFrameIncludeChildren();
    else	
	return includeChildren;
}

void BranchItem::setLastSelectedBranch()
{
    int d=depth();
    if (d>=0)
    {
	if (d==1)
	    // Hack to save an additional lastSelected for mapcenters in MapEditor
	    // depending on orientation
	    // this allows to go both left and right from there
	    if (lmo && lmo->getOrientation()==LinkableMapObj::LeftOfCenter)
	    {
		((BranchItem*)parentItem)->lastSelectedBranchNumAlt=parentItem->num(this);
		return;
	    }
	((BranchItem*)parentItem)->lastSelectedBranchNum=parentItem->num(this);
    }
}

void BranchItem::setLastSelectedBranch(int i)
{
	lastSelectedBranchNum=i;
}

BranchItem* BranchItem::getLastSelectedBranch()
{
    if (lastSelectedBranchNum>=branchCounter)
	return getBranchNum (branchCounter-1);
    else    
	return getBranchNum (lastSelectedBranchNum);
}

BranchItem* BranchItem::getLastSelectedBranchAlt()
{
    return getBranchNum (lastSelectedBranchNumAlt);
}




TreeItem* BranchItem::findMapItem (QPointF p, TreeItem* excludeTI)
{
    // Search branches
    TreeItem *ti;
    for (int i=0; i<branchCount(); ++i)
    {	
	ti=getBranchNum(i)->findMapItem(p, excludeTI);
	if (ti != NULL) return ti;
    }
    

    // Search myself
    if (getBranchObj()->isInClickBox (p) && (this != excludeTI) && getBranchObj()->isVisibleObj() ) 
	return this;

    // Search images
    ImageItem *ii;
    for (int i=0; i<imageCount(); ++i )
    {
	ii=getImageNum (i);
	LinkableMapObj *mo=ii->getLMO();
	if (mo && mo->isInClickBox(p) && 
	    (ii != excludeTI) && 
	    this!= excludeTI &&
	    mo->isVisibleObj() 
	) return ii;
    }

    // Search attributes
    AttributeItem *ai;
    for (int i=0; i<attributeCount(); ++i )
    {
	ai=getAttributeNum (i);
	LinkableMapObj *mo=ai->getLMO();
	if (mo && mo->isInClickBox(p) && 
	    (ai != excludeTI) && 
	    this!= excludeTI &&
	    mo->isVisibleObj() 
	) return ai;
    }
    return NULL;
}

void BranchItem::updateStyles(const bool &keepFrame)
{
    // FIXME-5 compare also MapItem::initLMO...

    if (lmo)
    { 
	if ( parentItem != rootItem)
	    lmo->setParObj ( ((MapItem*)parentItem)->getLMO() );
	else
	    lmo->setParObj (NULL);
	((BranchObj*)lmo)->setDefAttr(BranchObj::MovedBranch,keepFrame);
    }
}

BranchObj* BranchItem::getBranchObj()	
{
    return (BranchObj*)lmo;
}

BranchObj* BranchItem::createMapObj(QGraphicsScene *scene)  // FIXME-3 maybe move this into MapEditor to get rid of scene in VymModel?
{
    BranchObj *newbo;

    if (parentItem==rootItem)
    {
	newbo=new BranchObj(NULL,this);
	lmo=newbo;
	scene->addItem (newbo);
    } else
    {
	newbo=new BranchObj( ((MapItem*)parentItem)->getLMO(),this);
	lmo=newbo;
	// Set visibility depending on parents
	if (parentItem!=rootItem && 
	    ( ((BranchItem*)parentItem)->scrolled || !((MapItem*)parentItem)->getLMO()->isVisibleObj() ) )
	    newbo->setVisibility (false);
	if (depth()==1) 
	{
	   qreal r=190;
	   qreal a= -M_PI_4 + M_PI_2 * (num()) + (M_PI_4/2)*(num()/4 % 4);
	   QPointF p (r*cos (a), r*sin (a));
	   newbo->setRelPos (p);
	}
    }
    newbo->setDefAttr(BranchObj::NewBranch);
    initLMO();

    if (!getHeading().isEmpty() ) 
    {
	newbo->updateData();	//FIXME-3 maybe better model->emitDataHasChanged()?
	newbo->setColor (headingColor);
    }	
	
    return newbo;
}

