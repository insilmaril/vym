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
    //qDebug() << "Constr ImageItem " << this;
    init();
}

ImageItem::~ImageItem()
{
    //qDebug() << "Destr ImageItem " << this << "  ic=" << imageContainer;

    if (imageContainer) {
        delete imageContainer;
        imageContainer = nullptr;
    }
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

BranchItem *ImageItem::parentBranch() { return (BranchItem *)parentItem; }

bool ImageItem::load(const QString &fname)
{
    if (!imageContainer || !imageContainer->load(fname))
        return false;

    setOriginalFilename(fname);
    setHeadingPlainText(originalFilename);
    return true;
}

ImageContainer *ImageItem::createImageContainer(QGraphicsScene *scene) // FIXME-0 scene not used!
{
    imageContainer = new ImageContainer();
    imageContainer->setImageItem(this);
    qDebug() << "II::createImageContainer for " << this << "IC=" << imageContainer;
    /* FIXME-1 check visibility of new image when creating img container
    if (((BranchItem *)parentItem)->isScrolled() ||
        !((MapItem *)parentItem)->getMO()->isVisibleObj())
        imageContainer->setVisibility(false);
    imageContainer->setZValue(zValue);
    imageContainer->setPos(pos);
    imageContainer->updateVisibility();
    imageContainer->setLinkColor();
    */
    return imageContainer;
}

ImageContainer *ImageItem::getImageContainer()
{
    return imageContainer;
}

void ImageItem::unlinkImageContainer()
{
    // Called from destructor of containers to 
    // avoid double deletion 
    imageContainer = nullptr;
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

void ImageItem::setZValue(int z)
{
    imageContainer->setZValue(z);
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
                MapItem::getPosAttr() +
                MapItem::getLinkableAttr() +
                TreeItem::getGeneralAttr() + zAttr +
                attribut("href", QString("file:") + url) +
                nameAttr + scaleAttr + idAttr);
}
