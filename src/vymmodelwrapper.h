#ifndef VYMMODEL_WRAPPER_H
#define VYMMODEL_WRAPPER_H

#include <QColor>
#include <QObject>
#include <QJSValue>
#include <QVariant>

class BranchItem;
class AttributeWrapper;
class BranchWrapper;
class ImageWrapper;
class ItemListWrapper;
class VymModel;
class XLinkWrapper;

class VymModelWrapper : public QObject {
    Q_OBJECT
  public:
    Q_INVOKABLE VymModelWrapper(VymModel *m);
    ~VymModelWrapper();
    VymModel* getModel();

  public slots:
    void addMapCenterAtPos(qreal x, qreal y);
    void addSlide();
    int centerCount();
    void centerOnID(const QString &id); // tested: ok
    void copy();
    void cut();
    bool exportMap(QJSValueList args);
    Q_INVOKABLE BranchWrapper* findBranchByAttribute(const QString &key, const QString &value);
    Q_INVOKABLE AttributeWrapper* findAttributeById(const QString &);
    Q_INVOKABLE BranchWrapper* findBranchById(const QString &);
    Q_INVOKABLE BranchWrapper* findBranchBySelection(const QString &);
    Q_INVOKABLE ImageWrapper* findImageById(const QString &);
    Q_INVOKABLE ImageWrapper* findImageBySelection(const QString &);
    Q_INVOKABLE XLinkWrapper* findXLinkById(const QString &);
    QString getBackgroundColor();
    QString getBackgroundImageName();
    QString getDestPath();
    QString getFileDir();
    QString getFileName();
    QString getAuthor();
    QString getComment();
    QString getTitle();
    QString getLinkColorHint();
    QString getSelectionString();
    double getZoom();
    bool hasBackgroundImage();
    bool loadBackgroundImage(const QString &imagePath);
    bool loadBranchReplace(QString filename, BranchWrapper *bw);
    bool loadDataInsert(QString filename, int pos = -1, BranchWrapper *bw = nullptr);
    void moveSlideDown(int n);
    void moveSlideDown();
    void moveSlideUp(int n);
    void moveSlideUp();
    Q_INVOKABLE ItemListWrapper* itemList();
    void paste();
    void redo();
    void remove();
    void removeAttribute(AttributeWrapper *aw);
    void removeBranch(BranchWrapper *bw);
    void removeImage(ImageWrapper *iw);
    void removeKeepChildren(BranchWrapper *bw);
    void removeSlide(int n);
    void removeXLink(XLinkWrapper *xlw);
    QVariant repeatLastCommand();
    bool saveSelection(const QString &filename);
    bool select(const QString &s);
    Q_INVOKABLE AttributeWrapper* selectedAttribute();
    Q_INVOKABLE BranchWrapper* selectedBranch();
    Q_INVOKABLE XLinkWrapper* selectedXLink();
    bool selectUids(QJSValueList args);
    bool selectLatestAdded();
    void setDefaultLinkColor(const QString &color); // FIXME-3-4 maybe also rename other setMap* methods?
    void setAnimCurve(int n);
    void setAnimDuration(int n);
    void setAuthor(const QString &s);
    void setBackgroundColor(const QString &color);
    void setBackgroundImageName(const QString &name);
    void setComment(const QString &s);
    void setLinkStyle(const QString &style);
    void setLinkColorHint(const QString &hint);
    void setRotationView(float a);
    void setTitle(const QString &s);
    void setZoom(float z);
    void setSelectionBrushColor(const QString &color);
    void setSelectionPenColor(const QString &color);
    void setSelectionPenWidth(const qreal &);
    void sleep(int n);
    int slideCount();
    void undo();
    void unselectAll();
    void unsetBackgroundImage();

  private:
    VymModel *model;
};

#endif
