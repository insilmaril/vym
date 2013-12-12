#include <QDebug>

#include "xlinkobj.h"

#include "branchobj.h"
#include "branchitem.h"
#include "math.h"	// atan
#include "misc.h"	// max

/////////////////////////////////////////////////////////////////
// XLinkObj
/////////////////////////////////////////////////////////////////

int XLinkObj::arrowSize=6;		    // make instances
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
    delete (ctrl_p0);
    delete (ctrl_p1);
    delete (pointerEnd);
    delete (pointerBegin);
}


void XLinkObj::init () 
{
    visBranch=NULL;

    stateVis=Hidden;

    QPen pen=link->getPen();

    path=scene()->addPath (QPainterPath(), pen, Qt::NoBrush);	
    path->setZValue (dZ_XLINK);

    pointerBegin = new ArrowObj(this);
    pointerBegin->setPen( pen );
    pointerBegin->setUseFixedLength( true );
    pointerBegin->setFixedLength( 0 );

    pointerEnd = new ArrowObj(this);
    pointerEnd->setPen( pen );
    pointerEnd->setUseFixedLength( true );
    pointerEnd->setFixedLength( 0 );

    pen.setStyle (Qt::SolidLine);
    poly=scene()->addPolygon (QPolygonF(), pen, pen.color());	
    poly->setZValue (dZ_XLINK);

    // Control points for bezier path	
    qreal d=100;
    c0=QPointF (d,0);
    c1=QPointF (d,0);
    ctrl_p0=scene()->addEllipse (
	c0.x(), c0.y(),
	clickBorder*2, clickBorder*2,
	pen, pen.color() );
    ctrl_p1=scene()->addEllipse (
	c1.x(), c1.y(),
	clickBorder*2, clickBorder*2,
	pen, pen.color() );

    beginOrient=endOrient=LinkableMapObj::UndefinedOrientation;
    pen.setWidth (1);
    pen.setStyle (Qt::DashLine);

    curSelection=Unselected;

    setVisibility (true);
}

QPointF XLinkObj::getAbsPos() 
{
    switch (curSelection)
    {
	case C0:
	    return c0;
	    break;
	case C1:
	    return c1;
	    break;
	default:
	    return QPointF();
	    break;
    }
}

void XLinkObj::setStyleBegin(const QString &s)
{
    pointerBegin->setStyleEnd( s );
}

void XLinkObj::setStyleBegin(ArrowObj::OrnamentStyle os)
{
    pointerBegin->setStyleEnd( os );
}

ArrowObj::OrnamentStyle XLinkObj::getStyleBegin()
{
    return pointerBegin->getStyleEnd();
}

void XLinkObj::setStyleEnd(const QString &s)
{
    pointerEnd->setStyleEnd( s );
}

void XLinkObj::setStyleEnd(ArrowObj::OrnamentStyle os)
{
    pointerEnd->setStyleEnd( os );
}

ArrowObj::OrnamentStyle XLinkObj::getStyleEnd()
{
    return pointerEnd->getStyleEnd();
}

QPointF XLinkObj::getBeginPos()
{
    return beginPos;
}

QPointF XLinkObj::getEndPos()
{
    return endPos;
}

void XLinkObj::move (QPointF p)
{
    switch (curSelection)
    {
	case C0:
	    c0=p;
	    break;
	case C1:
	    c1=p;
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

void XLinkObj::setSelection (int cp)
{
    if (cp==0) 
	setSelection (C0);
    else if (cp==1)
	setSelection (C1);
    else
	qWarning()<<"XLO::setSelection cp="<<cp;
}

void XLinkObj::updateXLink()	
{
    QPointF a,b;
    QPolygonF pa;

    BranchObj *beginBO=NULL;
    BranchObj   *endBO=NULL;
    BranchItem *bi=link->getBeginBranch();
    if ( bi) beginBO=(BranchObj*)(bi->getLMO());
    bi=link->getEndBranch();
    if (bi) endBO=(BranchObj*)(bi->getLMO());

    if (beginBO) 
    {
	if (beginOrient != LinkableMapObj::UndefinedOrientation  &&
	    beginOrient != beginBO->getOrientation() )
	    c0.setX( -c0.x() );
	beginOrient = beginBO->getOrientation();
    }
    if (endBO)  
    {
	if (endOrient != LinkableMapObj::UndefinedOrientation  &&
	    endOrient != endBO->getOrientation() )
	    c1.setX( -c1.x() );
	endOrient = endBO->getOrientation();
    }

    if (visBranch)   
    {
	// Only one of the linked branches is visible
	// Draw arrowhead   //FIXME-1 missing shaft of arrow
	BranchObj *bo=(BranchObj*)(visBranch->getLMO());
	if (!bo) return;

	a=b=bo->getChildRefPos();


	if (bo->getOrientation()==LinkableMapObj::RightOfCenter)
	{
	    b.setX (b.x() + 2*arrowSize);
	    pa.clear();
	    pa << a <<
	          b <<
		  QPointF (b.x(), b.y() - arrowSize) <<
                  QPointF (b.x() + arrowSize, b.y()) <<
		  QPointF (b.x(), b.y() + arrowSize) <<
		  b <<
		  a;
	    poly->setPolygon(pa);
	} else
	{
	    b.setX (b.x() - 2*arrowSize);
	    pa.clear();
	    pa << a <<
	          b <<
		  QPointF (b.x(), b.y() - arrowSize) <<
                  QPointF (b.x() - arrowSize, b.y()) <<
		  QPointF (b.x(), b.y() + arrowSize) <<
		  b <<
		  a;
	    poly->setPolygon (pa);
	}   
    } else
    {
	// Both linked branches are visible

	// If a link is just drawn in the editor,
	// we have already a beginBranch
	if (beginBO) beginPos=beginBO->getChildRefPos();
	if (endBO) endPos=endBO->getChildRefPos();

        if (beginBO && endBO)
        {
            pointerBegin->move(beginPos + c0 );
            pointerBegin->setEndPoint(beginPos);

            pointerEnd->move(endPos + c1 );
            pointerEnd->setEndPoint(endPos);
        }
    }

    // Update control points for bezier
    QPainterPath p(beginPos);
    p.cubicTo ( beginPos + c0, endPos + c1, endPos);

    clickPath=p;
    path->setPath (p);	

    // Go back to create closed curve, 
    // needed for intersection check:	
    clickPath.cubicTo ( endPos + c1, beginPos + c0, beginPos);  

    QPen pen=link->getPen();
    path->setPen (pen);
    poly->setBrush (pen.color() );
    
    pen.setStyle (Qt::SolidLine);
    ctrl_p0->setRect (
	beginPos.x() + c0.x() - pointRadius/2, beginPos.y() + c0.y() - pointRadius/2,
	pointRadius, pointRadius );
    ctrl_p0->setPen (pen);
    ctrl_p0->setBrush (pen.color() );

    ctrl_p1->setRect (
	endPos.x() + c1.x() - pointRadius/2, endPos.y() + c1.y() - pointRadius/2,
	pointRadius, pointRadius );
    ctrl_p1->setPen (pen);
    ctrl_p1->setBrush (pen.color() );

    pen.setStyle (Qt::DashLine);
	
    BranchItem *bi_begin=link->getBeginBranch();
    BranchItem *bi_end  =link->getEndBranch();
    if (bi_begin && bi_end && link->getState()==Link::activeXLink)
	// FIXME-4 z-values: it may happen, that XLink is hidden below a separate rectFrame. Could lead to jumping on releasing mouse button
	// Note: with MapObj being a GraphicsItem now, maybe better reparent the xlinkobj
	//line->setZValue (dZ_DEPTH * max(bi_begin->depth(),bi_end->depth()) + dZ_XLINK); 
	path->setZValue (dZ_XLINK); 
    else	
	path->setZValue (dZ_XLINK);

    setVisibility();
}

void XLinkObj::positionBBox()
{
}

void XLinkObj::calcBBoxSize()
{
}

void XLinkObj::setVisibility (bool b)
{
    if (stateVis==FullShowControls)
    {
	ctrl_p0->show();
	ctrl_p1->show();
        pointerBegin->setUseFixedLength( false );
        pointerEnd->setUseFixedLength( false );
    } else
    {
	ctrl_p0->hide();
	ctrl_p1->hide();
        pointerBegin->setUseFixedLength( true );
        pointerBegin->setFixedLength( 0 );
        pointerEnd->setUseFixedLength( true );
        pointerEnd->setFixedLength( 0 );
    }

    MapObj::setVisibility (b);
    if (b)
    {
	if (stateVis==OnlyBegin) 
	{
	    path->hide();
	    poly->show();
            pointerBegin->hide();
            pointerEnd->hide();
	}
	else if (stateVis==OnlyEnd)
	{
	    path->hide();
	    poly->show();
            pointerBegin->hide();
            pointerEnd->hide();
	}
        else
        {
            // FIXME-0 pointers!
            path->show();
	    poly->hide();
            pointerBegin->show();
            pointerEnd->show();
        }
    }	
    else
    {
	poly->hide();
	path->hide();
        pointerBegin->hide();
        pointerEnd->hide();
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
	    if (curSelection != Unselected)
		stateVis=FullShowControls;
	    else	
		stateVis=Full;
	    setVisibility (true);
	} else
	{
	    if(!beginBO->isVisibleObj() && !endBO->isVisibleObj())
	    {	//None of the ends is visible
		visBranch=NULL;
		stateVis=Hidden;
		setVisibility (false);
	    } else
	    {	// Just one end is visible, draw a symbol that shows
		// that there is a link to a scrolled branch
		if (beginBO->isVisibleObj())
                {
                    stateVis=OnlyBegin;
		    visBranch=beginBI;
                }
		else
                {
		    visBranch=endBI;
                    stateVis=OnlyEnd;
                }
		setVisibility (true);
	    }
	}
    }
}

void XLinkObj::setC0(const QPointF &p)
{
    c0=p;
}

QPointF XLinkObj::getC0()
{
    return c0;
}

void XLinkObj::setC1(const QPointF &p)
{
    c1=p;
}

QPointF XLinkObj::getC1()
{
    return c1;
}

int XLinkObj::ctrlPointInClickBox (const QPointF &p)	
{
    CurrentSelection oldSel=curSelection;
    int ret=-1;

    QRectF r(p.x() - clickBorder, p.y() - clickBorder,
	             clickBorder *2, clickBorder*2) ;

    if (curSelection==C0 || curSelection==C1)
    {
	// If Cx selected, check both ctrl points 
	curSelection=C0;
	if (getClickPath().intersects (r) ) ret=0;
	curSelection=C1;
	if (getClickPath().intersects (r) ) ret=1;
    } 
    curSelection=oldSel;
    return ret;
}

bool XLinkObj::isInClickBox (const QPointF &p)
{
    // Return, if not visible at all...
    if (stateVis==Hidden) return false;

    CurrentSelection oldSel=curSelection;
    bool b=false;

    QRectF r(p.x() - clickBorder, p.y() - clickBorder,
	             clickBorder *2, clickBorder*2) ;

    switch (stateVis)
    {
	case FullShowControls:
	    // If Cx selected, check both ctrl points 
	    if (ctrlPointInClickBox(p) >-1) b=true;

            // Enable selecting the path, when a ctrl point is already selected
            if (!b && curSelection!=Unselected && clickPath.intersects (r)) b=true;
	    break;
	case OnlyBegin || OnlyEnd:    
	    // not selected, only partially visible
	    if (poly->boundingRect().contains(p) ) 
		b=true;
	    break;
	default:
	    // not selected, but path is fully visible
	    curSelection=Path;
	    if (getClickPath().intersects (r) ) b=true;
	    break;
    }
    curSelection=oldSel;
    return b;
}

QPainterPath XLinkObj::getClickPath()  // also needs mirroring if oriented left. Create method to generate the coordinates
{
    QPainterPath p;
    switch (curSelection)
    {
	case C0:
	    p.addEllipse (beginPos + c0,15,15);
	    return p;
	    break;
	case C1:
	    p.addEllipse (endPos + c1,15,15);
	    return p;
	    break;
	default:
	    return clickPath;
	    break;
    }
}

