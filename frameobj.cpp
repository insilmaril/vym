#include "frameobj.h"

#include <QColor>
#include <QDebug>
#include <QGraphicsScene>

#include "misc.h"  //for roof function

/////////////////////////////////////////////////////////////////
// FrameObj
/////////////////////////////////////////////////////////////////
FrameObj::FrameObj(QGraphicsItem *parent) :MapObj(parent)
{
    init ();
}

FrameObj::~FrameObj()
{
    clear();
}

void FrameObj::init()
{
    type=NoFrame;
    clear();
    borderWidth=1;
    penColor=QColor (Qt::black);
    brushColor=QColor (Qt::white);
    includeChildren=false;
}

void FrameObj::clear()
{
    switch (type)
    {
	case NoFrame:
	    break;
	case Rectangle:
	    delete rectFrame;
	    break;
	case RoundedRectangle:
	    delete pathFrame;
	    break;
	case Ellipse:
	    delete ellipseFrame;
	    break;
	case Cloud:
	    delete pathFrame;
	    break;
    }
    type=NoFrame;
    padding=0;	// No frame requires also no padding
    xsize=0;
}

void FrameObj::move(double x, double y)
{
    switch (type)
    {
	case NoFrame:
	    break;
	case Rectangle:
	    rectFrame->setPos (x,y);
	    break;
	case RoundedRectangle:
	    pathFrame->setPos (x,y);
	    break;
	case Ellipse:
	    ellipseFrame->setPos (x,y);
            break;
	case Cloud:
	    pathFrame->setPos (x,y);
	    break;
    }
}

void FrameObj::moveBy(double x, double y)
{
    MapObj::moveBy (x,y);
}

void FrameObj::positionBBox()
{
}

void FrameObj::calcBBoxSize()
{
}

void FrameObj::setRect(const QRectF &r)
{
    bbox=r;
    switch (type)
	{
	case NoFrame:
	    break;

	case Rectangle:
	    rectFrame->setRect (QRectF(bbox.x(),bbox.y(),bbox.width(),bbox.height() ));
	    break;

	case RoundedRectangle:
	{
	    QPointF tl = bbox.topLeft();
	    QPointF tr = bbox.topRight();
	    QPointF bl = bbox.bottomLeft();
	    QPointF br = bbox.bottomRight();
	    QPainterPath path;

	    qreal n = 10;
	    path.moveTo (tl.x() +n/2, tl.y());

	    // Top path
	    path.lineTo (tr.x()-n, tr.y());
	    path.arcTo  (tr.x()-n, tr.y(), n, n,90,-90);
	    path.lineTo (br.x()  , br.y()-n);
	    path.arcTo  (br.x()-n, br.y()-n, n, n,0,-90);
	    path.lineTo (bl.x()+n, br.y());
	    path.arcTo  (bl.x()  , bl.y()-n, n, n,-90,-90);
	    path.lineTo (tl.x()  , tl.y()+n);
	    path.arcTo  (tl.x()  , tl.y(), n, n,180,-90);
	    pathFrame->setPath(path);
	}
	    break;
	case Ellipse:
	    ellipseFrame->setRect (QRectF(bbox.x(),bbox.y(),bbox.width(),bbox.height() ));
            xsize = 20;//max(bbox.width(), bbox.height()) / 4;
	    break;

	case Cloud:
	    QPointF tl = bbox.topLeft();
	    QPointF tr = bbox.topRight();
	    QPointF bl = bbox.bottomLeft();
	    QPainterPath path;
	    path.moveTo (tl);

	    float w = bbox.width(); // width
	    float h = bbox.height();// height
	    int   n = w / 40;	    // number of intervalls
	    float d = w / n;	    // width of interwall

	    // Top path
	    for (float i = 0; i < n; i++)
	    {
		path.cubicTo (
		    tl.x() + i*d,     tl.y()- 100*roof ((i+0.5)/n) , 
		    tl.x() + (i+1)*d, tl.y()- 100*roof ((i+0.5)/n) , 
		    tl.x() + (i+1)*d + 20*roof ((i+1)/n), tl.y()- 50*roof((i+1)/n) );
	    }
	    // Right path
	    n = h/20;
	    d = h/n;
	    for (float i = 0; i < n; i++)
	    {
		path.cubicTo (
		    tr.x()+ 100*roof ((i+0.5)/n)        , tr.y() + i*d,
		    tr.x()+ 100*roof ((i+0.5)/n)        , tr.y() + (i+1)*d,
		    tr.x() + 60*roof ((i+1)/n)          , tr.y() + (i+1)*d );
	    }
	    n = w / 60;
	    d = w / n;
	    // Bottom path
	    for (float i = n; i > 0; i--)
	    {
		path.cubicTo (
		    bl.x() + i*d,  bl.y()+ 100*roof ((i-0.5)/n) , 
		    bl.x() + (i-1)*d,      bl.y()+ 100*roof ((i-0.5)/n) , 
		    bl.x() + (i-1)*d + 20*roof ((i-1)/n), bl.y()+ 50*roof((i-1)/n) );
	    }
	    // Left path
	    n = h / 20;
	    d = h / n;
	    for (float i = n; i > 0; i--)
	    {
		path.cubicTo (
		    tl.x()- 100*roof ((i-0.5)/n)        , tr.y() + i*d,
		    tl.x()- 100*roof ((i-0.5)/n)        , tr.y() + (i-1)*d,
		    tl.x()-  60*roof ((i-1)/n)          , tr.y() + (i-1)*d );
	    }
	    pathFrame->setPath(path);
            xsize = 50;
	    break;
    }
}

void FrameObj::setPadding (const int &i)
{
    padding = i;
}

int FrameObj::getPadding()
{
    if (type==NoFrame)
	return 0;
    else
	return padding;
}

qreal FrameObj::getTotalPadding()
{
    return xsize  + padding + borderWidth; 
}

qreal FrameObj::getXPadding()
{
    return xsize; 
}

void FrameObj::setBorderWidth (const int &i)
{
    borderWidth=i;
    repaint();
}

int FrameObj::getBorderWidth()
{
    return borderWidth;
}

FrameObj::FrameType FrameObj::getFrameType()
{
    return type;
}

FrameObj::FrameType FrameObj::getFrameType(const QString &s)
{
    if (s=="Rectangle")
	return Rectangle;
    else if (s=="RoundedRectangle")
	return RoundedRectangle;
    else if (s=="Ellipse")
	return Ellipse;
    else if (s=="Cloud")
	return Cloud;
    return NoFrame;	
}

QString FrameObj::getFrameTypeName()
{
    switch (type)
    {
	case Rectangle:
	    return "Rectangle";
	    break;
	case RoundedRectangle:
	    return "RoundedRectangle";
	    break;
	case Ellipse:
	    return "Ellipse";
	    break;
	case Cloud:
	    return "Cloud";
	    break;
	default:
	    return "NoFrame";
    }
}

void FrameObj::setFrameType(const FrameType &t)
{
    if (t!=type)
    {
        clear();
        type=t;
        switch (type)
        {
            case NoFrame:
                break;
            case Rectangle:
                rectFrame = scene()->addRect(QRectF(0,0,0,0), QPen(penColor), brushColor);
                rectFrame->setZValue(dZ_FRAME_LOW);
                rectFrame->setParentItem (this);
                rectFrame->show();
                break;
            case RoundedRectangle:
                {
                    QPainterPath path;
                    pathFrame = scene()->addPath(path, QPen(penColor), brushColor);
                    pathFrame->setZValue(dZ_FRAME_LOW);
                    pathFrame->setParentItem (this);
                    pathFrame->show();
                }
                break;
            case Ellipse:
                ellipseFrame = scene()->addEllipse(QRectF(0,0,0,0), QPen(penColor), brushColor);
                ellipseFrame->setZValue(dZ_FRAME_LOW);
                ellipseFrame->setParentItem (this);
                ellipseFrame->show();
                break;
            case Cloud:
                {
                    QPainterPath path;
                    pathFrame = scene()->addPath(path, QPen(penColor), brushColor);
                    pathFrame->setZValue(dZ_FRAME_LOW);
                    pathFrame->setParentItem (this);
                    pathFrame->show();
                    break;
                }
        }
    }
    setVisibility (visible);
}

void FrameObj::setFrameType(const QString &t)
{
    if (t=="Rectangle")
	FrameObj::setFrameType (Rectangle);
    else if (t=="RoundedRectangle")  
	FrameObj::setFrameType (RoundedRectangle);
    else if (t=="Ellipse")  
	FrameObj::setFrameType (Ellipse);
    else if (t=="Cloud")  
	FrameObj::setFrameType (Cloud);
    else    
	FrameObj::setFrameType (NoFrame);
}

void FrameObj::setPenColor (QColor col)
{
    penColor=col;
    repaint();
}

QColor FrameObj::getPenColor ()
{
    return penColor;
}

void FrameObj::setBrushColor (QColor col)
{
    brushColor=col;
    repaint();
}

QColor FrameObj::getBrushColor ()
{
    return brushColor;
}

void FrameObj::setFrameIncludeChildren(bool b)
{
    includeChildren=b;
}

bool FrameObj::getFrameIncludeChildren()
{
    return includeChildren;
}

void FrameObj::repaint()
{
    QPen pen;
    pen.setColor (penColor);
    pen.setWidth (borderWidth);
    QBrush brush (brushColor);
    switch (type)
    {
	case Rectangle:
	    rectFrame->setPen   (pen);
	    rectFrame->setBrush (brush);
	    break;
	case RoundedRectangle:
	    pathFrame->setPen   (pen);
	    pathFrame->setBrush (brush);
	    break;
	case Ellipse:
	    ellipseFrame->setPen   (pen);
	    ellipseFrame->setBrush (brush);
	    break;
	case Cloud:
	    pathFrame->setPen   (pen);
	    pathFrame->setBrush (brush);
	    break;
	default:
	    break;
    }
}

void FrameObj::setZValue (double z)
{
    switch (type)
    {
	case NoFrame:
	    break;
	case Rectangle:
	    rectFrame->setZValue (z);
	    break;
	case RoundedRectangle:
	    pathFrame->setZValue (z);
	    break;
	case Ellipse:
	    ellipseFrame->setZValue (z);
	    break;
	case Cloud:
	    pathFrame->setZValue (z);
	    break;
    }
}

void FrameObj::setVisibility (bool v)
{
    MapObj::setVisibility(v);
    switch (type)
    {
	case NoFrame:
	    break;
	case Rectangle:
	    if (visible)
		rectFrame->show();
	    else    
		rectFrame->hide();
	    break;
	case RoundedRectangle:
	    if (visible)
		pathFrame->show();
	    else    
		pathFrame->hide();
	    break;
	case Ellipse:
	    if (visible)
		ellipseFrame->show();
	    else    
		ellipseFrame->hide();
	    break;
	case Cloud:
	    if (visible)
		pathFrame->show();
	    else    
		pathFrame->hide();
	    break;
    }
}

QString FrameObj::saveToDir ()
{
    QString frameTypeAttr=attribut ("frameType",getFrameTypeName());
    if (type==NoFrame) 
	return singleElement ("frame", frameTypeAttr);

    QString penColAttr=attribut ("penColor",penColor.name() );
    QString brushColAttr=attribut ("brushColor",brushColor.name() );
    QString paddingAttr=attribut ("padding",QString::number (padding) );
    QString borderWidthAttr=attribut ("borderWidth",QString::number (borderWidth) );
    QString incChildren;
    if (includeChildren)
	incChildren=attribut ("includeChildren","true");
    return singleElement (
	"frame",frameTypeAttr + 
	penColAttr + 
	brushColAttr +
	paddingAttr +
	borderWidthAttr +
	incChildren);
}

