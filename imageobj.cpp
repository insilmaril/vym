#include "imageobj.h"
#include "mapobj.h"

#include <QDebug>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QSvgGenerator>

/////////////////////////////////////////////////////////////////
// ImageObj	
/////////////////////////////////////////////////////////////////
ImageObj::ImageObj()
{
    //qDebug() << "Const ImageObj ()  this=" << this;
    init();
}

ImageObj::ImageObj( QGraphicsItem *parent) : QGraphicsItem (parent )
{
    //qDebug() << "Const ImageObj  this=" << this << "  parent= " << parent ;
    init();
}

ImageObj::~ImageObj()
{
    //qDebug() << "Destr ImageObj  this=" << this << "  imageType = " << imageType ;
    switch (imageType)
    {
        case ImageObj::SVG:
            if (svgItem) delete (svgItem);
            break;
        case ImageObj::Pixmap:
            if (pixmapItem) 
            {
                // FIXME-1 testing
                delete (pixmapItem);
                qDebug() << "  destr IO  deleting pixmapItem";
            }
            break;
        case ImageObj::ModifiedPixmap:  
            if (pixmapItem) 
            {
                // FIXME-1 testing
                delete (pixmapItem);
                qDebug() << "  destr IO  deleting pixmapItem";
            }
            if (originalPixmap) 
            {
                // FIXME-1 testing
                delete (originalPixmap);
                qDebug() << "  destr IO  deleting originalPixmap";
            }
            break;
        default: 
            qDebug() << "ImgObj::copy other->imageType undefined";    
            break;
    }
}

void ImageObj::init()
{
    // qDebug() << "Const ImageObj (scene)";

    //FIXME-1 needed? setShapeMode (QGraphicsPixmapItem::BoundingRectShape);
    hide();

    imageType = ImageObj::Undefined;
    svgItem        = NULL;
    pixmapItem     = NULL;
    originalPixmap = NULL;
    scaleFactor    = 1;
}

void ImageObj::copy(ImageObj* other)    // FIXME-1 check copying of FloatImagObj...
{
    prepareGeometryChange();
    // FIXME-0 setPixmap (other->QGraphicsPixmapItem::pixmap());	
    qDebug() << "ImgObj::copy    this=" << this << "  other=" << other << "  type this=  " << imageType  << "type other= " << other->imageType;
    if (imageType != ImageObj::Undefined)
        qWarning() << "ImageObj::copy into existing image of type " << imageType;

    switch (other->imageType)
    {
        case ImageObj::SVG:
            qDebug() << "ImgObj::copy other is svg...";    // FIXME-0 check: no deep copy?
            if (!other->svgPath.isEmpty())
            {
                qWarning() << "ImgObj::copy svg: path is " << other->svgPath;
                load(other->svgPath);
                svgItem->setVisible( isVisible());
                imageType = ImageObj::SVG;
            } else 
                qWarning() << "ImgObj::copy svg: no svgPath available.";
            break;
        case ImageObj::Pixmap:
            qDebug() << "ImgObj::copy other is pm...";    // FIXME-1 check
            pixmapItem = new QGraphicsPixmapItem();
            pixmapItem->setPixmap (other->pixmapItem->pixmap());
            pixmapItem->setParentItem (parentItem() );
            pixmapItem->setVisible( isVisible());
            imageType = ImageObj::Pixmap;
            break;
        case ImageObj::ModifiedPixmap:
            qDebug() << "ImgObj::copy other is modified pm...";    // FIXME-0 implement copy
            // create new pixmap?
            pixmapItem->setPixmap (other->pixmapItem->pixmap());
            pixmapItem->setParentItem (parentItem());
            pixmapItem->setVisible( isVisible());
            imageType = ImageObj::Pixmap;
            break;
        default: 
            qWarning() << "ImgObj::copy other->imageType undefined";   
            return;
            break;
    }
    //setPos (other->pos());        // FIXME-1 remove...
    //setZValue(other->zValue());	
}

void ImageObj::setPos(const QPointF &pos)
{
    if (!parentItem() ) // FIXME-0 testing
    {
        setVisibility(false);
        return;
    }

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
    

void ImageObj::setVisibility (bool v)   
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
        case ImageObj::SVG:
            svgItem->setScale (f);
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

QRectF ImageObj::boundingRect() const   // FIXME-0 not always correct for svg
{
    switch (imageType)
    {
        case ImageObj::SVG:
            //qDebug() << "IO::boundingRect svg " << svgItem->boundingRect();
            return QRectF(0, 0, 
                    svgItem->boundingRect().width() * scaleFactor, 
                    svgItem->boundingRect().height() * scaleFactor);
        case ImageObj::Pixmap:
            return pixmapItem->boundingRect();
        case ImageObj::ModifiedPixmap:
            return pixmapItem->boundingRect();
        default: 
            break;
    }
}

void ImageObj::paint (QPainter *painter, const QStyleOptionGraphicsItem
*sogi, QWidget *widget)     // FIXME-1 modPixmap not used?
{
    switch (imageType)
    {
        case ImageObj::SVG:
            svgItem->paint(painter, sogi, widget);
            break;
        case ImageObj::Pixmap:
            qDebug() << "IO::paint pm this=" << this  << "  pmitem=" << pixmapItem;
            pixmapItem->paint(painter, sogi, widget);
            break;
        default: 
            break;
    }
}

bool ImageObj::shareCashed(const QString &fn)
{
    qDebug() << "IO::shareCashed " << fn;
    save(fn);
    svgPath = fn;
}

QString ImageObj::getCashPath()
{
    return svgPath;
}

bool ImageObj::load (const QString &fn) 
{
    qDebug() << "IO::load "  << fn;
    if (imageType != ImageObj::Undefined)
        qWarning() << "ImageObj::load (" << fn << ") into existing image of type " << imageType;

    if (fn.toLower().endsWith(".svg"))
    {
        svgItem = new QGraphicsSvgItem(fn);
        imageType = ImageObj::SVG;
        if (scene() ) scene()->addItem (svgItem);

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

            qDebug() << "IO::load  fn=" << fn << "  pm=" << pixmapItem << "  this=" << this;  // FIXME-1 debug
            imageType = ImageObj::Pixmap;

            return true;
        }
    }
    
    return false;
}

bool ImageObj::load (const QPixmap &pm)
{
    if (imageType != ImageObj::Undefined)
        qWarning() << "ImageObj::load (pm) into existing image of type " << imageType;

    prepareGeometryChange();

    imageType = ImageObj::Pixmap;
    
    pixmapItem = new QGraphicsPixmapItem (this);    // FIXME-1 existing pmi? 
    pixmapItem->setPixmap(pm);
    pixmapItem->setParentItem(parentItem() );
    qDebug() << "IO::load  pm this=" << this << "  pm =" << pixmapItem;  // FIXME-1 debug
    return true;
}

bool ImageObj::save(const QString &fn) 
{
    qDebug() << "ImgObj::save "  << fn;     // FIXME-1
    switch (imageType)
    {
        case ImageObj::SVG:
            if (svgItem)
            {
                //qDebug() << "IO::save svg" << fn; 
                QSvgGenerator generator;
                generator.setFileName(fn);
                // generator.setTitle(originalFileName);
                generator.setDescription("An SVG drawing created by vym - view your mind");
                QStyleOptionGraphicsItem qsogi;
                QPainter painter;
                painter.begin(&generator);
                svgItem->paint(&painter, &qsogi, NULL);
                painter.end();
            }
            return true;
            break;
        case ImageObj::Pixmap:
            //qDebug() << "IO::save pixmap " << fn;
            return pixmapItem->pixmap().save (fn, "PNG", 100);
            break;
        case ImageObj::ModifiedPixmap:
            //qDebug() << "IO::save modified pixmap " << fn;
            return originalPixmap->save (fn, "PNG", 100);
            break;
        default:
            break;
    }
}

QString ImageObj::getExtension()
{
    QString s;
    switch (imageType)
    {
        case ImageObj::SVG:
            s = ".svg";
            break;
        case ImageObj::Pixmap:
        case ImageObj::ModifiedPixmap:
            s = ".png";
            break;
        default:
            break;
    }
    return s;
}

ImageObj::ImageType ImageObj::getType()
{
    return imageType;
}

QIcon ImageObj::getIcon()
{
    if (imageType == Pixmap)
        return QIcon(pixmapItem->pixmap() );
    return QIcon(); // FIXME-0  create icon for svg
}

// FIXME-1 is originalPixmap used after all?
// FIXME-1 Solved: Activating 1 userflag creates 3 objects in scene: FlagObj, ImageObj, QGraphicsItem in ImgObj
