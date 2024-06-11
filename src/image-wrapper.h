#ifndef IMAGE_WRAPPER_H
#define IMAGE_WRAPPER_H

#include "scripting.h"

class ImageItem;
class VymModel;

class ImageWrapper : public VymScriptContext {
    Q_OBJECT
  public:
    ImageWrapper(ImageItem*);
    ~ImageWrapper();

  public slots:
    QString headingText();  
    void setHeadingRichText(const QString &);
    void setHeadingText(const QString &);

  private:
    ImageItem *imageItem;
};

#endif
