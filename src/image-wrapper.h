#ifndef IMAGE_WRAPPER_H
#define IMAGE_WRAPPER_H

class BranchWrapper;
class ImageItem;
class VymModel;

#include <QObject>

class ImageWrapper : public QObject {
    Q_OBJECT
  public:
    Q_INVOKABLE ImageWrapper(ImageItem*);
    ~ImageWrapper();
    Q_INVOKABLE VymModel* model();
    Q_INVOKABLE ImageItem* imageItem();

  public slots:
    qreal getPosX();
    qreal getPosY();
    qreal getScale();
    qreal getScenePosX();
    qreal getScenePosY();
    bool hasRichTextHeading();
    QString headingText();  
    bool relinkToBranch(BranchWrapper *dst);
    bool relinkToBranchAt(BranchWrapper *dst, int pos);
    void saveImage(const QString &filename);
    bool selectParent();
    void setHeadingRichText(const QString &);
    void setHeadingText(const QString &);
    void setHideLinkUnselected(bool b);
    void setPos(qreal x, qreal y);
    void setScale(const qreal &f);

  private:
    ImageItem *imageItemInt;
};

#endif
