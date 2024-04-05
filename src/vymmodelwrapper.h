#ifndef VYMMODELWRAPPER_H
#define VYMMODELWRAPPER_H

#include "scripting.h"

#include <QColor>
#include <QObject>
#include <QJSValue>
#include <QVariant>

#include "branch-wrapper.h"

class BranchItem;
class VymModel;

class VymModelWrapper : public VymScriptContext {
    Q_OBJECT
  public:
    VymModelWrapper(VymModel *m);

  public slots:
    void addBranch();   // FIXME move to BranchWrapper
    void addBranchAt(int pos);// FIXME move to BranchWrapper
    void addBranchBefore();// FIXME move to BranchWrapper
    void addMapCenterAtPos(qreal x, qreal y);
    void addMapInsert(QString filename, int pos, int contentFilter);
    void addMapInsert(const QString &filename, int pos);
    void addMapInsert(const QString &filename);
    void addMapReplace(QString filename);
    void addSlide();
    void addXLink(const QString &begin, const QString &end, int width,
                  const QString &color, const QString &penstyle);
    int branchCount();// FIXME move to BranchWrapper
    int centerCount();
    void centerOnID(const QString &id); // tested: ok
    void clearFlags();// FIXME move to BranchWrapper
    void colorBranch(const QString &color);// FIXME move to BranchWrapper
    void colorSubtree(const QString &color);// FIXME move to BranchWrapper
    void copy();// FIXME copy to BranchWrapper
    void cut();// FIXME copy to BranchWrapper
    void cycleTask();// FIXME move to BranchWrapper
    int depth();// FIXME move to BranchWrapper
    void detach();// FIXME move to BranchWrapper
    bool exportMap(QJSValueList args);
    BranchWrapper* findBranchByAttribute(const QString &key, const QString &value);
    BranchWrapper* findBranchById(const QString &);
    int getBranchIndex();// FIXME move to BranchWrapper
    QString getDestPath();
    QString getFileDir();
    QString getFileName();
    QString getFrameType(const bool & useInnerFrame);// FIXME move to BranchWrapper
    QString getHeadingPlainText();  // FIXME duplicate in BranchWrapper: headingText()
    QString getHeadingXML();// FIXME move to BranchWrapper
    QString getMapAuthor();
    QString getMapComment();
    QString getMapTitle();
    QString getNotePlainText();// FIXME move to BranchWrapper
    QString getNoteXML();// FIXME move to BranchWrapper
    qreal getPosX();// FIXME move to BranchWrapper, copy for image
    qreal getPosY();// FIXME move to BranchWrapper, copy for image
    qreal getScenePosX();// FIXME move to BranchWrapper, copy for image
    qreal getScenePosY();// FIXME move to BranchWrapper, copy for image
    int getRotationHeading();// FIXME move to BranchWrapper
    int getRotationSubtree();// FIXME move to BranchWrapper
    QString getSelectionString();//FIXME copy to BranchWrapper?
    int getTaskPriorityDelta();// FIXME move to BranchWrapper
    QString getTaskSleep();// FIXME move to BranchWrapper
    int getTaskSleepDays();// FIXME move to BranchWrapper
    QString getUrl();// FIXME move to BranchWrapper
    QString getVymLink();// FIXME move to BranchWrapper
    QString getXLinkColor();// FIXME move to BranchWrapper
    int getXLinkWidth();// FIXME move to BranchWrapper
    QString getXLinkPenStyle();// FIXME move to BranchWrapper
    QString getXLinkStyleBegin();// FIXME move to BranchWrapper
    QString getXLinkStyleEnd();// FIXME move to BranchWrapper
    bool hasActiveFlag(const QString &flag);// FIXME move to BranchWrapper
    bool hasNote();// FIXME move to BranchWrapper
    bool hasRichTextNote();// FIXME move to BranchWrapper
    bool hasTask();// FIXME move to BranchWrapper
    void importDir(const QString &path);
    bool isScrolled();// FIXME move to BranchWrapper
    void loadImage(const QString &filename);// FIXME move to BranchWrapper
    void loadNote(const QString &filename);// FIXME move to BranchWrapper
    void moveDown();// FIXME move to BranchWrapper
    void moveUp();// FIXME move to BranchWrapper
    void moveSlideDown(int n);
    void moveSlideDown();
    void moveSlideUp(int n);
    void moveSlideUp();
    bool newBranchIterator(const QString &itname, bool deepLevelsFirst = false);
    BranchWrapper* nextBranch(const QString &itname);
    void nop(); // FIXME remove?
    void note2URLs();// FIXME move to BranchWrapper
    bool parseVymText(const QString &text);
    void paste();
    void redo();
    bool relinkTo(const QString &parent, int num);// FIXME move to BranchWrapper
    bool relinkTo(const QString &parent);   // FIXME duplicated in BranchWrapper relinkTo
    void remove();// FIXME copy to BranchWrapper, ImageWrapper, leave for VymModel
    void removeChildren();// FIXME move to BranchWrapper
    void removeChildBranches();// FIXME move to BranchWrapper
    void removeKeepChildren();// FIXME move to BranchWrapper
    void removeSlide(int n);
    QVariant repeatLastCommand();
    void saveImage(const QString &filename);// FIXME move to ImageWrapper
    void saveNote(const QString &filename);// FIXME move to BranchWrapper
    void saveSelection(const QString &filename);
    void scroll();// FIXME duplicated in BranchWrapper
    bool select(const QString &s);
    BranchWrapper* selectedBranch();
    bool selectID(const QString &s);
    bool selectFirstBranch();// FIXME move to BranchWrapper
    bool selectFirstChildBranch();// FIXME move to BranchWrapper
    bool selectLastBranch();// FIXME move to BranchWrapper
    bool selectLastChildBranch();// FIXME move to BranchWrapper
    bool selectLastImage();// FIXME move to BranchWrapper
    bool selectParent();// FIXME move to BranchWrapper, copy to ImageWrapper
    bool selectLatestAdded();
    bool selectToggle(const QString &selectString); // FIXME move to BranchWrapper and ImageWrapper
    bool selectXLink(int n);// FIXME move to BranchWrapper
    bool selectXLinkOtherEnd(int n);// FIXME move to BranchWrapper
    void setDefaultLinkColor(const QString &color); // FIXME-4 maybe also rename other setMap* methods?
    void setAttribute(const QString &key, const QString &value);// FIXME move to BranchWrapper
    void setFlagByName(const QString &s);// FIXME move to BranchWrapper
    void setHeadingConfluencePageName();
    void setHeadingPlainText(const QString &s);// FIXME move to BranchWrapper and ImageWrapper
    void setHideExport(bool b);// FIXME move to BranchWrapper
    void setHideLinkUnselected(bool b);// FIXME move to BranchWrapper and ImageWrapper
    void setMapAnimCurve(int n);
    void setMapAnimDuration(int n);
    void setMapAuthor(const QString &s);
    void setMapBackgroundColor(const QString &color);
    void setMapComment(const QString &s);
    void setMapLinkStyle(const QString &style);
    void setMapRotation(float a); // tested: ok
    void setMapTitle(const QString &s);
    void setMapZoom(float z); // tested: ok
    void setNotePlainText(const QString &s);// FIXME move to BranchWrapper
    void setPos(qreal x, qreal y);// FIXME move to BranchWrapper and ImageWrapper
    void setFramePenWidth(const bool & useInnerFrame, int w);// FIXME move to BranchWrapper
    void setFrameBrushColor(const bool & useInnerFrame, const QString &color);// FIXME move to BranchWrapper
    void setFramePadding(const bool & useInnerFrame, int padding);// FIXME move to BranchWrapper
    void setFramePenColor(const bool & useInnerFrame, const QString &color);// FIXME move to BranchWrapper
    void setFrameType(const bool & useInnerFrame, const QString &type);// FIXME move to BranchWrapper
    void setRotationHeading(const int &i);// FIXME move to BranchWrapper
    void setRotationSubtree(const int &i);// FIXME move to BranchWrapper
    void setRotationsAutoDesign(const bool b);// FIXME move to BranchWrapper
    void setScale(qreal f);// FIXME move to BranchWrapper and ImageWrapper
    void setScaleSubtree(qreal f);// FIXME move to BranchWrapper
    void setScalingAutoDesign(const bool b);// FIXME move to BranchWrapper and ImageWrapper
    void setSelectionBrushColor(const QString &color);
    void setSelectionPenColor(const QString &color);
    void setSelectionPenWidth(const qreal &);
    void setTaskPriorityDelta(const int &n);// FIXME move to BranchWrapper
    bool setTaskSleep(const QString &s);// FIXME move to BranchWrapper
    void setUrl(const QString &s);// FIXME move to BranchWrapper
    void setVymLink(const QString &s);// FIXME move to BranchWrapper
    void setXLinkColor(const QString &color);// FIXME move to XLinkWrapper
    void setXLinkStyle(const QString &style);// FIXME move to XLinkWrapper
    void setXLinkStyleBegin(const QString &style);// FIXME move to XLinkWrapper
    void setXLinkStyleEnd(const QString &style);// FIXME move to XLinkWrapper
    void setXLinkWidth(int w);// FIXME move to XLinkWrapper
    void sleep(int n);
    int slideCount();
    void sortChildren(bool b);// FIXME move to BranchWrapper
    void sortChildren();// FIXME move to BranchWrapper
    void toggleFlagByUid(const QString &s);// FIXME move to BranchWrapper
    void toggleFlagByName(const QString &s);// FIXME move to BranchWrapper
    void toggleScroll();// FIXME move to BranchWrapper
    void toggleTarget();// FIXME move to BranchWrapper
    void toggleTask();// FIXME move to BranchWrapper
    void undo();
    bool unscroll();// FIXME duplicated in BranchWrapper
    void unscrollChildren();// FIXME move to BranchWrapper
    void unselectAll();
    void unsetFlagByName(const QString &s);// FIXME move to BranchWrapper
    int xlinkCount();// FIXME move to BranchWrapper

  private:
    VymModel *model;
};

#endif
