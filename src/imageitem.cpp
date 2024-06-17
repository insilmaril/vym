#include "imageitem.h"

#include <QDebug>
#include <QRegularExpression>
#include <QString>
#include <iostream>

#include "branchitem.h"
#include "image-container.h"
#include "image-wrapper.h"
#include "vymmodel.h"

bool isImage(const QString &fname)
{
    QRegularExpression re("(jpg|jpeg|png|xmp|gif|svg)$");
    re.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
    return fname.contains(re);
}

ImageItem::ImageItem():MapItem(nullptr)
{
    //qDebug() << "Constr ImageItem " << this;
    init();
}

ImageItem::~ImageItem()
{
    qDebug() << "Destr ImageItem " << this << "  ic=" << imageContainer << "  fpInZipDir=" << filePathInZipDir;

    if (imageContainer) {
        delete imageContainer;
        imageContainer = nullptr;

        // Remove images container, if no longer required
        if (parentBranch())
            parentBranch()->getBranchContainer()->updateChildrenStructure();
    }

    if (!filePathInZipDir.isEmpty() && QFile(filePathInZipDir).exists()) {
        // Remove file used to compress map
        qDebug() << "zipDirPath=" << filePathInZipDir << "  model tmpDir:" << model->tmpDirPath();
        if (!QFile(filePathInZipDir).remove())
            qWarning() << "Destr ImageItem failed to remove " << filePathInZipDir;
        else
            qDebug() << "Destr ImageItem removed " << filePathInZipDir;
    }

    if (imageWrapperInt) {
        delete imageWrapperInt;
        imageWrapperInt = nullptr;
    }
}

void ImageItem::init()
{
    imageContainer = nullptr;
    imageWrapperInt = nullptr;
    setType(Image);
    hideLinkUnselected = true;
    originalFilename = "no original name available";
    filePathInZipDir.clear();
}

BranchItem *ImageItem::parentBranch() { return (BranchItem *)parentItem; }

void ImageItem::setParentBranch(BranchItem *pbi)
{
    setModel(pbi->getModel());
    rootItem = model->getRootItem();
    parentItem = pbi;
}

ImageWrapper* ImageItem::imageWrapper()
{
    if (!imageWrapperInt)
        imageWrapperInt = new ImageWrapper(this);

    return imageWrapperInt;
}

void ImageItem::setFilePathInZipDir()
{
    filePathInZipDir = model->zipDirPath() + "/images/" + 
        "image-" + QString().number(itemID) + imageContainer->getExtension();
    qDebug() << "II::setFilePathInZipDir to " << filePathInZipDir;
}

bool ImageItem::load(const QString &fname)
{
    if (!imageContainer || !imageContainer->load(fname))
        return false;

    setOriginalFilename(fname);
    setFilePathInZipDir();
    setHeadingPlainText(originalFilename);
    return true;
}

ImageContainer *ImageItem::createImageContainer()
{
    imageContainer = new ImageContainer();
    imageContainer->setImageItem(this);
    // qDebug() << "II::createImageContainer for " << this << "IC=" << imageContainer;
    return imageContainer;
}

ImageContainer *ImageItem::getImageContainer()
{
    return imageContainer;
}

void ImageItem::updateContainerStackingOrder()
{
    // After relinking images (also moving up/down), the order of the 
    // ImageContainers does not match the order of ImageItems any longer
    // For simplicity we always reparent. The absolute position will not be changed here
    //
    // See also BranchItem::updateContainerStackingOrder()

    int n = num();

    QPointF sp = imageContainer->scenePos();

    imageContainer->setParentItem(nullptr);

    parentBranch()->addToImagesContainer(imageContainer);

    while (n < parentBranch()->imageCount() - 1) {
        // Insert container of this image above others

        // The next sibling container might currently still be temporarily 
        // linked to tmpParentContainer, in that case it is not a sibling and 
        // cannot be inserted using QGraphicsItem::stackBefore
        //
        // We try the next sibling then, if this fails, just append at the end.
        if ( (parentBranch()->getImageNum(n + 1))->getContainer()->parentItem() != parentBranch()->getImagesContainer() )
            n++;
        else {
            imageContainer->stackBefore( (parentBranch()->getImageNum(n + 1))->getContainer() );
            break;
        }
    }

    imageContainer->setPos(imageContainer->parentItem()->sceneTransform().inverted().map(sp));
}


void ImageItem::unlinkImageContainer()
{
    // Called from destructor of containers to 
    // avoid double deletion 
    imageContainer = nullptr;
}

void ImageItem::setScale(qreal f)
{
    if (imageContainer)
        imageContainer->setScale(f);
}

qreal ImageItem::scale()
{
    if (imageContainer)
        return imageContainer->scale();
    else
        return 1;
}

qreal ImageItem::width()
{
    if (imageContainer)
        return imageContainer->rect().width();
    else
        return -1;
}

qreal ImageItem::height()
{
    if (imageContainer)
        return imageContainer->rect().height();
    else
        return -1;
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
    // This is used when exporting maps or saving selection
    if (imageContainer)
        return imageContainer->save(fn);
    else
        return false;
}

QString ImageItem::saveToDir(const QString &tmpdir)
{
    if (!imageContainer) {
        qWarning() << "ImageItem::saveToDir  no imageContainer!";
        return QString();
    }

    if (hidden)
        return QString();

    QString url = "images/image-" + QString().number(itemID) +
          imageContainer->getExtension();

    // And really save the image  (svgs will be copied from cache!)
    QString currentPath = tmpdir + url;
    qDebug() << "II::saveToDir     cp=" << currentPath << " zipPath=" << filePathInZipDir;
    if (!QFile(currentPath).exists())
        // Only save, if not already there
        imageContainer->save(currentPath);
    else
        qDebug() << "II::saveToDir  file exists already: " << currentPath;

    QString attributes;

    if (!originalFilename.isEmpty())
        attributes += attribute("originalName", originalFilename);

    attributes += attribute("href", QString("file:") + url);
    attributes += attribute("scale", QString().setNum(imageContainer->scale()));
    attributes += MapItem::getPosAttr() +
                  MapItem::getLinkableAttr() +
                  TreeItem::getGeneralAttr();

    attributes += attribute("uuid", uuid.toString());

    if (originalFilename == headingPlain())
        return singleElement("floatimage", attributes);
    else {
        QString s = beginElement("floatimage", attributes);
        incIndent();
        s += headingInt.saveToDir();
        decIndent();
        s += endElement("floatimage");
        return s;
    }
}
