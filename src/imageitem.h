#ifndef IMAGEITEM_H
#define IMAGEITEM_H

#include "mapitem.h"

class ImageContainer;
class QGraphicsScene;

bool isImage(const QString &fname);

class ImageItem : public MapItem {
  public:
    ImageItem();

    virtual ~ImageItem();

  protected:
    void init();

  public:
    BranchItem* parentBranch();
    virtual bool load(const QString &fname);
    ImageContainer* createImageContainer();
    ImageContainer* getImageContainer();
    void unlinkImageContainer();

  protected:
    ImageContainer *imageContainer;
    QString originalFilename;

  public:
    void setScaleFactor(qreal);
    qreal getScaleFactor();
    qreal width();
    qreal height();
    void setOriginalFilename(const QString &);
    QString getOriginalFilename();
    QString getUniqueFilename();
    bool saveImage(const QString &fn);
    QString saveToDir(const QString &, const QString &);
};

#endif
