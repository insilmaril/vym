#ifndef IMAGE_CONTAINER_H
#define IMAGE_CONTAINER_H

#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsSvgItem>

#include "container.h"
#include "selectable-container.h"

class ImageItem;

/*! \brief Base class for images in containers, which can be pixmaps or svg
 *
 * ImageContainer is used both by items part of the map "tree" in
 * a ImageItem and also as flag  part of a FlagRow
 *
 * Both of these types are actually drawn onto the map
 */

class ImageContainer : public SelectableContainer {
  public:
    enum ImageType { Undefined, Pixmap, ModifiedPixmap, SVG, ClonedSVG };

    ImageContainer();
    virtual ~ImageContainer();
    virtual void copy(ImageContainer*);
    virtual void init();
    void setVisibility(bool);
    void setWidth(qreal w);
    void setScaleFactor(qreal f);
    qreal getScaleFactor();
    void select();

    bool load(const QString &, bool createClone = false);
    bool save(const QString &);
    QString getExtension();
    ImageType getType();
    QIcon getIcon();

    void setImageItem(ImageItem*);
    ImageItem* getImageItem();

    void reposition();

  protected:
    ImageContainer::ImageType imageType;

    QGraphicsSvgItem *svgItem;
    QString svgCachePath;

    QGraphicsPixmapItem *pixmapItem;
    QPixmap *originalPixmap;

    qreal scaleFactor;

    ulong imageID;

    ImageItem *imageItem; // FIXME-2 only used when part of ImageItem
};
#endif
