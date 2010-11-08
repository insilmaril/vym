#include "frameobj.h"

#include <QColor>
#include <QDebug>

/////////////////////////////////////////////////////////////////
// FrameObj
/////////////////////////////////////////////////////////////////
FrameObj::FrameObj() : MapObj()
{
//    cout << "Const FrameObj ()\n";
    init ();
}

FrameObj::FrameObj(QGraphicsScene *s) :MapObj(s)
{
//    cout << "Const FrameObj\n";
    init ();
}

FrameObj::~FrameObj()
{
    clear();
}

void FrameObj::init()
{
    type=NoFrame;
    padding=10;
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
	case Ellipse:
	    delete ellipseFrame;
	    break;
    }
    type=NoFrame;
    padding=0;
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
	case Ellipse:
	    ellipseFrame->setPos (x,y);
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
	//  rectFrame->prepareGeometryChange();
	    rectFrame->setRect (QRectF(bbox.x(),bbox.y(),bbox.width(),bbox.height() ));
	    break;
	case Ellipse:
	//  ellipseFrame->prepareGeometryChange();
	    ellipseFrame->setRect (QRectF(bbox.x(),bbox.y(),bbox.width(),bbox.height() ));
	    break;
    }
}

void FrameObj::setPadding (const int &i)
{
    padding=i;
    repaint();
}

int FrameObj::getPadding()
{
    if (type==NoFrame) 
	return 0;
    else    
	return padding;
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

QString FrameObj::getFrameTypeName()
{
    switch (type)
    {
	case Rectangle:
	    return "Rectangle";
	    break;
	case Ellipse:
	    return "Ellipse";
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
	    rectFrame = scene->addRect(QRectF(0,0,0,0), QPen(penColor), brushColor);
	    rectFrame->setZValue(Z_INIT);
	    rectFrame->show();
	    break;
	case Ellipse:
	    ellipseFrame = scene->addEllipse(QRectF(0,0,0,0), QPen(penColor), brushColor);
	    ellipseFrame->setZValue(Z_INIT);
	    ellipseFrame->show();
	    break;
	}
    }
    setVisibility (visible);
}

void FrameObj::setFrameType(const QString &t)
{
    if (t=="Rectangle")
	FrameObj::setFrameType (Rectangle);
    else if (t=="Ellipse")  
	FrameObj::setFrameType (Ellipse);
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
    //  qDebug()<<"FO:repaint tI="<<treeItem;
    //	qDebug()<<"              "<<treeItem->getHeading();
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
	case Ellipse:
	    ellipseFrame->setPen   (pen);
	    ellipseFrame->setBrush (brush);
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
	case Ellipse:
	    ellipseFrame->setZValue (z);
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
	case Ellipse:
	    if (visible)
		ellipseFrame->show();
	    else    
		ellipseFrame->hide();
	    break;
    }
}

QString FrameObj::saveToDir ()
{
    if (type==NoFrame) return QString();
    QString frameTypeAttr=attribut ("frameType",getFrameTypeName());
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

