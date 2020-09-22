#include <QDebug>

#include "branchitem.h"
#include "ornamentedobj.h"
#include "linkablemapobj.h"
#include "vymmodel.h"

extern bool debug;

/////////////////////////////////////////////////////////////////
// OrnamentedObj
/////////////////////////////////////////////////////////////////

OrnamentedObj::OrnamentedObj(QGraphicsItem *parent,TreeItem *ti) :LinkableMapObj(parent,ti)
{
    //qDebug()<< "Const OrnamentedObj (s,ti) ti="<<ti;
    treeItem=ti;
    init ();
}

OrnamentedObj::~OrnamentedObj()
{
    delete heading;
    delete systemFlagRowObj;
    delete standardFlagRowObj;
    delete frame;
}


void OrnamentedObj::init ()
{
    heading = new HeadingObj(this);
    heading->setTreeItem (treeItem);
    heading->move (absPos.x(), absPos.y());

    systemFlagRowObj=new FlagRowObj(this);
    standardFlagRowObj=new FlagRowObj(this);

    frame = new FrameObj (this);
    frame->setTreeItem (treeItem);

    angle=0;
}

void OrnamentedObj::copy (OrnamentedObj* other)
{
    LinkableMapObj::copy(other);
    heading->copy(other->heading);
    setColor   (other->heading->getColor());	

    systemFlagRowObj->copy (other->systemFlagRowObj);
    standardFlagRowObj->copy (other->standardFlagRowObj);

    ornamentsBBox=other->ornamentsBBox;
}

void OrnamentedObj::setLinkColor()
{
    VymModel *model=treeItem->getModel();
    if (!model) return;
    if (model->getMapLinkColorHint()==HeadingColor)
	LinkableMapObj::setLinkColor (heading->getColor());
    else    
	LinkableMapObj::setLinkColor (model->getMapDefLinkColor());
}

void OrnamentedObj::setColor (QColor col)
{
    heading->setColor(col);
    setLinkColor();
}

QColor OrnamentedObj::getColor ()
{
    return heading->getColor();
}

QRectF OrnamentedObj::getBBoxHeading()
{
    return heading->getBBox();
}

void OrnamentedObj::setRotation (const qreal &a)
{
    MapObj::setRotation (a);
    heading->setRotation(a); // FIXME-4 duplicated code...
}

FrameObj* OrnamentedObj::getFrame()
{
    return frame;
}

FrameObj::FrameType OrnamentedObj::getFrameType()
{
    return frame->getFrameType();
}

QString OrnamentedObj::getFrameTypeName()
{
    return frame->getFrameTypeName();
}

void OrnamentedObj::setFrameType(const FrameObj::FrameType &t)
{
    frame->setFrameType(t);
    if (t == FrameObj::NoFrame)
    {
	linkpos=LinkableMapObj::Bottom;
	useBottomline=true;
    } else  
    {
	linkpos=LinkableMapObj::Middle;
	useBottomline=false;
    }
    updateVisibility();	// Update bottomline
    calcBBoxSize();
    positionBBox();
    requestReposition();
}

void OrnamentedObj::setFrameType(const QString &s)
{
    setFrameType(frame->getFrameType (s));
}

void OrnamentedObj::setFramePadding (const int &i)
{
    frame->setPadding (i);
    calcBBoxSize();
    positionBBox();
    requestReposition();
}

int OrnamentedObj::getFramePadding ()
{
    return frame->getPadding();
}

void OrnamentedObj::setFrameBorderWidth (const int &i)
{
    frame->setBorderWidth(i);
    calcBBoxSize();
    positionBBox();
    requestReposition();
}

int OrnamentedObj::getFrameBorderWidth()
{
    return frame->getBorderWidth();
}

void OrnamentedObj::setFramePenColor(QColor col)
{
    frame->setPenColor (col);
}

QColor OrnamentedObj::getFramePenColor()
{
    return frame->getPenColor ();
}

void OrnamentedObj::setFrameBrushColor(QColor col)
{
    frame->setBrushColor (col);
}

QColor OrnamentedObj::getFrameBrushColor()
{
    return frame->getBrushColor ();
}

void OrnamentedObj::setFrameIncludeChildren(bool b)
{
    calcBBoxSizeWithChildren();
    frame->setFrameIncludeChildren (b);
    requestReposition();
}

bool OrnamentedObj::getFrameIncludeChildren()
{
    return frame->getFrameIncludeChildren ();
}

QRectF OrnamentedObj::getOrnamentsBBox()
{
    return ornamentsBBox;
}

void OrnamentedObj::positionContents()
{
    double x=absPos.x();
    double y=absPos.y();
    double dp=frame->getTotalPadding();
    double dp2=dp/2;
    double ox=leftPad + dp;
    double oy=topPad  + dp;
    
    // vertical align heading to bottom
    heading->setZValue (dZ_TEXT);
    heading->setTransformOriginPoint (
	QPointF ( ox + systemFlagRowObj->getBBox().width(),
		  oy + ornamentsBBox.height() - heading->getHeight() 
		) );
    heading->move (ox + x + systemFlagRowObj->getBBox().width(),
		   oy + y + ornamentsBBox.height() - heading->getHeight() 
		    );
    // Flags
    systemFlagRowObj-> move (ox +x , oy + y );
    systemFlagRowObj->setZValue (dZ_ICON);
    standardFlagRowObj->move (ox +x + heading->getWidth() + systemFlagRowObj->getBBox().width() , oy + y );
    standardFlagRowObj->setZValue (dZ_ICON);

    ornamentsBBox.moveTopLeft ( QPointF (ox+x,oy+y));
    clickPoly=QPolygonF (ornamentsBBox);

    // Update bboxTotal coordinate (size set already)
    if (orientation==LinkableMapObj::LeftOfCenter )
	bboxTotal.setRect (
	    bbox.x()+(bbox.width() - bboxTotal.width()) , 
	    bbox.y()+bbox.height()/2 - bboxTotal.height()/2,
	    bboxTotal.width(),
	    bboxTotal.height());
    else
	bboxTotal.setRect (
	    bbox.x(), 
	    bbox.y()+bbox.height()/2 - bboxTotal.height()/2,
	    bboxTotal.width(),
	    bboxTotal.height());

    // Update frame
    dp=frame->getXPadding();
    dp2=dp / 2;
    frame->setZValue (dZ_FRAME_LOW);
    if (treeItem && 
	treeItem->isBranchLikeType() && 
	((BranchItem*)treeItem)->getFrameIncludeChildren() 
	)
	frame->setRect( QRectF(
	    bboxTotal.x()+dp2,
	    bboxTotal.y()+dp2,
	    bboxTotal.width()-dp,
	    bboxTotal.height()-dp));
     else
	frame->setRect( QRectF(
	    bbox.x() + dp,
	    bbox.y() + dp,
	    bbox.width() - 2 * dp,
	    bbox.height() - 2 * dp));
}

void OrnamentedObj::move (double x, double y)
{
    MapObj::move (x,y);
    positionBBox();
}

void OrnamentedObj::move (QPointF p)
{
    move (p.x(), p.y());
}   

void OrnamentedObj::moveBy (double x, double y)
{
    MapObj::moveBy (x,y);
    frame->moveBy (x,y);
    systemFlagRowObj->moveBy (x,y);
    standardFlagRowObj->moveBy (x,y);
    heading->moveBy (x,y);
    updateLinkGeometry();
    requestReposition();
}

void OrnamentedObj::moveBy (QPointF p)
{
    moveBy (p.x(), p.y());
}   

void OrnamentedObj::move2RelPos(double x, double y)
{
    setRelPos (QPointF(x,y));
    if (parObj)
    {
	QPointF p=parObj->getChildRefPos();
	move (p.x() + x, p.y() + y);
    }
}

void OrnamentedObj::move2RelPos(QPointF p)
{
    move2RelPos (p.x(), p.y());
}

QUuid OrnamentedObj::findSystemFlagUidByPos(const QPointF &p) 
{
    return systemFlagRowObj->findFlagUidByPos(p);	
}

QRectF OrnamentedObj::getBBoxSystemFlag (const QString &s)    
{
    FlagObj *fo = systemFlagRowObj->findFlagObj (s);
    if (fo) return fo->getBBox();

    return QRectF();
}

