#include "imageitem.h"

#include "branchitem.h"
#include "mapobj.h"	// z-values

#include <QDebug>
#include <QString>
#include <iostream>

bool isImage (const QString &fname)
{
    QRegExp rx("(jpg|jpeg|png|xmp|gif|svg)$");
    rx.setCaseSensitivity (Qt::CaseInsensitive);
    return fname.contains (rx);
}

ImageItem::ImageItem()
{
    qDebug()<<"Constr ImageItem";
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
    hideLinkUnselected = true;
    imageType = ImageObj::Undefined;
    originalFilename = "no original name available";
    zValue = dZ_FLOATIMG;
    posMode = Relative;
}

void ImageItem::clear() 
{
    // pure virtual in parent treeitem
    // not used here currently
}

ImageObj::ImageType ImageItem::getImageType()
{
    return imageType;
}

bool ImageItem::load(const QString &fname) // FIXME-1 what if there is already an image with different type loaded?
{
    FloatImageObj *fio = (FloatImageObj*)mo;
    if (!fio->load(fname) ) return false;

    /*
    if (fname.toLower().endsWith(".svg"))
    {   
        // Load svg
        // originalSvg = new (QGraphicsSvgItem);   // FIXME-0 maybe only load svg data into ImageObj?
        
        
        //FIXME-0 cont here...
        fio->loadSvg(fname); // testing only

        *
        if (fio->load(fname))
        {
            imageType = ImageObj::SVG;

        } else
            return false;
        *

    } else
    {   
        // Load pixmap  // FIXME-1 originalImage should be within ImageObj, not ImageItem
        // FIXME-1 Check: Is pixmap loaded into II, IO or FIO by filenmae after all?
        originalImage = new (QImage);

        bool ok = originalImage->load (fname);   
        if (fio && ok)
        {
            fio->load (originalImage);

            imageType = ImageObj::Pixmap;
        } else
        {
            qWarning() << "ImageItem::load failed for " << fname;
            delete originalImage;
            originalImage = NULL;
            return false;
        }
    }
    */
    setOriginalFilename (fname);
    setHeadingPlainText (originalFilename);

    return true;	
}

FloatImageObj* ImageItem::createMapObj()
{
    FloatImageObj *fio = new FloatImageObj ( ((MapItem*)parentItem)->getMO(),this);
    mo = fio;
    if (((BranchItem*)parentItem)->isScrolled() || !((MapItem*)parentItem)->getMO()->isVisibleObj() )
	    fio->setVisibility (false);
    initLMO();	// set rel/abs position in mapitem
    fio->setZValue(zValue);
    fio->setRelPos (pos);
    fio->updateVisibility();
    return fio;
}

void ImageItem::setScaleFactor (qreal f)
{
    if (mo) ((FloatImageObj*)mo)->setScaleFactor(f);
}

qreal ImageItem::getScaleFactor ()
{
    if (mo) return ((FloatImageObj*)mo)->getScaleFactor();
}

void ImageItem::setZValue(int z)
{
    zValue = z;
    if (mo) ((FloatImageObj*)mo)->setZValue(z);
}

void ImageItem::setOriginalFilename(const QString & fn)
{
    originalFilename = fn;

    // Set short name. Search from behind:
    int i = originalFilename.lastIndexOf("/");
    if (i >= 0) originalFilename=originalFilename.remove (0, i + 1);
    setHeadingPlainText (originalFilename);
}

QString ImageItem::getOriginalFilename()
{
    return originalFilename;
}

bool ImageItem::saveImage(const QString &fn, const QString &format)  
{
    // This is used when exporting maps or saving selection
    FloatImageObj *fio = (FloatImageObj*)mo;
    return fio->save (fn);  // FIXME-0 what about format?
}

QString ImageItem::saveToDir (const QString &tmpdir,const QString &prefix) 
{
    if (hidden) return "";

    // Save uuid 
    QString idAttr = attribut("uuid", uuid.toString());

    QString zAttr = attribut ("zValue", QString().setNum(zValue));
    QString url;

    // Create unique string for filename based on memory address
    ulong n = reinterpret_cast <ulong> (this);

    url = "images/" + prefix + "image-" + QString().number(n, 10) + ".png" ;

    // And really save the image
    FloatImageObj *fio = (FloatImageObj*)mo;
    fio->save (tmpdir + "/" + url);

    QString nameAttr = attribut ("originalName",originalFilename);

    QString scaleAttr =
	attribut ("scaleFactor", QString().setNum(fio->getScaleFactor()));

    return singleElement ("floatimage",  
	getMapAttr() 
	+ getGeneralAttr()
	+ zAttr  
	+ attribut ("href", QString ("file:") + url)
	+ nameAttr
	+ scaleAttr
        + idAttr
    );	
}

