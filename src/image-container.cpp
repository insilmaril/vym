#include "image-container.h"

#include <QDebug>
#include <QIcon>

#include "branch-container.h"
#include "branchitem.h"
#include "file.h"
#include "imageitem.h"
#include "link-container.h"
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

            svgItem->setParentItem(this);
            setRect(other->rect());
            break;
        case ImageContainer::Pixmap: {
            pixmapItem = new QGraphicsPixmapItem();
            pixmapItem->setPixmap(other->pixmapItem->pixmap());
            pixmapItem->setParentItem(this);
            QRectF r = other->pixmapItem->boundingRect();
            pixmapItem->setPos( - r.width() / 2, - r.height() /2);
            setRect(other->rect());
            imageType = ImageContainer::Pixmap;
            }
            break;
        default:
            qWarning() << "ImgObj::copy other->imageType undefined";
            return;
            break;
    }
    setScale(other->scaleFactorInt);
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
    layoutInt =  Container::Horizontal;

    svgItem = nullptr;
    pixmapItem = nullptr;
    scaleFactorInt = 1;
    overlay = false;    // Inherits FrameContainer, which has overlay == true

    // FIXME-3 for testing we do some coloring and additional drawing
    //setPen(QPen(Qt::white));
}

void ImageContainer::setWidth(qreal w)
{
    if (boundingRect().width() == 0)
        return;

    setScale(w / boundingRect().width());
}

void ImageContainer::setScale(qreal f)
{
    scaleFactorInt = f;
    QGraphicsItem::setScale(f);
}

qreal ImageContainer::scale() { return scaleFactorInt; }

void ImageContainer::select()
{
    SelectableContainer::select(
	    this,
	    imageItem->mapDesign()->selectionPen(),
	    imageItem->mapDesign()->selectionBrush());
    float d = 10;
    QRectF r = rect();
    selectionContainer->setRect(QRectF(r.x() - d, r.y() - d, r.width() + 2 * d, r.height() + 2 * d));
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
        QRectF r = svgItem->boundingRect();

        // Center svg
        svgItem->setPos( -r.width() / 2, - r.height() / 2);
        setRect(mapFromItem(svgItem, svgItem->boundingRect()).boundingRect());
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

        // Center svg
        QRectF r = pixmapItem->boundingRect();
        pixmapItem->setPos( -r.width() / 2, - r.height() / 2);
        setRect(mapFromItem(pixmapItem, pixmapItem->boundingRect()).boundingRect());
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
        case ImageContainer::Pixmap: {
            bool b = pixmapItem->pixmap().save(fn, "PNG", 100);
            return b;
                                     }
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

void ImageContainer::linkTo(BranchContainer *pbc)
{
    if (!pbc)
        return;

    pbc->getLinkContainer()->addLink(upLink);
}

void ImageContainer::updateUpLink()
{
    /*
    if (imageItem)
        qDebug() << "IC::updateUpLink()  ii=" << imageItem->headingText()  << "  vis=" << isVisible() << " par_item=" << parentItem();
    else
        qDebug() << "IC::updateUpLink() No ii.";
    */

    // Sets geometry
    // Called from MapEditor e.g. during animation or
    // from VymModel, when colors change

    if (!isVisible())
        return;

    QPointF upLinkSelf_sp = mapToScene(center());   // upLinkPos();     // FIXME-4 use nearest corner?
    QPointF downLink_sp = mapToScene(center());     // downLinkPos();   // Could be needed for bottomline later.

    BranchContainer *pbc = nullptr;
    if (imageItem)
        pbc = imageItem->parentBranch()->getBranchContainer();

    if (pbc) {
        QPointF upLinkParent_sp;

        upLinkParent_sp = pbc->downLinkPos();

        QGraphicsItem *upLinkParent = upLink->parentItem();
        if (!upLinkParent)
            return;

        upLink->setUpLinkPosParent(
            upLinkParent->sceneTransform().inverted().map(upLinkParent_sp));
        upLink->setUpLinkPosSelf(
            upLinkParent->sceneTransform().inverted().map(upLinkSelf_sp));
        upLink->setDownLinkPos(
            upLinkParent->sceneTransform().inverted().map(downLink_sp));
    }
    else {
        // FIXME-3  flags have no upLink...   qWarning() << "ImageContainer::updateUpLink - No parent branch container ?!";
        return;
    }

    // Color of link (depends on current parent)
    BranchItem *pb = imageItem->parentBranch();
    if (pb) {
        if (upLink->getLinkColorHint() == LinkObj::HeadingColor)
            upLink->setLinkColor(pb->headingColor());
        else
            upLink->setLinkColor(pb->mapDesign()->defaultLinkColor());
    }

    // Finally update geometry
    upLink->updateLinkGeometry();
}

void ImageContainer::updateVisibility()
{
    //qDebug() << "IC::updateVisibility";
    if (imageItem->hideLinkUnselected()) {
        if (!SelectableContainer::isSelected())
            upLink->setVisible(false);
        else
            upLink->setVisible(true);
    } else
        upLink->setVisible(true);
}

void ImageContainer::reposition()
{
    updateUpLink();
}

