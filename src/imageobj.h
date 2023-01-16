#ifndef IMAGEOBJ_H
#define IMAGEOBJ_H

#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsSvgItem>

/*! \brief Base class for images, which can be pixmaps or svg
 *
 * ImageObj is used both by items part of the map "tree" in
 * ImageItem and as flag in FlagObj
 *
 * Both of these types are actually drawn onto the map
 */

class ImageObj : public QGraphicsItem {
  public:
    enum ImageType { Undefined, Pixmap, ModifiedPixmap, SVG, ClonedSVG };

    ImageObj();
    ImageObj(QGraphicsItem *);
    ~ImageObj();
    void init();
    void copy(ImageObj *);
    void setPos(const QPointF &pos);
    void setPos(const qreal &x, const qreal &y);
    void setZValue(qreal z);
    void setVisibility(bool);
    void setWidth(qreal w);
    void setScaleFactor(qreal f);
    qreal getScaleFactor();
    virtual QRectF boundingRect() const;
    virtual void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);
    bool load(const QString &, bool createClone = false);
    bool save(const QString &);
    QString getExtension();
    ImageType getType();
    QIcon getIcon();

  protected:
    ImageObj::ImageType imageType;

    QGraphicsSvgItem *svgItem;
    QString svgCachePath;

    QGraphicsPixmapItem *pixmapItem;
    QPixmap *originalPixmap;

    qreal scaleFactor;

    ulong imageID;
};
#endif
