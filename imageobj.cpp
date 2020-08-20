#include "imageobj.h"
#include "mapobj.h"

#include <QDebug>

/////////////////////////////////////////////////////////////////
// ImageObj	
/////////////////////////////////////////////////////////////////
ImageObj::ImageObj( QGraphicsItem *parent) : QGraphicsItem (parent )
{
    // qDebug() << "Const ImageObj (scene)";

    //FIXME-0 setShapeMode (QGraphicsPixmapItem::BoundingRectShape);
    hide();

    imageType = ImageObj::Undefined;
    svgItem        = NULL;
    pixmapItem     = NULL;
    originalPixmap = NULL;
    scaleFactor    = 1;
}

ImageObj::~ImageObj()
{
    //qDebug() << "Destr ImageObj  imageType = " << imageType ;
    switch (imageType)
    {
        case ImageObj::SVG:
            if (svgItem) delete (svgItem);
            break;
        case ImageObj::Pixmap:
            if (pixmapItem) delete (pixmapItem);
            break;
        case ImageObj::ModifiedPixmap:  
            if (pixmapItem) delete (pixmapItem);
            if (originalPixmap) delete (originalPixmap);
            break;
        default: 
            qDebug() << "ImgObj::copy other->imageType undefined";    
            break;
    }
}

void ImageObj::copy(ImageObj* other)
{
    prepareGeometryChange();
    // FIXME-0 setPixmap (other->QGraphicsPixmapItem::pixmap());	
    qDebug() << "IO::copy    types: this = " << imageType  << "other = " << other->imageType;
    switch (other->imageType)
    {
        case ImageObj::SVG:
            qDebug() << "ImgObj::copy called svg...";    // FIXME-1 check
            svgItem = new QGraphicsSvgItem(other->svgItem);
            break;
        case ImageObj::Pixmap:
            qDebug() << "ImgObj::copy called pm...";    // FIXME-1 check
            pixmapItem->setPixmap (other->pixmapItem->pixmap());
            pixmapItem->setParentItem (other->parentItem());
            break;
        case ImageObj::ModifiedPixmap:
            qDebug() << "ImgObj::copy called mpm...";    // FIXME-0 implement copy
            pixmapItem->setPixmap (other->pixmapItem->pixmap());
            pixmapItem->setParentItem (other->parentItem());
            break;
        default: 
            qWarning() << "ImgObj::copy other->imageType undefined";   
            return;
            break;
    }
    setPos (other->pos());
    setVisibility (other->isVisible() );
    setZValue(other->zValue());	
}

void ImageObj::setPos(const QPointF &pos)
{
    switch (imageType)
    {
        case ImageObj::SVG:
            svgItem->setPos(pos);
            break;
        case ImageObj::Pixmap:
            pixmapItem->setPos(pos);
            break;
        case ImageObj::ModifiedPixmap:
            pixmapItem->setPos(pos);
            break;
        default: 
            break;
    }
}

void ImageObj::setPos(const qreal &x, const qreal &y)
{
    setPos (QPointF (x, y));
}

void ImageObj::setZValue (qreal z)
{
    switch (imageType)
    {
        case ImageObj::SVG:
            svgItem->setZValue(z);
            break;
        case ImageObj::Pixmap:
        case ImageObj::ModifiedPixmap:
            pixmapItem->setZValue(z);
            break;
        default: 
            break;
    }
}
    

void ImageObj::setVisibility (bool v)   // FIXME-0 add pixmap, svg
{
    switch (imageType)
    {
        case ImageObj::SVG:
            v ? svgItem->show() : svgItem->hide();
            break;
        case ImageObj::Pixmap:
        case ImageObj::ModifiedPixmap:
            v ? pixmapItem->show() : pixmapItem->hide();
            break;
        default: 
            break;
    }
}

void  ImageObj::setScaleFactor(qreal f) 
{
    scaleFactor = f;
    switch (imageType)
    {
        case ImageObj::SVG:// FIXME-0 not implemented
            break;
        case ImageObj::Pixmap: 
            if (f != 1 )
            {
                // create ModifiedPixmap
                originalPixmap = new QPixmap (pixmapItem->pixmap());
                imageType = ModifiedPixmap;

                setScaleFactor (f);
            }
            break;
        case ImageObj::ModifiedPixmap:  
            if (!originalPixmap)
            {
                qWarning() << "ImageObj::setScaleFactor   no originalPixmap!";
                return;
            }
            pixmapItem->setPixmap(
                    originalPixmap->scaled( 
                        originalPixmap->width() * f, 
                        originalPixmap->height() * f));
            break;
        default: 
            break;
    }
}

qreal  ImageObj::getScaleFactor()
{
    return scaleFactor;
}

QRectF ImageObj::boundingRect() const 
{
    switch (imageType)
    {
        case ImageObj::SVG:
            return svgItem->boundingRect();
        case ImageObj::Pixmap:
            return pixmapItem->boundingRect();
        case ImageObj::ModifiedPixmap:
            return pixmapItem->boundingRect();
        default: 
            break;
    }
}

void ImageObj::paint (QPainter *painter, const QStyleOptionGraphicsItem
*sogi, QWidget *widget)     // FIXME-4 not used?!
{
    qDebug() << "IO::paint";
    switch (imageType)
    {
        case ImageObj::SVG:
            svgItem->paint(painter, sogi, widget);
            break;
        case ImageObj::Pixmap:
            pixmapItem->paint(painter, sogi, widget);
            break;
        default: 
            break;
    }
}

bool ImageObj::load (const QString &fn) 
{
    if (fn.toLower().endsWith(".svg"))
    {
        svgItem = new QGraphicsSvgItem(fn);
        imageType = ImageObj::SVG;
        scene()->addItem (svgItem);

        return true;
    } else
    {
        QPixmap pm;
        if (pm.load (fn))
        {
            prepareGeometryChange();

            pixmapItem = new QGraphicsPixmapItem (this);    // FIXME-1 existing pmi? 
            pixmapItem->setPixmap (pm);
            pixmapItem->setParentItem(parentItem() );

            imageType = ImageObj::Pixmap;

            return true;
        }
    }
    
    return false;
}

bool ImageObj::load (const QPixmap &pm)
{
    prepareGeometryChange();

    imageType = ImageObj::Pixmap;
    
    pixmapItem = new QGraphicsPixmapItem (this);    // FIXME-1 existing pmi? 
    pixmapItem->setPixmap(pm);
    pixmapItem->setParentItem(parentItem() );
    return true;
}

bool ImageObj::save(const QString &fn)  // FIXME-0 evtl. add ".png" to name
{
    switch (imageType)
    {
        case ImageObj::SVG:
            if (svgItem)
                qDebug() << "II::save svg - not implemented"; // FIXME-0 not implemented 
            return false;
            break;
        case ImageObj::Pixmap:
            qDebug() << "II::save pixmap";
            return pixmapItem->pixmap().save (fn, "PNG", 100);
            break;
        case ImageObj::ModifiedPixmap:
            qDebug() << "II::save modified pixmap";
            return originalPixmap->save (fn, "PNG", 100);
            break;
        default:
            break;
    }
}

