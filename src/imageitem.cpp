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

ImageItem::ImageItem():MapItem(nullptr) // FIXME-2 MapItem should no longer be needed
{
    qDebug() << "Constr ImageItem " << this;
    init();
}


ImageItem::~ImageItem()
{
    // qDebug() << "Destr ImageItem " << this;

    // The related data in imageContainer is not deleted here,
    // but when the BranchItem is deleted, which has the imageContainer 
    // somewhere in it's tree of containers. (Deleting of a BranchItem first 
    // triggers deletion all its related containers)
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
    //FIXME-0 BranchItem *parentBranch = (BranchItem*)parentItem;

    imageContainer = new ImageContainer(scene);
    /* FIXME-0 cont here, check visibility of new image
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

void ImageItem::setScaleFactor(qreal f)
{
    if (imageContainer)
        imageContainer->setScaleFactor(f);
}

qreal ImageItem::getScaleFactor()
{
    if (imageContainer)
        return imageContainer->getScaleFactor();
    else
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

QString ImageItem::getUniqueFilename()
{
    if (imageContainer)
        return "image-" + getUuid().toString() + imageContainer->getExtension();
    else
        return QString();
}

bool ImageItem::saveImage(const QString &fn)
{
    qDebug() << "II:saveImage " << fn << imageContainer;
    // This is used when exporting maps or saving selection
    if (imageContainer)
        return imageContainer->save(fn);
    else 
        return false;
}

QString ImageItem::saveToDir(const QString &tmpdir, const QString &prefix)
{
    qDebug() << "II:saveToDir " << tmpdir << imageContainer;
    if (!imageContainer) {
        qWarning() << "ImageItem::saveToDir  no imageContainer!";
        return QString();
    }

    if (hidden)
        return "";

    // Save uuid
    QString idAttr = attribut("uuid", uuid.toString());

    QString zAttr = attribut("zValue", QString().setNum(zValue));
    QString url;

    url = "images/" + prefix + "image-" + QString().number(itemID) +
          imageContainer->getExtension();

    // And really save the image  (svgs will be copied from cash!)
    imageContainer->save(tmpdir + "/" + url);

    QString nameAttr = attribut("originalName", originalFilename);

    QString scaleAttr =
        attribut("scaleFactor", QString().setNum(imageContainer->getScaleFactor()));

    return singleElement("floatimage",
                         getMapAttr() + getGeneralAttr() + zAttr +
                             attribut("href", QString("file:") + url) +
                             nameAttr + scaleAttr + idAttr);
}
