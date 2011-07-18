#ifndef IMAGEITEM_H
#define IMAGEITEM_H

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
    enum ImageType {Undefined,Pixmap,SVG};

public:
    ImageItem();
    ImageItem(const QList<QVariant> &data, TreeItem *parent = 0);

    virtual ~ImageItem();

protected:  
    void init();
    ImageType imageType;
public:	
    virtual ImageType getImageType();

    virtual void load (const QImage &img);
    virtual bool load (const QString &fname);
    virtual FloatImageObj* createMapObj(QGraphicsScene *scene);	    //! Create classic object in GraphicsView
protected:  
    qreal scaleX;
    qreal scaleY;
    QImage  originalImage;
    QString originalFilename;
    int zValue;

public:	
    virtual qreal getScaleX();
    virtual qreal getScaleY();
    virtual void setScale (qreal,qreal);

    virtual void setZValue(int z);
    virtual void setOriginalFilename(const QString &);
    virtual QString getOriginalFilename();
    virtual bool save (const QString &fn, const QString &format);
    virtual QString saveToDir(const QString &,const QString&);

};


#endif

