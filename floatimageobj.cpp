#include <QImageReader>

#include "floatimageobj.h"
#include "branchobj.h"

/////////////////////////////////////////////////////////////////
// FloatImageObj
/////////////////////////////////////////////////////////////////

FloatImageObj::FloatImageObj (QGraphicsScene* s,TreeItem *ti):FloatObj(s,ti)
{
   //cout << "Const FloatImageObj s="<<s<<"  ti="<<ti<<endl;
    setParObj (this);	
    init();
}

FloatImageObj::~FloatImageObj ()
{
//  cout << "Destr FloatImageObj "<<this<<"\n";
    delete(icon);
}

void FloatImageObj::init () 
{
    icon=new ImageObj (scene);
    icon->setPos (absPos.x(), absPos.y() );
    icon->setVisibility (true);
    setZValue (Z_INIT);
    bbox.setSize (QSizeF(icon->boundingRect().width(), icon->boundingRect().height()));
    clickBox.setSize (QSizeF(icon->boundingRect().width(), icon->boundingRect().height()));
    useRelPos=true;

    //Hide flags
    systemFlags->setShowFlags(false);
}

void FloatImageObj::copy (FloatImageObj* other)
{		    
    FloatObj::copy (other);
    icon->copy (other->icon);
    positionBBox();
}

void FloatImageObj::setZValue (const int &i)
{
    icon->setZValue (i);
    zPlane=i;
}

int FloatImageObj::z ()
{
    return qRound (icon->zValue());
}

void FloatImageObj::load (const QImage &img)
{
    icon->load(QPixmap::fromImage(img));
    bbox.setSize (QSizeF(icon->boundingRect().width()+8, icon->boundingRect().height()+8));
    clickBox.setSize (QSizeF(icon->boundingRect().width()+8, icon->boundingRect().height()+8));
    positionBBox();
}

void FloatImageObj::setVisibility(bool v)
{
    OrnamentedObj::setVisibility(v);
    if (v)
	icon->setVisibility(true);
    else
	icon->setVisibility(false);
}

void FloatImageObj::move (double x, double y)
{
    FloatObj::move(x,y);
    icon->setPos (x+4,y+4);
    positionBBox();
}

void FloatImageObj::move (QPointF p)
{
    OrnamentedObj::move (p.x(),p.y());
}

void FloatImageObj::positionBBox()
{
    clickBox=bbox;
}

void FloatImageObj::calcBBoxSize()
{
    // TODO
}

QRectF FloatImageObj::getBBoxSizeWithChildren()
{
    //TODO abstract in linkablemapobj.h, not calculated
    return bboxTotal;
}

void FloatImageObj::calcBBoxSizeWithChildren()
{
    //TODO abstract in linkablemapobj.h
}

