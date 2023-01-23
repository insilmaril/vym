#include "image-container.h"

#include <QDebug>
#include <QIcon>

#include "file.h"
#include "imageitem.h"
#include "mapdesign.h"

extern QDir cacheDir;
extern ulong imageLastID;

/////////////////////////////////////////////////////////////////
// ImageContainer
/////////////////////////////////////////////////////////////////
ImageContainer::ImageContainer()
{
    //qDebug() << "Const ImageContainer ()  this=" << this;
    init();
}

ImageContainer::~ImageContainer()
{
    //qDebug() << "Destr ImageContainer  this=" << this << "  imageType = " << imageType ;
    if (imageItem) imageItem->unlinkImageContainer();
}

void ImageContainer::copy(ImageContainer *other)
{
    prepareGeometryChange();
    if (imageType != ImageContainer::Undefined)
        qWarning() << "ImageContainer::copy into existing image of type "
                   << imageType;

    switch (other->imageType) {
        case ImageContainer::SVG:
        case ImageContainer::ClonedSVG:
            if (!other->svgCachePath.isEmpty()) {
                // Loading here will also set imageType
                load(other->svgCachePath, true);
            }
            else
                qWarning() << "ImgObj::copy svg: no svgCachePath available.";

            svgItem->setVisible(isVisible());
            svgItem->setParentItem(this);
            setRect(svgItem->boundingRect());
            break;
        case ImageContainer::Pixmap:
            pixmapItem = new QGraphicsPixmapItem();
            pixmapItem->setPixmap(other->pixmapItem->pixmap());
            pixmapItem->setParentItem(this); // FIXME-2 check...
            pixmapItem->setVisible(isVisible());
            //pixmapItem->setOffset(-20,-20);  // FIXME-2 testing
            setRect(pixmapItem->boundingRect());
            imageType = ImageContainer::Pixmap;
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
    setScaleFactor(other->scaleFactor);
}

void ImageContainer::init()
{
    containerType = Image;

    imageItem = nullptr;
    selectionContainer = nullptr;

    // Assign ID
    imageLastID++;
    imageID = imageLastID;

    imageType = ImageContainer::Undefined;
    layout =  Container::Horizontal;

    svgItem = nullptr;
    pixmapItem = nullptr;
    originalPixmap = nullptr;
    scaleFactor = 1;
    overlay = false;    // Inherits FrameContainer, which has overlay == true

     // FIXME-3 for testing we do some coloring and additional drawing
    //setPen(QPen(Qt::red));
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

void ImageContainer::setWidth(qreal w)
{
    if (boundingRect().width() == 0)
        return;

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

void ImageContainer::select()
{
    SelectableContainer::select(
	    this,
	    imageItem->getMapDesign()->selectionColor());
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
            svgCachePath = fn;
        } else {
            imageType = ImageContainer::SVG;

            // Copy original file to cache
            QFile svgFile(fn);
            QString newPath = cacheDir.path() + "/" + QString().number(imageID) +
                              "-" + basename(fn);
            if (!svgFile.copy(newPath)) {
                qWarning() << "ImageContainer::load (" << fn
                           << ") could not be copied to " << newPath;
            }

            svgCachePath = newPath;
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
                QFile svgFile(svgCachePath);
                if (!QFile(fn).exists() && !svgFile.copy(fn)) {
                    qWarning() << "ImageContainer::save  failed to copy " << svgCachePath
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
            return QPixmap(svgCachePath);
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

void ImageContainer::reposition() {}    // No action necessary  // FIXME-2 remove then?
