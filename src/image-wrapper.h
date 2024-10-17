#ifndef IMAGE_WRAPPER_H
#define IMAGE_WRAPPER_H

class BranchWrapper;
class ImageItem;
class VymModel;

#include <QObject>

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
    bool relinkToBranch(BranchWrapper *dst);
    bool relinkToBranchAt(BranchWrapper *dst, int pos);
    bool selectParent();
    void setHeadingRichText(const QString &);
    void setHeadingText(const QString &);
    void setHideLinkUnselected(bool b); // FIXME-3 move to BranchWrapper and ImageWrapper
    void setPos(qreal x, qreal y);

  private:
    ImageItem *imageItemInt;
};

#endif
