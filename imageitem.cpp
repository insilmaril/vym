#include "imageitem.h"

#include "branchitem.h"
#include "mapobj.h"	// z-values

#include <QString>
#include <iostream>
using namespace std;

ImageItem::ImageItem()
{
    init();
}

ImageItem::ImageItem (const QList<QVariant> &data, TreeItem *parent):MapItem (data,parent)
{
    init();
}

ImageItem::~ImageItem()
{
    //cout <<"Destr ImageItem\n";
    if (lmo) delete lmo;
}

void ImageItem::init()
{
    setType (Image);
    imageType=Undefined;
    hideLinkUnselected=true;
    originalFilename="no original name available";
    zValue=Z_INIT;
    scaleX=1;
    scaleY=1;
    posMode=Relative;
}

ImageItem::ImageType ImageItem::getImageType()
{
    return imageType;
}

void ImageItem::load(const QImage &img)
{
    originalImage=img;
    if (lmo) ((FloatImageObj*)lmo)->load (originalImage);
}

bool ImageItem::load(const QString &fname)
{
    bool ok=originalImage.load (fname);    //FIXME-4 Error handling missing
    if (lmo && ok)
    {
	setOriginalFilename (fname);
	setHeading (originalFilename);
	((FloatImageObj*)lmo)->load (originalImage);
    }	
    return ok;	
}

FloatImageObj* ImageItem::createMapObj(QGraphicsScene *scene)
{
    FloatImageObj *fio=new FloatImageObj (scene,this);
    lmo=fio;
    fio->setParObj ( ((MapItem*)parentItem)->getLMO());
    if (((BranchItem*)parentItem)->isScrolled() || !((MapItem*)parentItem)->getLMO()->isVisibleObj() )
	    fio->setVisibility (false);
    initLMO();
    fio->setZValue(zValue);
    fio->setRelPos (pos);
    //cout << "II::createMO   fio="<<fio<<"   tI="<<fio->getTreeItem()<<endl;
    return fio;
}

void ImageItem::setScale (qreal sx, qreal sy)
{
    scaleX=sx;
    scaleY=sy;
    int w=originalImage.width()*scaleX;
    int h=originalImage.height()*scaleY;
    if (lmo) ((FloatImageObj*)lmo)->load (originalImage.scaled (w,h));
}

qreal ImageItem::getScaleX ()
{
    return scaleX;
}

qreal ImageItem::getScaleY ()
{
    return scaleY;
}

void ImageItem::setZValue(int z)
{
    zValue=z;
    if (lmo) ((FloatImageObj*)lmo)->setZValue(z);
}

void ImageItem::setOriginalFilename(const QString & fn)
{
    originalFilename=fn;

    // Set short name. Search from behind:
    int i=originalFilename.lastIndexOf("/");
    if (i>=0) originalFilename=originalFilename.remove (0,i+1);
    setHeading (originalFilename);
}

QString ImageItem::getOriginalFilename()
{
    return originalFilename;
}

void ImageItem::save(const QString &fn, const QString &format)
{
    originalImage.save (fn,qPrintable (format));
}

QString ImageItem::saveToDir (const QString &tmpdir,const QString &prefix) 
{
    if (hidden) return "";

    QString zAttr=attribut ("zValue",QString().setNum(zValue));
    QString url;

    ulong n=reinterpret_cast <ulong> (this);

    url="images/"+prefix+"image-" + QString().number(n,10) + ".png" ;

    // And really save the image
    originalImage.save (tmpdir +"/"+ url, "PNG");
 
    QString nameAttr=attribut ("originalName",originalFilename);

    QString scaleAttr=
	attribut ("scaleX",QString().setNum(scaleX))+
	attribut ("scaleY",QString().setNum(scaleY));

    return singleElement ("floatimage",  
	getMapAttr() 
	+getGeneralAttr()
	+zAttr  
	+attribut ("href",QString ("file:")+url)
	+nameAttr
	+scaleAttr
    );	
}

