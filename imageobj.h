#ifndef IMAGEOBJ_H
#define IMAGEOBJ_H

#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsSvgItem>

/*! \brief Base class for images, which can be pixmaps or svg
 *
 * ImageObj is used both by items part of the map "tree" in 
 * ImageItem and as flag in FlagObj
 *
 * Both of these types are actually drawn onto the map
*/

class ImageObj: public QGraphicsItem
{
public:
    enum ImageType {Undefined, Pixmap, SVG};

    ImageObj( QGraphicsItem*);
    ~ImageObj();
    void copy (ImageObj*);
    void setVisibility(bool);
    virtual QRectF boundingRect() const;
    virtual void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*);
    void save (const QString &, const char *);
    bool load (const QString &);
    bool load (const QPixmap &);

private:
     ImageObj::ImageType imageType;

     QGraphicsSvgItem svgItem;
     QGraphicsPixmapItem pixmapItem;
};
#endif
