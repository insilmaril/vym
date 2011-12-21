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
    delete systemFlags;
    delete standardFlags;
    delete frame;
}


void OrnamentedObj::init ()
{
    heading = new HeadingObj(this);
    heading->setTreeItem (treeItem);
    heading->move (absPos.x(), absPos.y());

    systemFlags=new FlagRowObj(this);
    standardFlags=new FlagRowObj(this);

    frame = new FrameObj (this);
    frame->setTreeItem (treeItem);

    angle=0;
}

void OrnamentedObj::copy (OrnamentedObj* other)
{
    LinkableMapObj::copy(other);
    heading->copy(other->heading);
    setColor   (other->heading->getColor());	

    systemFlags->copy (other->systemFlags);
    standardFlags->copy (other->standardFlags);

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
    heading->setRotation(a); // FIXME-2 duplicated code...
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

void OrnamentedObj::positionContents()	//FIXME-3 called multiple times for each object after moving an image with mouse
{
    double x=absPos.x();
    double y=absPos.y();
    double dp=frame->getPadding();
    double dp2=dp/2;
    double ox=leftPad + dp;
    double oy=topPad + dp;
    
    double d=dZ_DEPTH*treeItem->depth();

//    if (debug) qDebug()<< "OO: positionContents "<<treeItem->getHeading()<<" dp="<<dp<<" absPos=="<<absPos<<" bboxTotal="<<bboxTotal<<"  ox="<<ox<<" oy="<<oy;
    // vertical align heading to bottom
    heading->setZValue (d + dZ_TEXT);
    heading->setTransformOriginPoint (
	QPointF ( ox + systemFlags->getBBox().width(),
		  oy + ornamentsBBox.height() - heading->getHeight() 
		) );
    heading->move (ox + x + systemFlags->getBBox().width(),
		   oy + y + ornamentsBBox.height() - heading->getHeight() 
		    );
    // Flags
    systemFlags-> move (ox +x , oy + y );
    systemFlags->setZValue (d + dZ_ICON);
    standardFlags->move (ox +x + heading->getWidth() + systemFlags->getBBox().width() , oy + y );
    standardFlags->setZValue (d + dZ_ICON);

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
    frame->setZValue (d + dZ_FRAME_HIGH);
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
	    bbox.x()+dp2,
	    bbox.y()+dp2,
	    bbox.width()-dp,
	    bbox.height()-dp));
}

void OrnamentedObj::move (double x, double y)
{
    MapObj::move (x,y);
    positionContents();
    updateLinkGeometry();
    requestReposition();
}

void OrnamentedObj::move (QPointF p)
{
    move (p.x(), p.y());
}   

void OrnamentedObj::moveBy (double x, double y)
{

    MapObj::moveBy (x,y);
    frame->moveBy (x,y);
    systemFlags->moveBy (x,y);
    standardFlags->moveBy (x,y);
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
	QPointF p=parObj->getChildPos();
	move (p.x()+x, p.y() +y);
    }
}

void OrnamentedObj::move2RelPos(QPointF p)
{
    move2RelPos (p.x(),p.y());
}

void OrnamentedObj::activateStandardFlag(Flag *flag)
{
    standardFlags->activate(flag);
    calcBBoxSize();
    positionBBox();
    move (absPos.x(), absPos.y() );
    forceReposition();
}

void OrnamentedObj::deactivateStandardFlag(const QString &name)
{
    standardFlags->deactivate(name);
    calcBBoxSize();
    positionBBox();
    move (absPos.x(),absPos.y() );
    forceReposition();
}


QString OrnamentedObj::getSystemFlagName(const QPointF &p) 
{
    return systemFlags->getFlagName(p);	
}

QRectF OrnamentedObj::getBBoxFlag (const QString &s)
{
    FlagObj *fo=systemFlags->findFlag (s);
    if (fo) return fo->getBBox();
    fo=standardFlags->findFlag (s);
    if (fo) return fo->getBBox();
    return QRectF();
}

