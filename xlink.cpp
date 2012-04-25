#include <QDebug>

#include "xlink.h"

#include "branchitem.h"
#include "misc.h"
#include "vymmodel.h"
#include "xlinkitem.h"
#include "xlinkobj.h"

/////////////////////////////////////////////////////////////////
// Link
/////////////////////////////////////////////////////////////////

Link::Link (VymModel *m)
{
    //qDebug() << "Const Link () this="<<this;
    model=m;
    init();
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

    type=Linear;
    pen=model->getMapDefXLinkPen();
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

void Link::setPen (const QPen &p)
{
    pen=p;
}

QPen Link::getPen ()
{
    return pen;
}

void Link::setLinkType (const QString &s)
{
    if (s=="Linear")
	type=Linear;
    else if (s=="Bezier")	
	type=Bezier;
    else
	qWarning()<<"Link::setLinkType  Unknown type: "<<s;
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
	    QString colAttr=attribut ("color",pen.color().name());
	    QString widAttr=attribut ("width",QString().setNum(pen.width(),10));
	    QString styAttr=attribut ("penstyle",penStyleToString (pen.style()));
	    QString ctrlAttr;
	    QString typeAttr;
	    switch (type)
	    {
		case Linear: 
		    typeAttr=attribut("type","Linear"); 
		    break;
		case Bezier: 
		    typeAttr=attribut("type","Bezier"); 
		    if (xlo)
		    {
			ctrlAttr +=attribut ("c1",pointToString (xlo->getC1() ) );
			ctrlAttr +=attribut ("c2",pointToString (xlo->getC2() ) );
		    }
		    break;
	    }
	    QString begSelAttr=attribut ("beginID",model->getSelectString(beginBranch));
	    QString endSelAttr=attribut ("endID",  model->getSelectString(endBranch));
	    s=singleElement ("xlink", 
		colAttr 
		+widAttr 
		+styAttr 
		+typeAttr 
		+ctrlAttr
		+begSelAttr 
		+endSelAttr);
	}
    }
    return s;
}

XLinkObj* Link::getXLinkObj()
{
    return xlo;
}

XLinkObj* Link::createMapObj()  
{
    if (!xlo) xlo=new XLinkObj (beginBranch->getLMO(),this);  
    xlo->setVisibility();
    return xlo;
}

MapObj* Link::getMO()
{
    return xlo;
}

