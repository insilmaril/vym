#include <QDebug>

#include "xlinkobj.h"

#include "branchobj.h"
#include "branchitem.h"
#include "misc.h"	// max

/////////////////////////////////////////////////////////////////
// XLinkObj
/////////////////////////////////////////////////////////////////

int XLinkObj::arrowSize=10;		    // make instances

XLinkObj::XLinkObj (QGraphicsScene* scene,Link *l):MapObj(scene)
{
    //qDebug()<< "Const XLinkObj (s,Link)";
    link=l;
    init();
}



XLinkObj::~XLinkObj ()
{
    //qDebug() << "Destr XLinkObj";
    delete (line);
    delete (poly);
}


void XLinkObj::init () 
{
    visBranch=NULL;

    pen.setColor ( link->getColor() );
    pen.setWidth ( link->getWidth() );
    pen.setCapStyle (  Qt::RoundCap );
    line=scene->addLine(QLineF(1,1,1,1),pen);
    line->setZValue (dZ_LINK);
    poly=scene->addPolygon(QPolygonF(),pen, link->getColor());
    poly->setZValue (dZ_LINK);
    setVisibility (true);
}

void XLinkObj::setEnd (QPointF p)
{
    endPos=p;
}


void XLinkObj::updateXLink()
{
    QPointF a,b;
    QPolygonF pa;
    if (visBranch)   
    {
	// Only one of the linked branches is visible
	BranchObj *bo=(BranchObj*)(visBranch->getLMO());
	if (!bo) return;

	a=b=bo->getChildPos();
	if (bo->getOrientation()==LinkableMapObj::RightOfCenter)
	{
	    b.setX (b.x()+25);
	    
	    pa.clear();
	    pa<< QPointF(b.x(),b.y())<<
		QPointF(b.x()-arrowSize,b.y()-arrowSize)<<
		QPointF(b.x()-arrowSize,b.y()+arrowSize);
	    poly->setPolygon(pa);
	} else
	{
	    b.setX (b.x()-25);
	    pa.clear();
	    pa<< QPointF(b.x(),b.y())<<
		QPointF(b.x()+arrowSize,b.y()-arrowSize)<<
		QPointF(b.x()+arrowSize,b.y()+arrowSize);
	    poly->setPolygon (pa);
	}   
    } else
    {
	// Both linked branches are visible
	BranchItem *bi=link->getBeginBranch();
	if ( bi)
	{
	    // If a link is just drawn in the editor,
	    // we have already a beginBranch
	    BranchObj *bo=(BranchObj*)(bi->getLMO());
	    if (bo) 
		a=bo->getChildPos();
	    else 
		return;	
	}   
	else
	    // This shouldn't be reached normally...
	    a=beginPos;

	bi=link->getEndBranch();
	if (bi)
	{
	    BranchObj *bo=(BranchObj*)(bi->getLMO());
	    if (bo) 
		b=bo->getChildPos();
	    else 
		return;	
	}
	else
	    b=endPos;
    }

    beginPos=a;
    endPos=b;
    pen.setColor ( link->getColor() );
    pen.setWidth ( link->getWidth() );
    poly->setBrush (link->getColor() );
    line->setPen (pen);
    line->setLine(a.x(), a.y(), b.x(), b.y());
    BranchItem *bi_begin=link->getBeginBranch();
    BranchItem *bi_end  =link->getEndBranch();
    if (bi_begin && bi_end)
	line->setZValue (dZ_DEPTH * max(bi_begin->depth(),bi_end->depth()) + dZ_XLINK); 
}

void XLinkObj::positionBBox()
{
}

void XLinkObj::calcBBoxSize()
{
}

void XLinkObj::setVisibility (bool b)
{
    MapObj::setVisibility (b);
    if (b)
    {
	line->show();
	if (visBranch) 
	    poly->show();
	else	
	    poly->hide();
    }	
    else
    {
	line->hide();
	poly->hide();
    }	
}

void XLinkObj::setVisibility ()
{
    BranchItem* beginBI=link->getBeginBranch();
    BranchObj* beginBO=NULL;
    if (beginBI) beginBO=(BranchObj*)(beginBI->getLMO());

    BranchObj* endBO=NULL;
    BranchItem* endBI=link->getEndBranch();
    if (endBI) endBO=(BranchObj*)(endBI->getLMO());
    if (beginBO && endBO)
    {
	if(beginBO->isVisibleObj() && endBO->isVisibleObj())
	{   // Both ends are visible
	    visBranch=NULL;
	    setVisibility (true);
	} else
	{
	    if(!beginBO->isVisibleObj() && !endBO->isVisibleObj())
	    {	//None of the ends is visible
		visBranch=NULL;
		setVisibility (false);
	    } else
	    {	// Just one end is visible, draw a symbol that shows
		// that there is a link to a scrolled branch
		if (beginBO->isVisibleObj())
		    visBranch=beginBI;
		else
		    visBranch=endBI;
		setVisibility (true);
	    }
	}
    }
}


