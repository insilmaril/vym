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
public:	
    virtual bool load (const QString &fname);
    virtual FloatImageObj* createMapObj();	    //! Create classic object in GraphicsView
protected:  
    QString originalFilename;
    int zValue;

public:	
    void setScaleFactor(qreal);
    qreal getScaleFactor();
    void setZValue(int z);
    void setOriginalFilename(const QString &);
    QString getOriginalFilename();
    QString getUniqueFilename();
    bool saveImage (const QString &fn);
    QString saveToDir(const QString &,const QString&);
};

#endif

