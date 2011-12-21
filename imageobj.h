#ifndef IMAGEOBJ_H
#define IMAGEOBJ_H

#include <QGraphicsScene>
#include <QGraphicsPixmapItem>

/*! \brief Base class for pixmaps.
*/

class ImageObj: public QGraphicsPixmapItem
{
public:
    ImageObj( QGraphicsItem*);
    ~ImageObj();
    void copy (ImageObj*);
    void setVisibility(bool);
    void save (const QString &, const char *);
    bool load (const QString &);
    bool load (const QPixmap &);
};
#endif
