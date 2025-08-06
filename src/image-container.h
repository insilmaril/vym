#ifndef IMAGE_CONTAINER_H
#define IMAGE_CONTAINER_H

#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsSvgItem>

#include "container.h"
#include "linkable-container.h"
#include "selectable-container.h"

class ImageItem;

/*! \brief Base class for images in containers, which can be pixmaps or svg
 *
 * ImageContainer is used both by items part of the map "tree" in
 * a ImageItem and also as flag  part of a FlagRow
 *
 * Both of these types are actually drawn onto the map
 */

class ImageContainer : public Container, public LinkableContainer, public SelectableContainer {
  public:
    enum ImageType { Undefined, Pixmap, SVG, ClonedSVG };

    ImageContainer();
    virtual ~ImageContainer();
    virtual QSvgRenderer* svgRenderer();
    virtual void copy(ImageContainer*);
    virtual void init();
    void setWidth(qreal w);
    void setScale(qreal f);
    qreal scale();
    void select();

    bool load(const QString &);
    bool save(const QString &);
    void setOriginalFilename(const QString &);
    QString originalFilename();
    QString extension();
    ImageType getType();
    QIcon getIcon();

    void setImageItem(ImageItem*);
    ImageItem* getImageItem();

    void linkTo(BranchContainer *pbc);
    void updateUpLink();
    void updateVisibility();
    void reposition();

  protected:
    ImageContainer::ImageType imageType;

    QString originalFilenameInt;

    QGraphicsSvgItem *svgItem;
    QString svgCachePath;

    QGraphicsPixmapItem *pixmapItem;

    qreal scaleFactorInt;

    ulong imageID;

    ImageItem *imageItem; // only used when part of ImageItem, not in FlagContainer
};
#endif
