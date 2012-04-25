#include <QDebug>

#include "xlinkobj.h"

#include "branchobj.h"
#include "branchitem.h"
#include "math.h"	// atan
#include "misc.h"	// max

/////////////////////////////////////////////////////////////////
// XLinkObj
/////////////////////////////////////////////////////////////////

int XLinkObj::arrowSize=10;		    // make instances
int XLinkObj::clickBorder=8;
int XLinkObj::pointRadius=10;

XLinkObj::XLinkObj (QGraphicsItem* parent,Link *l):MapObj(parent)
{
    //qDebug()<< "Const XLinkObj (parent,Link)";
    link=l;
    init();
}



XLinkObj::~XLinkObj ()
{
    //qDebug() << "Destr XLinkObj";
    delete (poly);
    delete (path);
    delete (ctrl_p1);
    delete (ctrl_p2);
    delete (ctrl_l1);
    delete (ctrl_l2);
}


void XLinkObj::init () 
{
    visBranch=NULL;

    QPen pen=link->getPen();

    poly=scene()->addPolygon (QPolygonF(), pen, pen.color());	
    poly->setZValue (dZ_XLINK);

    path=scene()->addPath (QPainterPath(), pen, Qt::NoBrush);	
    path->setZValue (dZ_XLINK);

    // Control points for bezier path
    qreal d=100;
    c1=QPointF (d,0);
    c2=QPointF (d,0);
    ctrl_p1=scene()->addEllipse (
	c1.x(), c1.y(),
	clickBorder*2, clickBorder*2,
	pen, pen.color() );
    ctrl_p2=scene()->addEllipse (
	c2.x(), c2.y(),
	clickBorder*2, clickBorder*2,
	pen, pen.color() );

    QPen pen2(pen);
    pen2.setWidth (1);
    pen2.setStyle (Qt::DashLine);
    ctrl_l1=scene()->addLine(0,0,0,0, pen2);
    ctrl_l2=scene()->addLine(0,0,0,0, pen2);

    curSelection=Unselected;

    setVisibility (true);
}

QPointF XLinkObj::getAbsPos() 
{
    switch (curSelection)
    {
	case C1:
	    return c1;
	    break;
	case C2:
	    return c2;
	    break;
	default:
	    return QPointF();
	    break;
    }
}

void XLinkObj::move (QPointF p)
{
    switch (curSelection)
    {
	case C1:
	    c1=p;
	    break;
	case C2:
	    c2=p;
	    break;
	default:
	    break;
    }
    updateXLink();
}

void XLinkObj::setEnd (QPointF p)
{
    endPos=p;
}

void XLinkObj::setSelection (CurrentSelection s)
{
    curSelection=s;
    setVisibility();
}

void XLinkObj::updateXLink()
{
    QPointF a,b;
    QPolygonF pa;
    if (visBranch)   
    {
	// Only one of the linked branches is visible
	// Draw arrowhead   //FIXME-1 missing shaft of arrow
	BranchObj *bo=(BranchObj*)(visBranch->getLMO());
	if (!bo) return;

	a=b=bo->getChildPos();
	if (bo->getOrientation()==LinkableMapObj::RightOfCenter)
	{
	    b.setX (b.x()+25);
	    
	    pa.clear();
	    pa<< QPointF(b.x(),b.y())<<
		QPointF(b.x() - arrowSize,b.y() - arrowSize)<<
		QPointF(b.x() - arrowSize,b.y() + arrowSize);
	    poly->setPolygon(pa);
	} else
	{
	    b.setX (b.x()-25);
	    pa.clear();
	    pa<< QPointF(b.x(),b.y())<<
		QPointF(b.x() + arrowSize,b.y() - arrowSize)<<
		QPointF(b.x() + arrowSize,b.y() + arrowSize);
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

    // Update control points for bezier
    QPainterPath p(beginPos);
    p.cubicTo ( beginPos + c1, endPos + c2, endPos);

    clickPath=p;
    path->setPath (p);	

    // Go back to create closed curve, 
    // needed for intersection check:	//FIXME-1 but problem with dotted paths
    clickPath.cubicTo ( endPos + c2, beginPos + c1, beginPos);  

    
    ctrl_p1->setRect (
	beginPos.x() + c1.x() - pointRadius/2, beginPos.y() + c1.y() - pointRadius/2,
	pointRadius, pointRadius );
    ctrl_p2->setRect (
	endPos.x() + c2.x() - pointRadius/2, endPos.y() + c2.y() - pointRadius/2,
	pointRadius, pointRadius );

    ctrl_l1->setLine ( 
	beginPos.x(), beginPos.y(),
	c1.x() + beginPos.x(), c1.y() + beginPos.y() );
    ctrl_l2->setLine ( 
	endPos.x(), endPos.y(),
	c2.x() + endPos.x(), c2.y() + endPos.y() );
	
    QPen pen=link->getPen();
    path->setPen (pen);
    poly->setBrush (pen.color() );
    BranchItem *bi_begin=link->getBeginBranch();
    BranchItem *bi_end  =link->getEndBranch();
    if (bi_begin && bi_end && link->getState()==Link::activeXLink)
	// FIXME-4 z-values: it may happen, that XLink is hidden below a separate rectFrame. Could lead to jumping on releasing mouse button
	// Note: with MapObj being a GraphicsItem now, maybe better reparent the xlinkobj
	//line->setZValue (dZ_DEPTH * max(bi_begin->depth(),bi_end->depth()) + dZ_XLINK); 
	path->setZValue (dZ_XLINK); 
    else	
	path->setZValue (dZ_XLINK);
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
    bool showControls=false;
    if (b)
    {
	path->show();
	if (curSelection != Unselected)
	    showControls=true;
	if (visBranch) 
	{
	    path->hide();
	    poly->show();
	}
	else	
	    poly->hide();
    }	
    else
    {
	poly->hide();
	path->hide();
    }	

    if (showControls)
    {
	ctrl_p1->show();
	ctrl_p2->show();
	ctrl_l1->show();
	ctrl_l2->show();
    } else
    {
	ctrl_p1->hide();
	ctrl_p2->hide();
	ctrl_l1->hide();
	ctrl_l2->hide();
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

void XLinkObj::setC1(const QPointF &p)
{
    c1=p;
}

QPointF XLinkObj::getC1()
{
    return c1;
}

void XLinkObj::setC2(const QPointF &p)
{
    c2=p;
}

QPointF XLinkObj::getC2()
{
    return c2;
}

bool XLinkObj::isInClickBox (const QPointF &p)	//FIXME-1   what about ctrl points
{
    return getClickPath().intersects (
		QRectF (p.x() - clickBorder, p.y() - clickBorder,
			clickBorder *2, clickBorder*2) );
}

QPainterPath XLinkObj::getClickPath()	//FIXME-1   what about ctrl points
{
    QPainterPath p;
    switch (curSelection)
    {
	case C1:
	    p.addEllipse (beginPos + c1,20,20);
	    return p;
	    break;
	case C2:
	    p.addEllipse (endPos + c2,20,20);
	    return p;
	    break;
	default:
	    return clickPath;
	    break;
    }
}

