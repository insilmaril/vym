#include <QDebug>

#include "xlink.h"

#include "vymmodel.h"
#include "xlinkitem.h"
#include "xlinkobj.h"

/////////////////////////////////////////////////////////////////
// Link
/////////////////////////////////////////////////////////////////

Link::Link (VymModel *m)
{
    //qDebug() << "Const Link () this="<<this;
    init();
    model=m;
}

Link::~Link ()
{
//    qDebug()<<"* Destr Link begin this="<<this<<"  bLI="<<beginLinkItem<<"  eLI="<<endLinkItem;
    deactivate();
//    qDebug()<<"* Destr Link end   this="<<this;
}

void Link::init () 
{
    xlo=NULL;
    beginBranch=NULL;
    endBranch=NULL;
    beginLinkItem=NULL;
    endLinkItem=NULL;
    xLinkState=Link::undefinedXLink;

    color=QColor (180,180,180);
    width=1;
}

void Link::setBeginBranch (BranchItem *bi)
{
    if (bi) 
    {
	xLinkState=initXLink;
	beginBranch=bi;
    }	
}

BranchItem* Link::getBeginBranch ()
{
    return beginBranch;
}

void Link::setEndBranch (BranchItem *bi)
{
    if (bi) 
    {
	xLinkState=initXLink;
	endBranch=bi;
    }	    
}

BranchItem* Link::getEndBranch()
{
    return endBranch;
}

void Link::setEndPoint (QPointF p)
{
    if (xlo) xlo->setEnd (p);
}

void Link::setBeginLinkItem (XLinkItem *li)
{
    if (li) 
    {
	xLinkState=initXLink;
	beginLinkItem=li;
    }	
}

XLinkItem* Link::getBeginLinkItem ()
{
    return beginLinkItem;
}

void Link::setEndLinkItem (XLinkItem *li)
{
    if (li) 
    {
	xLinkState=initXLink;
	endLinkItem=li;
    }	    
}

XLinkItem* Link::getEndLinkItem()
{
    return endLinkItem;
}

XLinkItem* Link::getOtherEnd (XLinkItem *xli)
{
    if (xli==beginLinkItem) return endLinkItem;
    if (xli==endLinkItem) return beginLinkItem;
    return NULL;
}

void Link::setWidth (int w)
{
    width=w;
    if (xlo) xlo->updateXLink();
}

int Link::getWidth()
{
    return width;
}

void Link::setColor(QColor c)
{
    color=c;
    if (xlo) xlo->updateXLink();
}

QColor Link::getColor()
{
    return color;
}

bool Link::activate ()	
{
    if (beginBranch && endBranch)
    {
	
	if (beginBranch==endBranch) return false;
	xLinkState=activeXLink;
	model->updateActions();
	return true;
    } else
	return false;
}

void Link::deactivate ()    
{
    // Remove pointers from XLinkItem to Link and
    // delete XLinkObj

//    qDebug()<<"Link::deactivate ******************************";
    xLinkState=deleteXLink;
    if (beginLinkItem) beginLinkItem->setLink (NULL);
    if (endLinkItem) endLinkItem->setLink (NULL);
    if (xlo)
    {
	delete (xlo);  
	xlo=NULL;
    }
}

Link::XLinkState Link::getState()
{
    return xLinkState;
}

void Link::removeXLinkItem (XLinkItem *xli)
{
    // Only mark _one_ end for removal here!
    if (xli==beginLinkItem) beginLinkItem=NULL;
    if (xli==endLinkItem) endLinkItem=NULL;
    xLinkState=deleteXLink;
}

void Link::updateLink()
{
    if(xlo ) xlo->updateXLink();
}

QString Link::saveToDir ()
{
//    qDebug()<<"Link::saveToDir  this="<<this<<" beginBranch="<<beginBranch<<"  endBranch="<<endBranch<<"  state="<<xLinkState;
    QString s="";
    if (beginBranch && endBranch && xLinkState==activeXLink)
    {
	if (beginBranch==endBranch )
	    qWarning ("Link::saveToDir  ignored, because beginBranch==endBranch, ");
	else
	{
	    QString colAttr=attribut ("color",color.name());
	    QString widAttr=attribut ("width",QString().setNum(width,10));
	    QString begSelAttr=attribut ("beginID",model->getSelectString(beginBranch));
	    QString endSelAttr=attribut ("endID",  model->getSelectString(endBranch));
	    s=singleElement ("xlink", colAttr +widAttr +begSelAttr +endSelAttr);

	}
    }
    return s;
}

XLinkObj* Link::getXLinkObj()
{
    return xlo;
}

XLinkObj* Link::createMapObj(QGraphicsScene *scene)  
{
    if (!xlo) xlo=new XLinkObj (scene,this);  
    xlo->setVisibility();
    return xlo;
}

MapObj* Link::getMO()
{
    return xlo;
}

