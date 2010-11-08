#ifndef IMAGEITEM_H
#define IMAGEITEM_H

#include <QList>
#include <QPixmap>
#include <QVariant>

#include "floatimageobj.h"
//#include "treeitem.h"
#include "mapitem.h"


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

    virtual void load (const QPixmap &pm);
    virtual bool load (const QString &fname);
    virtual FloatImageObj* createMapObj(QGraphicsScene *scene);	    //! Create classic object in GraphicsView

protected:  
    QPixmap pixmap;
    QString originalFilename;
    int zValue;

public:	
    virtual void setZValue(int z);
    virtual void setOriginalFilename(const QString &);
    virtual QString getOriginalFilename();
    virtual void save (const QString &fn, const QString &format);
    virtual QString saveToDir(const QString &,const QString&);

};


#endif

