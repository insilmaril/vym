#ifndef IMAGE_WRAPPER_H
#define IMAGE_WRAPPER_H

#include "scripting.h"

class ImageItem;
class VymModel;

class ImageWrapper : public QObject {
    Q_OBJECT
  public:
    ImageWrapper(ImageItem*);
    ~ImageWrapper();
    VymModel* model();
    ImageItem* imageItem();

  public slots:
    qreal getPosX();
    qreal getPosY();
    qreal getScenePosX();
    qreal getScenePosY();
    bool hasRichTextHeading();
    QString headingText();  
    bool selectParent();
    void setHeadingRichText(const QString &);
    void setHeadingText(const QString &);

  private:
    ImageItem *imageItemInt;
};

#endif
