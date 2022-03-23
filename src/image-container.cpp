#include "image-container.h"

#include <QDebug>
#include <QIcon>

#include "file.h"

extern QDir cashDir;
extern ulong imageLastID;

/////////////////////////////////////////////////////////////////
// ImageContainer
/////////////////////////////////////////////////////////////////
ImageContainer::ImageContainer(QGraphicsScene *scene)
{
    qDebug() << "Const ImageContainer ()  this=" << this;
    scene->addItem(this);
    init();
}

ImageContainer::~ImageContainer()
{
    qDebug() << "Destr ImageContainer  this=" << this << "  imageType = " << imageType ;
    switch (imageType) {
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
    qDebug() << "Const ImageContainer (scene)";
    hide();

    // Assign ID
    imageLastID++;
    imageID = imageLastID;

    imageType = ImageContainer::Undefined;
    svgItem = nullptr;
    pixmapItem = nullptr;
    originalPixmap = nullptr;
    scaleFactor = 1;
}

void ImageContainer::setPos(const QPointF &pos)
{
    switch (imageType) {
    case ImageContainer::SVG:
    case ImageContainer::ClonedSVG:
        svgItem->setPos(pos);
        break;
    case ImageContainer::Pixmap:
        pixmapItem->setPos(pos);
        break;
    case ImageContainer::ModifiedPixmap:
        pixmapItem->setPos(pos);
        break;
    default:
        break;
    }
}

void ImageContainer::setPos(const qreal &x, const qreal &y) { setPos(QPointF(x, y)); }

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
    setRect(r);
}

void ImageContainer::paint(QPainter *painter, const QStyleOptionGraphicsItem *sogi,
                     QWidget *widget)
{
    // Not really called, but required because paint is pure virtual in  QGraphicsItem // FIXME-2 really still needed?

    switch (imageType) {
    case ImageContainer::SVG:
    case ImageContainer::ClonedSVG:
        svgItem->paint(painter, sogi, widget);
        break;
    case ImageContainer::Pixmap:
    case ImageContainer::ModifiedPixmap:
        pixmapItem->paint(painter, sogi, widget);
        break;
    default:
        break;
    }
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
        if (scene())
            scene()->addItem(svgItem);

        if (createClone) {
            imageType = ImageContainer::ClonedSVG;
            svgCashPath = fn;
        }
        else {
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
        }

        return true;
    }
    else {
        QPixmap pm;
        if (pm.load(fn)) {
            prepareGeometryChange();

            if (pixmapItem)
                qWarning() << "ImageContainer::load " << fn
                           << "pixmapIteam already exists";
            pixmapItem = new QGraphicsPixmapItem();
            pixmapItem->setPixmap(pm);
            pixmapItem->setParentItem(parentItem());
            imageType = ImageContainer::Pixmap;

            // FIXME-2testing...
            qDebug() << "IC::load succes pix=" << pixmapItem << "scene: " << scene();
            scene()->addItem(pixmapItem);
            pixmapItem->show();


            return true;
        }
    }

    return false;
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
