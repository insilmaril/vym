#ifndef VYMMODEL_WRAPPER_H
#define VYMMODEL_WRAPPER_H

#include "scripting.h"

#include <QColor>
#include <QObject>
#include <QJSValue>
#include <QVariant>

class BranchItem;
class AttributeWrapper;
class BranchWrapper;
class ImageWrapper;
class VymModel;
class XLinkWrapper;

class VymModelWrapper : public VymScriptContext {
    Q_OBJECT
  public:
    VymModelWrapper(VymModel *m);

  public slots:
    void addMapCenterAtPos(qreal x, qreal y);
    void addSlide();
    int centerCount();
    void centerOnID(const QString &id); // tested: ok
    void copy();
    void cut();
    int depth();        // FIXME-3 move to BranchWrapper
    void detach();      // FIXME-3 move to BranchWrapper
    bool exportMap(QJSValueList args);
    BranchWrapper* findBranchByAttribute(const QString &key, const QString &value);
    BranchWrapper* findBranchById(const QString &);
    BranchWrapper* findBranchBySelection(const QString &);
    ImageWrapper* findImageById(const QString &);
    ImageWrapper* findImageBySelection(const QString &);
    XLinkWrapper* findXLinkById(const QString &);
    QString getDestPath();
    QString getFileDir();
    QString getFileName();
    QString getHeadingXML();    // FIXME-3 move to BranchWrapper
    QString getAuthor();
    QString getComment();
    QString getTitle();
    QString getNotePlainText(); // FIXME-3 getNoteText in BranchWrapper. Rework test scripts
    QString getNoteXML();       // FIXME-3 move to BranchWrapper
    int getRotationHeading();   // FIXME-3 move to BranchWrapper
    int getRotationSubtree();   // FIXME-3 move to BranchWrapper
    QString getSelectionString();//FIXME-3 copy to BranchWrapper?
    bool loadBranchReplace(QString filename, BranchWrapper *bw);
    bool loadDataInsert(QString filename);
    void moveSlideDown(int n);
    void moveSlideDown();
    void moveSlideUp(int n);
    void moveSlideUp();
    void newBranchIterator(const QString &itname, bool deepLevelsFirst = false);
    BranchWrapper* nextBranch(const QString &itname);
    void note2URLs();       // FIXME-3 move to BranchWrapper
    void paste();
    void redo();
    void remove();              // FIXME-3 still needed? Rename to removeSelection()?
    void removeBranch(BranchWrapper *bw);
    void removeImage(ImageWrapper *iw);
    void removeKeepChildren(BranchWrapper *bw);
    void removeSlide(int n);
    void removeXLink(XLinkWrapper *xlw);
    QVariant repeatLastCommand();
    void saveImage(const QString &filename);// FIXME-3 move to ImageWrapper
    void saveNote(const QString &filename); // FIXME-3 move to BranchWrapper
    void saveSelection(const QString &filename);
    bool select(const QString &s);
    AttributeWrapper* selectedAttribute();
    BranchWrapper* selectedBranch();
    XLinkWrapper* selectedXLink();
    bool selectUids(QJSValueList args);
    bool selectLatestAdded();
    bool selectToggle(const QString &selectString); // FIXME-3 move to BranchWrapper and ImageWrapper
    void setDefaultLinkColor(const QString &color); // FIXME-3-4 maybe also rename other setMap* methods?
    void setHeadingConfluencePageName();// FIXME-3 move to BranchWrapper
    void setHideExport(bool b);         // FIXME-3 move to BranchWrapper
    void setHideLinkUnselected(bool b); // FIXME-3 move to BranchWrapper and ImageWrapper
    void setAnimCurve(int n);
    void setAnimDuration(int n);
    void setAuthor(const QString &s);
    void setBackgroundColor(const QString &color);
    void setComment(const QString &s);
    void setLinkStyle(const QString &style);
    void setRotation(float a);
    void setTitle(const QString &s);
    void setZoom(float z);
    void setNotePlainText(const QString &s);    // FIXME-3 OBSOLETE moved to BranchWrapper
    void setRotationHeading(const int &i);      // FIXME-3 move to BranchWrapper
    void setRotationSubtree(const int &i);      // FIXME-3 move to BranchWrapper
    void setRotationsAutoDesign(const bool b);  // FIXME-3 move to BranchWrapper
    void setScale(qreal f);                     // FIXME-3 move to BranchWrapper and ImageWrapper
    void setScaleSubtree(qreal f);              // FIXME-3 move to BranchWrapper
    void setScalingAutoDesign(const bool b);    // FIXME-3 move to BranchWrapper and ImageWrapper
    void setSelectionBrushColor(const QString &color);
    void setSelectionPenColor(const QString &color);
    void setSelectionPenWidth(const qreal &);
    void sleep(int n);
    int slideCount();
    void toggleTarget();        // FIXME-3 move to BranchWrapper
    void undo();
    void unselectAll();

  private:
    VymModel *model;
};

#endif
