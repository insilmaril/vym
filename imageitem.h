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
    ImageObj::ImageType imageType;                  // FIXME-1 still required here?
public:	
    virtual ImageObj::ImageType getImageType();     // FIXME-1 still required here?

    virtual bool load (const QString &fname);
    virtual FloatImageObj* createMapObj();	    //! Create classic object in GraphicsView
protected:  
    QString originalFilename;
    int zValue;

public:	
    void setScaleFactor(qreal);
    qreal getScaleFactor();
    virtual void setZValue(int z);
    virtual void setOriginalFilename(const QString &);
    virtual QString getOriginalFilename();
    virtual bool saveImage (const QString &fn, const QString &format);
    virtual QString saveToDir(const QString &,const QString&);
};

#endif

