#include "image-container.h"

#include <QDebug>
#include <QIcon>

#include "file.h"
#include "imageitem.h"

extern QDir cashDir;
extern ulong imageLastID;

/////////////////////////////////////////////////////////////////
// ImageContainer
/////////////////////////////////////////////////////////////////
ImageContainer::ImageContainer()
{
    qDebug() << "Const ImageContainer ()  this=" << this;
    init();
}

ImageContainer::~ImageContainer()   // FIXME-1 remove imagesContainer from branch-container, if no longer needed
{
    qDebug() << "Destr ImageContainer  this=" << this << "  imageType = " << imageType ;
    if (imageItem) imageItem->unlinkImageContainer();
}

void ImageContainer::copy(ImageContainer *other)    // FIXME-00000 copy rect!
{
    qDebug() << "IC::copy"; // FIXME-2 testing only
    qDebug() << "  type="  << other->imageType;
    qDebug() << "  vis ="  << isVisible();
    qDebug() << "  other->r ="  << other->rect();
    prepareGeometryChange();
    if (imageType != ImageContainer::Undefined)
        qWarning() << "ImageContainer::copy into existing image of type "
                   << imageType;

    switch (other->imageType) {
        case ImageContainer::SVG:
        case ImageContainer::ClonedSVG:
            if (!other->svgCashPath.isEmpty()) {
                // Loading here will also set imageType
                load(other->svgCashPath, true);
            }
            else
                qWarning() << "ImgObj::copy svg: no svgCashPath available.";

            svgItem->setVisible(isVisible());
            svgItem->setParentItem(this);
            setRect(svgItem->boundingRect());
            break;
        case ImageContainer::Pixmap:
            pixmapItem = new QGraphicsPixmapItem();
            pixmapItem->setPixmap(other->pixmapItem->pixmap());
            pixmapItem->setParentItem(this); // FIXMEe-2 check...
            pixmapItem->setVisible(isVisible());
            imageType = ImageContainer::Pixmap;
            setRect(pixmapItem->boundingRect());
            break;
        case ImageContainer::ModifiedPixmap:
            // create new pixmap?
            pixmapItem->setPixmap(other->pixmapItem->pixmap());
            pixmapItem->setParentItem(this);
            pixmapItem->setVisible(isVisible());
            imageType = ImageContainer::Pixmap;
            break;
        default:
            qWarning() << "ImgObj::copy other->imageType undefined";
            return;
            break;
    }
    qDebug() << "  r ="  << rect();
    setScaleFactor(other->scaleFactor);
}

void ImageContainer::init()
{
    type = Image;

    imageItem = nullptr;

    // Assign ID
    imageLastID++;
    imageID = imageLastID;

    imageType = ImageContainer::Undefined;
    layout =  Container::Horizontal;

    svgItem = nullptr;
    pixmapItem = nullptr;
    originalPixmap = nullptr;
    scaleFactor = 1;

    // Debugging only
    setPen(QPen(Qt::red));  // FIXME-2
}

void ImageContainer::setZValue(qreal z)
{
    switch (imageType) {
        case ImageContainer::SVG:
        case ImageContainer::ClonedSVG:
            svgItem->setZValue(z);
            break;
        case ImageContainer::Pixmap:
        case ImageContainer::ModifiedPixmap:
            pixmapItem->setZValue(z);
            break;
        default:
            break;
    }
}

void ImageContainer::setVisibility(bool v)
{
    switch (imageType) {
        case ImageContainer::SVG:
        case ImageContainer::ClonedSVG:
            v ? svgItem->show() : svgItem->hide();
            break;
        case ImageContainer::Pixmap:
        case ImageContainer::ModifiedPixmap:
            v ? pixmapItem->show() : pixmapItem->hide();
            break;
        default:
            break;
    }
}

void ImageContainer::setWidth(qreal w)  // FIXME-000
{
    if (boundingRect().width() == 0)
        return;

    qDebug() << "IC::setWidth w=" << w << "bR.w=" << boundingRect().width() << " sf=" << w/boundingRect().width();

    setScaleFactor(w / boundingRect().width());
}

void ImageContainer::setScaleFactor(qreal f)
{
    scaleFactor = f;
        switch (imageType) {
            case ImageContainer::SVG:
            case ImageContainer::ClonedSVG: {
                svgItem->setScale(f);
                QRectF r = rect();
                r.setWidth(r.width() * f);
                r.setHeight(r.height() * f);
                setRect(r);
                break;
            }
            case ImageContainer::Pixmap:
                if (f != 1) {
                    // create ModifiedPixmap
                    originalPixmap = new QPixmap(pixmapItem->pixmap());
                    imageType = ModifiedPixmap;

                    setScaleFactor(f);
                }
                break;
            case ImageContainer::ModifiedPixmap:
                if (!originalPixmap) {
                    qWarning() << "ImageContainer::setScaleFactor   no originalPixmap!";
                    return;
                }
                pixmapItem->setPixmap(originalPixmap->scaled(
                    originalPixmap->width() * f, originalPixmap->height() * f));
                break;
            default:
            break;
    }
}

qreal ImageContainer::getScaleFactor() { return scaleFactor; }

void ImageContainer::updateRect()   // FIXME-2 never called
{
    QRectF r;
    switch (imageType) {
        case ImageContainer::SVG:
        case ImageContainer::ClonedSVG:
            r = QRectF(0, 0, svgItem->boundingRect().width() * scaleFactor,
                             svgItem->boundingRect().height() * scaleFactor);
            break;
        case ImageContainer::Pixmap:
            r = pixmapItem->boundingRect();
            break;
        case ImageContainer::ModifiedPixmap:
            r = pixmapItem->boundingRect();
            break;
        default:
            qWarning() << "ImageContainer::updateRect unknown imageType";
            break;
    }
    qDebug() << "IC::updateRect  r= " << r;
    setRect(r);
}

bool ImageContainer::load(const QString &fn, bool createClone)
{
    // createClone == true, if called via copy()
    if (imageType != ImageContainer::Undefined) {
        qWarning() << "ImageContainer::load (" << fn
                   << ") into existing image of type " << imageType;
        return false;
    }

    if (fn.toLower().endsWith(".svg")) {
        svgItem = new QGraphicsSvgItem(fn, this);

        if (createClone) {
            imageType = ImageContainer::ClonedSVG;
            svgCashPath = fn;
        } else {
            imageType = ImageContainer::SVG;

            // Copy original file to cash
            QFile svgFile(fn);
            QString newPath = cashDir.path() + "/" + QString().number(imageID) +
                              "-" + basename(fn);
            if (!svgFile.copy(newPath)) {
                qWarning() << "ImageContainer::load (" << fn
                           << ") could not be copied to " << newPath;
            }

            svgCashPath = newPath;
        }   // No clone created
        setRect(svgItem->boundingRect());
    } else {
        // Not svg
        QPixmap pm;
        if (!pm.load(fn)) return false;

        if (pixmapItem)
            qWarning() << "ImageContainer::load " << fn
                       << "pixmapIteam already exists";

        imageType = ImageContainer::Pixmap;
        pixmapItem = new QGraphicsPixmapItem(this);
        pixmapItem->setPixmap(pm);

        setRect(pixmapItem->boundingRect());
    }

    return true;
}

bool ImageContainer::save(const QString &fn)
{
    switch (imageType) {
        case ImageContainer::SVG:
        case ImageContainer::ClonedSVG:
            if (svgItem) {
                QFile svgFile(svgCashPath);
                if (!QFile(fn).exists() && !svgFile.copy(fn)) {
                    qWarning() << "ImageContainer::save  failed to copy " << svgCashPath
                               << " to " << fn;
                    return false;
                }
            }
            return true;
            break;
        case ImageContainer::Pixmap:
            return pixmapItem->pixmap().save(fn, "PNG", 100);
            break;
        case ImageContainer::ModifiedPixmap:
            return originalPixmap->save(fn, "PNG", 100);
            break;
        default:
            break;
    }
    return false;
}

QString ImageContainer::getExtension()
{
    QString s;
    switch (imageType) {
        case ImageContainer::SVG:
        case ImageContainer::ClonedSVG:
            s = ".svg";
            break;
        case ImageContainer::Pixmap:
        case ImageContainer::ModifiedPixmap:
            s = ".png";
            break;
        default:
            break;
    }
    return s;
}

ImageContainer::ImageType ImageContainer::getType() { return imageType; }

QIcon ImageContainer::getIcon()
{
    switch (imageType) {
        case ImageContainer::SVG:
        case ImageContainer::ClonedSVG:
            return QPixmap(svgCashPath);
            break;
        case ImageContainer::Pixmap:
        case ImageContainer::ModifiedPixmap:
            return QIcon(pixmapItem->pixmap());
            break;
        default:
            break;
    }
    return QIcon();
}

void ImageContainer::setImageItem(ImageItem* ii) {
    imageItem = ii;
}

ImageItem* ImageContainer::getImageItem() { return imageItem;}

void ImageContainer::reposition() {}    // No action necessary
