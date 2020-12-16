#include "imageobj.h"

#include "file.h"
#include "mapobj.h"

#include <QDebug>
#include <QDir>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QSvgGenerator>

extern QDir cashDir;
extern ulong imageLastID;

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
        case ImageObj::ClonedSVG:
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
            qDebug() << "Destr ImgObj: imageType undefined";    
            break;
    }
}

void ImageObj::init()
{
    // qDebug() << "Const ImageObj (scene)";
    hide();

    // Assign ID
    imageLastID++;
    imageID = imageLastID;

    imageType = ImageObj::Undefined;
    svgItem        = NULL;
    pixmapItem     = NULL;
    originalPixmap = NULL;
    scaleFactor    = 1;
}

void ImageObj::copy(ImageObj* other)    
{
    prepareGeometryChange();
    if (imageType != ImageObj::Undefined)
        qWarning() << "ImageObj::copy into existing image of type " << imageType;

    switch (other->imageType)
    {
        case ImageObj::SVG:
        case ImageObj::ClonedSVG:
            if (!other->svgCashPath.isEmpty())
            {
                load(other->svgCashPath, true);
            } else 
                qWarning() << "ImgObj::copy svg: no svgCashPath available.";

            svgItem->setVisible( isVisible());
            break;
        case ImageObj::Pixmap:
            pixmapItem = new QGraphicsPixmapItem();
            pixmapItem->setPixmap (other->pixmapItem->pixmap());
            pixmapItem->setParentItem (parentItem() );
            pixmapItem->setVisible( isVisible());
            imageType = ImageObj::Pixmap;
            break;
        case ImageObj::ModifiedPixmap:
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
    setScaleFactor (other->scaleFactor);
}

void ImageObj::setPos(const QPointF &pos)
{
    switch (imageType)
    {
        case ImageObj::SVG:
        case ImageObj::ClonedSVG:
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
        case ImageObj::ClonedSVG:
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
        case ImageObj::ClonedSVG:
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

void ImageObj::setWidth(qreal w)  
{
    if (boundingRect().width() == 0) return;

    setScaleFactor ( w / boundingRect().width());
}

void ImageObj::setScaleFactor(qreal f) 
{
    scaleFactor = f;
    switch (imageType)
    {
        case ImageObj::SVG:
        case ImageObj::ClonedSVG:
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

QRectF ImageObj::boundingRect() const   
{
    switch (imageType)
    {
        case ImageObj::SVG:
        case ImageObj::ClonedSVG:
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
    return QRectF();
}

void ImageObj::paint (QPainter *painter, const QStyleOptionGraphicsItem
*sogi, QWidget *widget)     
{
    // Not really called, but required because paint is pure virtual in QGraphicsItem
    
    switch (imageType)
    {
        case ImageObj::SVG:
        case ImageObj::ClonedSVG:
            svgItem->paint(painter, sogi, widget);
            break;
        case ImageObj::Pixmap:
        case ImageObj::ModifiedPixmap:
            pixmapItem->paint(painter, sogi, widget);
            break;
        default: 
            break;
    }
}

bool ImageObj::load (const QString &fn, bool createClone) 
{
    // createClone == true, if called via copy()
    
    if (imageType != ImageObj::Undefined)
    {
        qWarning() << "ImageObj::load (" << fn << ") into existing image of type " << imageType;
        return false;
    }

    if (fn.toLower().endsWith(".svg"))
    {
        svgItem = new QGraphicsSvgItem(fn);
        if (scene() ) scene()->addItem (svgItem);

        if (createClone)
        {
            imageType = ImageObj::ClonedSVG;
            svgCashPath = fn;
        } else
        {
            imageType = ImageObj::SVG;

            // Copy original file to cash
            QFile svgFile (fn);
            QString newPath = cashDir.path() + "/" + QString().number(imageID) + "-" + basename(fn);
            if (!svgFile.copy (newPath))
            {
                qWarning() << "ImageObj::load (" << fn << ") could not be copied to " << newPath;
            }

            svgCashPath = newPath;
        }

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

bool ImageObj::save(const QString &fn) 
{
    switch (imageType)
    {
        case ImageObj::SVG:
        case ImageObj::ClonedSVG:
            if (svgItem)
            {
                QFile svgFile(svgCashPath);
                if(!svgFile.copy(fn))
                {
                    qWarning() << "ImageObj::save  failed to copy " << svgCashPath << " to " << fn;
                    return false;
                }

                /*   
                 *   Old code to write svg, but Qt cannot write linearGradients correctly
                 *
                QSvgGenerator generator;
                generator.setFileName(fn);
                // generator.setTitle(originalFileName);
                generator.setDescription("An SVG drawing created by vym - view your mind");
                QStyleOptionGraphicsItem qsogi;
                QPainter painter;
                painter.begin(&generator);
                svgItem->paint(&painter, &qsogi, NULL);
                painter.end();
                */
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
    return false;
}

QString ImageObj::getExtension()
{
    QString s;
    switch (imageType)
    {
        case ImageObj::SVG:
        case ImageObj::ClonedSVG:
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
    switch (imageType)
    {
        case ImageObj::SVG:
        case ImageObj::ClonedSVG:
            return QPixmap(svgCashPath);
            break;
        case ImageObj::Pixmap:
        case ImageObj::ModifiedPixmap:
            return QIcon(pixmapItem->pixmap() );
            break;
        default:
            break;
    }
    return QIcon(); 
}
