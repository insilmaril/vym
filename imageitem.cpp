#include "imageitem.h"

#include "branchitem.h"
#include "mapobj.h"	// z-values

#include <QDebug>
#include <QString>
#include <iostream>

bool isImage (const QString &fname)
{
    QRegExp rx("(jpg|jpeg|png|xmp|gif|svg)");
    rx.setCaseSensitivity (Qt::CaseInsensitive);
    return fname.contains (rx);
}

ImageItem::ImageItem()
{
    //qDebug()<<"Constr ImageItem";
    init();
}

ImageItem::ImageItem (const QList<QVariant> &data, TreeItem *parent):MapItem (data,parent)
{
    init();
}

ImageItem::~ImageItem()
{
    //qDebug()<<"Destr ImageItem";
    if (mo) delete mo;
}

void ImageItem::init()
{
    setType (Image);
    imageType=Undefined;
    hideLinkUnselected=true;
    originalFilename="no original name available";
    zValue=dZ_FLOATIMG;
    scaleX=1;
    scaleY=1;
    posMode=Relative;
}

void ImageItem::clear()
{
}

ImageItem::ImageType ImageItem::getImageType()
{
    return imageType;
}

void ImageItem::load(const QImage &img)
{
    originalImage=img;
    if (mo) ((FloatImageObj*)mo)->load (originalImage);
}

bool ImageItem::load(const QString &fname)
{
    bool ok=originalImage.load (fname);    //FIXME-4 Error handling missing
    if (mo && ok)
    {
	setOriginalFilename (fname);
    setHeadingText (originalFilename);
	((FloatImageObj*)mo)->load (originalImage);
    }	else
	qDebug() <<"ImageItem::load failed for "<<fname;
    return ok;	
}

FloatImageObj* ImageItem::createMapObj()
{
    FloatImageObj *fio=new FloatImageObj ( ((MapItem*)parentItem)->getMO(),this);
    mo=fio;
    if (((BranchItem*)parentItem)->isScrolled() || !((MapItem*)parentItem)->getMO()->isVisibleObj() )
	    fio->setVisibility (false);
    initLMO();	// set rel/abs position in mapitem
    fio->setZValue(zValue);
    fio->setRelPos (pos);
    fio->updateVisibility();
    return fio;
}

void ImageItem::setScale (qreal sx, qreal sy)
{
    scaleX=sx;
    scaleY=sy;
    int w=originalImage.width()*scaleX;
    int h=originalImage.height()*scaleY;
    if (mo) ((FloatImageObj*)mo)->load (originalImage.scaled (w,h));
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
    if (mo) ((FloatImageObj*)mo)->setZValue(z);
}

void ImageItem::setOriginalFilename(const QString & fn)
{
    originalFilename=fn;

    // Set short name. Search from behind:
    int i=originalFilename.lastIndexOf("/");
    if (i>=0) originalFilename=originalFilename.remove (0,i+1);
    setHeadingText (originalFilename);
}

QString ImageItem::getOriginalFilename()
{
    return originalFilename;
}

bool ImageItem::save(const QString &fn, const QString &format)
{
    return originalImage.save (fn,qPrintable (format)); 
}

QString ImageItem::saveToDir (const QString &tmpdir,const QString &prefix) 
{
    if (hidden) return "";

    // Save uuid 
    QString idAttr=attribut("uuid",uuid.toString());

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
        +idAttr
    );	
}

