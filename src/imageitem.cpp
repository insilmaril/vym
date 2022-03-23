#include "imageitem.h"

#include <QDebug>
#include <QString>
#include <iostream>

#include "branchitem.h"

#include "image-container.h"

bool isImage(const QString &fname)
{
    QRegExp rx("(jpg|jpeg|png|xmp|gif|svg)$");
    rx.setCaseSensitivity(Qt::CaseInsensitive);
    return fname.contains(rx);
}

ImageItem::ImageItem():MapItem(nullptr)
{
    qDebug() << "Constr ImageItem";
    init();
}


ImageItem::~ImageItem()
{
    qDebug()<<"Destr ImageItem";
    /* FIXME-2   if (mo)
        delete mo;
        */
}

void ImageItem::init()
{
    imageContainer = nullptr;
    setType(Image);
    hideLinkUnselected = true;  // FIXME-2 needed?
    originalFilename = "no original name available";
    zValue = dZ_FLOATIMG;
}

void ImageItem::clear() // FIXME-2 check if needed
{
    // pure virtual in parent treeitem
    // not used here currently
}

bool ImageItem::load(const QString &fname)
{
    if (!imageContainer || !imageContainer->load(fname))
        return false;

    setOriginalFilename(fname);
    setHeadingPlainText(originalFilename);

    return true;
}

ImageContainer *ImageItem::createImageContainer(QGraphicsScene *scene)
{
    BranchItem *parentBranch = (BranchItem*)parentItem;

    imageContainer = new ImageContainer(scene);
    //parentBranch->getChildrenImagesContainer()->
    // FIXME-2 removeFloatImageObj *fio = new FloatImageObj(((MapItem *)parentItem)->getMO(), this);
    /* FIXME-0 cont here
    mo = fio;
    if (((BranchItem *)parentItem)->isScrolled() ||
        !((MapItem *)parentItem)->getMO()->isVisibleObj())
        fio->setVisibility(false);
    // initLMO(); // set rel/abs position in mapitem
    fio->setZValue(zValue);
    fio->setRelPos(pos);
    fio->updateVisibility();
    fio->setLinkColor();
    */
    return imageContainer;
}

ImageContainer *ImageItem::getImageContainer()
{
    return imageContainer;
}

void ImageItem::setScaleFactor(qreal f) // FIXME-0 
{
    /*
    if (mo)
        ((FloatImageObj *)mo)->setScaleFactor(f);
        */
}

qreal ImageItem::getScaleFactor() // FIXME-0 
{
    /*
    if (mo)
        return ((FloatImageObj *)mo)->getScaleFactor();
        */
    return 1;
}

void ImageItem::setZValue(int z) // FIXME-0
{
    /*
    zValue = z;
    if (mo)
        ((FloatImageObj *)mo)->setZValue(z);
        */
}

void ImageItem::setOriginalFilename(const QString &fn)
{
    originalFilename = fn;

    // Set short name. Search from behind:
    int i = originalFilename.lastIndexOf("/");
    if (i >= 0)
        originalFilename = originalFilename.remove(0, i + 1);
    setHeadingPlainText(originalFilename);
}

QString ImageItem::getOriginalFilename() { return originalFilename; }

QString ImageItem::getUniqueFilename() // FIXME-0
{
    /*
    FloatImageObj *fio = (FloatImageObj *)mo;
    return "image-" + getUuid().toString() + fio->getExtension();
    */
    return QString();
}

bool ImageItem::saveImage(const QString &fn) // FIXME-0
{
    return false;
/*
    // This is used when exporting maps or saving selection
    FloatImageObj *fio = (FloatImageObj *)mo;
    return fio->save(fn);
    */
}

QString ImageItem::saveToDir(const QString &tmpdir, const QString &prefix) // FIXME-0
{
    return QString();
    /*
    if (hidden)
        return "";

    // Save uuid
    QString idAttr = attribut("uuid", uuid.toString());

    QString zAttr = attribut("zValue", QString().setNum(zValue));
    QString url;

    FloatImageObj *fio = (FloatImageObj *)mo;

    url = "images/" + prefix + "image-" + QString().number(itemID) +
          fio->getExtension();

    // And really save the image  (svgs will be copied from cash!)
    fio->save(tmpdir + "/" + url);

    QString nameAttr = attribut("originalName", originalFilename);

    QString scaleAttr =
        attribut("scaleFactor", QString().setNum(fio->getScaleFactor()));

    return singleElement("floatimage",
                         getMapAttr() + getGeneralAttr() + zAttr +
                             attribut("href", QString("file:") + url) +
                             nameAttr + scaleAttr + idAttr);
     */
}
