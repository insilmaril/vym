#ifndef IMAGEITEM_H
#define IMAGEITEM_H

#include <QGraphicsSvgItem>
#include <QList>
#include <QPixmap>
#include <QVariant>

#include "floatimageobj.h"
//#include "treeitem.h"
#include "mapitem.h"

bool isImage (const QString &fname);

class ImageItem: public MapItem
{
public:
    ImageItem();
    ImageItem(const QList<QVariant> &data, TreeItem *parent = 0);

    virtual ~ImageItem();

protected:  
    void init();
    void clear();
    ImageObj::ImageType imageType;
public:	
    virtual ImageObj::ImageType getImageType();

    virtual bool load (const QString &fname);
    virtual FloatImageObj* createMapObj();	    //! Create classic object in GraphicsView
protected:  
    qreal scaleX;
    qreal scaleY;
    QImage *originalImage;
    QGraphicsSvgItem *originalSvg;
    QString originalFilename;
    int zValue;

public:	
    virtual qreal getScaleX();
    virtual qreal getScaleY();
    virtual void setScale (qreal,qreal);

    virtual void setZValue(int z);
    virtual void setOriginalFilename(const QString &);
    virtual QString getOriginalFilename();
    virtual bool saveImage (const QString &fn, const QString &format);
    virtual QString saveToDir(const QString &,const QString&);
};

#endif

