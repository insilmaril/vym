#include <QImageReader>
#include <QPixmap>

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
    clickBox.setSize (QSizeF(icon->boundingRect().width(), icon->boundingRect().height()));filename="";
    originalFilename="no original name available";
    filetype="";
    useRelPos=true;

    //Hide flags
    systemFlags->setShowFlags(false);
}

void FloatImageObj::copy (FloatImageObj* other)
{		    
    FloatObj::copy (other);
    icon->copy (other->icon);
    filetype=other->filetype;
    filename=other->filename;
    originalFilename=other->originalFilename;
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

void FloatImageObj::load (const QPixmap &pixmap)
{
    icon->load(pixmap);
    bbox.setSize (QSizeF(icon->boundingRect().width()+8, icon->boundingRect().height()+8));
    clickBox.setSize (QSizeF(icon->boundingRect().width()+8, icon->boundingRect().height()+8));
    positionBBox();
    filetype="PNG";
    filename="No filename given";
}

bool FloatImageObj::load (const QString &fn)
{   
    QImageReader reader (fn);
    QImage img;

    if( reader.read (&img))
    {
	icon->setPixmap(QPixmap::fromImage(img));
	bbox.setSize (QSizeF(icon->boundingRect().width()+8, icon->boundingRect().height()+8));
	positionBBox();
	filename=fn;
	filetype=reader.format();
	setOriginalFilename (fn);
	return true;
    } else
	return false;
    
}

void FloatImageObj::save (const QString &fn, const QString &format)
{   
    icon->save (fn,qPrintable (format));
}

void FloatImageObj::setOriginalFilename(const QString & fn)
{
    originalFilename=fn;

    // Set short name, too. Search from behind:
    int i=originalFilename.findRev("/");
    if (i>=0) originalFilename=originalFilename.remove (0,i+1);
}

QString FloatImageObj::getOriginalFilename()
{
    return originalFilename;
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

