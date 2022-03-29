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
ImageContainer::ImageContainer(QGraphicsScene *scene)
{
    qDebug() << "Const ImageContainer ()  this=" << this << "items=" << scene->items().count();
    scene->addItem(this);
    init();
}

ImageContainer::~ImageContainer()
{
    qDebug() << "Destr ImageContainer  this=" << this << "  imageType = " << imageType ;
    qDebug() << " - svgItem:    " << svgItem;
    qDebug() << " - childItems: " << childItems();
    if (svgItem && !childItems().contains(svgItem))
        qDebug() << " - childItems does not contain svgItem: " << svgItem;

    switch (imageType) {    // FIXME-0 all childItems should have myself as parent and would be deleted automatically
        case ImageContainer::SVG:
        case ImageContainer::ClonedSVG:
            if (svgItem)
                delete (svgItem);
            break;
        case ImageContainer::Pixmap:
            if (pixmapItem)
                delete (pixmapItem);
            break;
        case ImageContainer::ModifiedPixmap:
            if (pixmapItem)
                delete (pixmapItem);
            if (originalPixmap)
                delete (originalPixmap);
            break;
        default:
            qDebug() << "Destr ImgObj: imageType undefined";
            break;
    }
    qDebug() << "Destr ImageContainer  this=" << this << " finished";
    qDebug() << " - childItems: " << childItems();
}

void ImageContainer::copy(ImageContainer *other)
{
    qDebug() << "IC::copy"; // FIXME-2 testing only
    prepareGeometryChange();
    if (imageType != ImageContainer::Undefined)
        qWarning() << "ImageContainer::copy into existing image of type "
                   << imageType;

    switch (other->imageType) {
        case ImageContainer::SVG:
        case ImageContainer::ClonedSVG:
            if (!other->svgCashPath.isEmpty()) {
                load(other->svgCashPath, true);
            }
            else
                qWarning() << "ImgObj::copy svg: no svgCashPath available.";

            svgItem->setVisible(isVisible());
            break;
        case ImageContainer::Pixmap:
            pixmapItem = new QGraphicsPixmapItem();
            pixmapItem->setPixmap(other->pixmapItem->pixmap());
            pixmapItem->setParentItem(parentItem());
            pixmapItem->setVisible(isVisible());
            imageType = ImageContainer::Pixmap;
            break;
        case ImageContainer::ModifiedPixmap:
            // create new pixmap?
            pixmapItem->setPixmap(other->pixmapItem->pixmap());
            pixmapItem->setParentItem(parentItem());
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
    type = Image;

    // Assign ID
    imageLastID++;
    imageID = imageLastID;

    imageType = ImageContainer::Undefined;
    svgItem = nullptr;
    pixmapItem = nullptr;
    originalPixmap = nullptr;
    scaleFactor = 1;
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
            case ImageContainer::ClonedSVG:
                svgItem->setScale(f);
                break;
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

void ImageContainer::updateRect()
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

    qDebug() << "IC::load" <<fn << "createClone=" << createClone; 

    if (imageType != ImageContainer::Undefined) {
        qWarning() << "ImageContainer::load (" << fn
                   << ") into existing image of type " << imageType;
        return false;
    }

    if (fn.toLower().endsWith(".svg")) {
        qDebug() << " - IC::load  childItems=" << childItems();
        svgItem = new QGraphicsSvgItem(fn);
        qDebug() << " - IC::load created svg=" << svgItem;
        svgItem->setParentItem(this);
        qDebug() << " - IC::load relinked  svg=" << svgItem;
        qDebug() << " - IC::load  childItems=" << childItems();
        qDebug() << " - IC::load  cI.first  =" << childItems().first();
        if (childItems().first() != svgItem)
            qDebug() << " - IC::load  childItems.first != svgItem=";
        else
            qDebug() << " - IC::load  childItems.first == svgItem=";
        svgItem->setPos(-200,0);
        qDebug() << "ok-1";
        childItems().first()->setPos(200,0);
        delete svgItem;
        svgItem = nullptr;
        svgItem = (QGraphicsSvgItem*)(childItems().first());
        qDebug() << " - IC::load updated svg=" << svgItem;
        //qDebug() << " - IC::load  childItems=" << childItems();

        qDebug() << "ok0";

        if (createClone) {
            imageType = ImageContainer::ClonedSVG;
            svgCashPath = fn;
        }
        else {
            qDebug() << "ok1";
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
            qDebug() << "ok2";
        }   // No clone created
    } else {
        QPixmap pm;
        if (!pm.load(fn)) return false;

        prepareGeometryChange();

        if (pixmapItem)
            qWarning() << "ImageContainer::load " << fn
                       << "pixmapIteam already exists";

        imageType = ImageContainer::Pixmap;
        pixmapItem = new QGraphicsPixmapItem(this);
        pixmapItem->setPixmap(pm);
        pixmapItem->setParentItem(parentItem());

        qDebug() << "IC::load created  pixmapItem=" << pixmapItem;
    }

    qDebug() << "ok3";
    //updateRect();
    setRect(-50,-50,100,100);
    qDebug() << "ok4";
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

void ImageContainer::reposition()
{
    qDebug() << "ImageContainer::reposition type" << imageType;
    if (pixmapItem) qDebug() << " - pixmapItem=" << pixmapItem;
    if (svgItem) qDebug() << " - svgItem=" << svgItem;
    qDebug() << " - childItems: " << childItems();
}
