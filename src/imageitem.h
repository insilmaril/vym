#ifndef IMAGEITEM_H
#define IMAGEITEM_H

#include "mapitem.h"

class ImageContainer;
class ImageWrapper;
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
    void setParentBranch(BranchItem *);
    ImageWrapper* imageWrapper();
    virtual bool load(const QString &fname);
    ImageContainer* createImageContainer();
    ImageContainer* getImageContainer();
    void updateContainerStackingOrder();
    void unlinkImageContainer();

  protected:
    ImageContainer *imageContainer;
    ImageWrapper *imageWrapperInt;
    QString originalFilename;
    QString currentFilename;

  public:
    void setScale(qreal);
    qreal scale();
    qreal width();
    qreal height();
    void setOriginalFilename(const QString &);
    QString getOriginalFilename();
    QString getUniqueFilename();
    bool saveImage(const QString &fn);
    QString saveToDir(const QString &tmpdir, const QString &prefix);
};

#endif
