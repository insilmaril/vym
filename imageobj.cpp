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
}

ImageObj::~ImageObj()
{
 //  qDebug() << "Destr ImageObj";
}

void ImageObj::copy(ImageObj* other)
{
    prepareGeometryChange();
    // FIXME-0 setPixmap (other->QGraphicsPixmapItem::pixmap());	
    qDebug() << "IO::copy    types: this = " << imageType  << "other = " << other->imageType;
    switch (other->imageType)
    {
        case ImageObj::SVG:
            qDebug() << "ImgObj::copy called svg...";    // FIXME-0 testing
            svgItem = new QGraphicsSvgItem(other->svgItem);
            svgItem->setZValue(other->zValue());	
            break;
        case ImageObj::Pixmap:
            qDebug() << "ImgObj::copy called pm...";    // FIXME-0 testing
            pixmapItem.setPixmap (other->pixmapItem.pixmap());
            pixmapItem.setParentItem (other->parentItem());
            pixmapItem.setZValue(dZ_FLOATIMG);	
            pixmapItem.setZValue(other->zValue());	
            break;
        default: 
            qDebug() << "ImgObj::copy other->imageType undefined";    // FIXME-0 testing
            break;
    }
    setPos (other->pos());
    setVisibility (other->isVisible() );
}

void ImageObj::setPos(const QPointF &pos)
{
    switch (imageType)
    {
        case ImageObj::SVG:
            svgItem->setPos(pos);
            break;
        case ImageObj::Pixmap:
            pixmapItem.setPos(pos);
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
            pixmapItem.setZValue(z);
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
            v ? pixmapItem.show() : pixmapItem.hide();
            break;
        default: 
            break;
    }
}

QRectF ImageObj::boundingRect() const 
{
    switch (imageType)
    {
        case ImageObj::SVG:
            return svgItem->boundingRect();
        case ImageObj::Pixmap:
            return pixmapItem.boundingRect();
        default: 
            break;
    }
}

void ImageObj::paint (QPainter *painter, const QStyleOptionGraphicsItem
*sogi, QWidget *widget)     // FIXME-4 not used?!
{
    switch (imageType)
    {
        case ImageObj::SVG:
            svgItem->paint(painter, sogi, widget);
            break;
        case ImageObj::Pixmap:
            pixmapItem.paint(painter, sogi, widget);
            break;
        default: 
            break;
    }
}

void ImageObj::save(const QString &fn, const char *format)
{
    qDebug() << "IO::save " << fn;
    switch (imageType)
    {
        case ImageObj::SVG:
            if (svgItem)
                qDebug() << "II::save svg"; // FIXME-0 not implemented FIXME-0 and not called
            break;
        case ImageObj::Pixmap:
            qDebug() << "II::save image";
            pixmapItem.pixmap().save (fn, format, 100);
            break;
        default:
            break;
    }
}

bool ImageObj::load (const QString &fn) // FIXME-0 allow also svg
{
    if (fn.toLower().endsWith(".svg"))
    {
        qDebug() << "ImageObj::load svg " << fn;

        svgItem = new QGraphicsSvgItem(fn);
        imageType = ImageObj::SVG;
        scene()->addItem (svgItem);

        return true;
    } else
    {
        qDebug() << "ImageObj::load pixmap" << fn;

        QPixmap pm;
        if (pm.load (fn))
        {
            prepareGeometryChange();
            pixmapItem.setPixmap (pm);

            imageType = ImageObj::Pixmap;

            return true;
        }
    }
    
    return false;
}

bool ImageObj::load (const QPixmap &pm)
{
    qDebug() << "IO::load pm";
    prepareGeometryChange();

    imageType = ImageObj::Pixmap;
    
    pixmapItem.setPixmap(pm);
    pixmapItem.setParentItem(parentItem() );
    return true;
}

