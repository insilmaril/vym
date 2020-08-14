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
    setZValue(dZ_FLOATIMG);	
    hide();

    imageType = ImageObj::Undefined;
}

ImageObj::~ImageObj()
{
 //  qDebug() << "Destr ImageObj";
}

void ImageObj::copy(ImageObj* other)
{
    qDebug() << "ImgObj::copy called...";    // FIXME-0 testing
    prepareGeometryChange();
    setVisibility (other->isVisible() );
    // FIXME-0 setPixmap (other->QGraphicsPixmapItem::pixmap());	
    setPos (other->pos());
}

void ImageObj::setVisibility (bool v)   // FIXME-0 add pixmap, svg
{
    if (v)
        show();
    else
        hide();
}

QRectF ImageObj::boundingRect() const // FIXME-0 add svg
{
    switch (imageType)
    {
        case ImageObj::Pixmap:
            return pixmapItem.boundingRect();
        break;
        default: 
        break;
    }
}

void ImageObj::paint (QPainter *painter, const QStyleOptionGraphicsItem
*sogi, QWidget *widget)  
{
    switch (imageType)
    {
        case ImageObj::Pixmap:
            pixmapItem.paint(painter, sogi, widget);
        break;
        default: 
        break;
    }
}

void ImageObj::save(const QString &fn, const char *format)
{
    //FIXME-0 pixmapItem().save (fn,format,100);
}

bool ImageObj::load (const QString &fn) // FIXME-0 allow also svg
{
    // FIXME-0  add svg

    if (true)
    {
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
    prepareGeometryChange();

    imageType = ImageObj::Pixmap;
    
    pixmapItem.setPixmap(pm);
    return true;
}

