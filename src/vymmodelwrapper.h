#ifndef VYMMODELWRAPPER_H
#define VYMMODELWRAPPER_H

#include "scripting.h"

#include <QColor>
#include <QObject>
#include <QJSValue>
#include <QVariant>

class BranchItem;
class BranchWrapper;
class ImageWrapper;
class VymModel;

class VymModelWrapper : public VymScriptContext {
    Q_OBJECT
  public:
    VymModelWrapper(VymModel *m);

  public slots:
    void addMapCenterAtPos(qreal x, qreal y);
    void addMapInsert(QString filename, int pos, int contentFilter);
    void addMapInsert(const QString &filename, int pos);
    void addMapInsert(const QString &filename);
    void addMapReplace(QString filename);
    void addSlide();
    void addXLink(const QString &begin, const QString &end, int width,
                  const QString &color, const QString &penstyle);
    int centerCount();
    void centerOnID(const QString &id); // tested: ok
    void copy();// FIXME-3 copy to BranchWrapper (use without selecting in VymModel!)
    void cut();// FIXME-3 copy to BranchWrapper  (use without selecting in VymModel!)
    void cycleTask();// FIXME-3 move to BranchWrapper
    int depth();// FIXME-3 move to BranchWrapper
    void detach();// FIXME-3 move to BranchWrapper
    bool exportMap(QJSValueList args);
    BranchWrapper* findBranchByAttribute(const QString &key, const QString &value);
    BranchWrapper* findBranchById(const QString &);
    ImageWrapper* findImageById(const QString &);
    int getBranchIndex();// FIXME-3 move to BranchWrapper
    QString getDestPath();
    QString getFileDir();
    QString getFileName();
    QString getFrameType(const bool & useInnerFrame);// FIXME-3 move to BranchWrapper
    QString getHeadingXML();// FIXME-3 move to BranchWrapper
    QString getMapAuthor();
    QString getMapComment();
    QString getMapTitle();
    QString getNotePlainText();// FIXME-3 move to BranchWrapper
    QString getNoteXML();// FIXME-3 move to BranchWrapper
    qreal getPosX();// FIXME-3 move to BranchWrapper, copy for image
    qreal getPosY();// FIXME-3 move to BranchWrapper, copy for image
    qreal getScenePosX();// FIXME-3 move to BranchWrapper, copy for image
    qreal getScenePosY();// FIXME-3 move to BranchWrapper, copy for image
    int getRotationHeading();// FIXME-3 move to BranchWrapper
    int getRotationSubtree();// FIXME-3 move to BranchWrapper
    QString getSelectionString();//FIXME-3 copy to BranchWrapper?
    int getTaskPriorityDelta();// FIXME-3 move to BranchWrapper
    QString getTaskSleep();// FIXME-3 move to BranchWrapper
    int getTaskSleepDays();// FIXME-3 move to BranchWrapper
    QString getXLinkColor();// FIXME-3 move to BranchWrapper
    int getXLinkWidth();// FIXME-3 move to BranchWrapper
    QString getXLinkPenStyle();// FIXME-3 move to BranchWrapper
    QString getXLinkStyleBegin();// FIXME-3 move to BranchWrapper
    QString getXLinkStyleEnd();// FIXME-3 move to BranchWrapper
    bool hasActiveFlag(const QString &flag);// FIXME-3 move to BranchWrapper
    bool hasNote();// FIXME-3 move to BranchWrapper
    bool hasRichTextNote();// FIXME-3 move to BranchWrapper
    bool hasTask();// FIXME-3 move to BranchWrapper
    void importDir(const QString &path);
    void loadImage(const QString &filename);// FIXME-3 move to BranchWrapper
    void loadNote(const QString &filename);// FIXME-3 move to BranchWrapper
    void moveDown();// FIXME-3 move to BranchWrapper
    void moveUp();// FIXME-3 move to BranchWrapper
    void moveSlideDown(int n);
    void moveSlideDown();
    void moveSlideUp(int n);
    void moveSlideUp();
    void newBranchIterator(const QString &itname, bool deepLevelsFirst = false);
    BranchWrapper* nextBranch(const QString &itname);
    void note2URLs();// FIXME-3 move to BranchWrapper
    void paste();   // FIXME copy to BranchWrappe (use without selecting in VymModel!)
    void redo();
    bool relinkTo(const QString &parent, int num);// FIXME-3 move to BranchWrapper
    void remove();// FIXME-3 copy to ImageWrapper, leave for VymModel (already in BranchWrapper)
    void removeChildren();// FIXME-3 move to BranchWrapper
    void removeChildBranches();// FIXME-3 move to BranchWrapper
    void removeKeepChildren();// FIXME-3 move to BranchWrapper
    void removeSlide(int n);
    QVariant repeatLastCommand();
    void saveImage(const QString &filename);// FIXME-3 move to ImageWrapper
    void saveNote(const QString &filename);// FIXME-3 move to BranchWrapper
    void saveSelection(const QString &filename);
    bool select(const QString &s);
    BranchWrapper* selectedBranch();
    bool selectID(const QString &s);
    bool selectFirstChildBranch();// FIXME-3 move to BranchWrapper
    bool selectLastChildBranch();// FIXME-3 move to BranchWrapper
    bool selectLastImage();// FIXME-3 move to BranchWrapper
    bool selectLatestAdded();
    bool selectToggle(const QString &selectString); // FIXME-3 move to BranchWrapper and ImageWrapper
    bool selectXLink(int n);// FIXME-3 move to BranchWrapper
    bool selectXLinkOtherEnd(int n);// FIXME-3 move to BranchWrapper
    void setDefaultLinkColor(const QString &color); // FIXME-3-4 maybe also rename other setMap* methods?
    void setFlagByName(const QString &s);// FIXME-3 OBSOLETE moved to BranchWrapper
    void setHeadingConfluencePageName();
    void setHeadingPlainText(const QString &s);// FIXME-3 OBSOLETE moved to BranchWrapper and ImageWrapper
    void setHideExport(bool b);// FIXME-3 move to BranchWrapper
    void setHideLinkUnselected(bool b);// FIXME-3 move to BranchWrapper and ImageWrapper
    void setMapAnimCurve(int n);    // FIXME-3 rename
    void setMapAnimDuration(int n); // FIXME-3 rename
    void setMapAuthor(const QString &s);// FIXME-3 rename setAuthor
    void setMapBackgroundColor(const QString &color);// FIXME-3 rename 
    void setMapComment(const QString &s);// FIXME-3 rename 
    void setMapLinkStyle(const QString &style);// FIXME-3 rename 
    void setMapRotation(float a); // tested: ok// FIXME-3 rename 
    void setMapTitle(const QString &s);// FIXME-3 rename 
    void setMapZoom(float z); // tested: ok// FIXME-3 rename 
    void setNotePlainText(const QString &s);// FIXME-3 OBSOLETE moved to BranchWrapper
    void setFramePenWidth(const bool & useInnerFrame, int w);// FIXME-3 move to BranchWrapper
    void setFrameBrushColor(const bool & useInnerFrame, const QString &color);// FIXME-3 move to BranchWrapper
    void setFramePadding(const bool & useInnerFrame, int padding);// FIXME-3 move to BranchWrapper
    void setFramePenColor(const bool & useInnerFrame, const QString &color);// FIXME-3 move to BranchWrapper
    void setFrameType(const bool & useInnerFrame, const QString &type);// FIXME-3 move to BranchWrapper
    void setRotationHeading(const int &i);// FIXME-3 move to BranchWrapper
    void setRotationSubtree(const int &i);// FIXME-3 move to BranchWrapper
    void setRotationsAutoDesign(const bool b);// FIXME-3 move to BranchWrapper
    void setScale(qreal f);// FIXME-3 move to BranchWrapper and ImageWrapper
    void setScaleSubtree(qreal f);// FIXME-3 move to BranchWrapper
    void setScalingAutoDesign(const bool b);// FIXME-3 move to BranchWrapper and ImageWrapper
    void setSelectionBrushColor(const QString &color);
    void setSelectionPenColor(const QString &color);
    void setSelectionPenWidth(const qreal &);
    void setTaskPriorityDelta(const int &n);// FIXME-3 move to BranchWrapper
    bool setTaskSleep(const QString &s);// FIXME-3 move to BranchWrapper
    void setXLinkColor(const QString &color);// FIXME-3 move to XLinkWrapper
    void setXLinkStyle(const QString &style);// FIXME-3 move to XLinkWrapper
    void setXLinkStyleBegin(const QString &style);// FIXME-3 move to XLinkWrapper
    void setXLinkStyleEnd(const QString &style);// FIXME-3 move to XLinkWrapper
    void setXLinkWidth(int w);// FIXME-3 move to XLinkWrapper
    void sleep(int n);
    int slideCount();
    void sortChildren(bool b);// FIXME-3 move to BranchWrapper
    void sortChildren();// FIXME-3 move to BranchWrapper
    void toggleFlagByName(const QString &s);// FIXME-3 OBSOLETE moved to BranchWrapper
    void toggleTarget();// FIXME-3 move to BranchWrapper
    void toggleTask();// FIXME-3 move to BranchWrapper
    void undo();
    void unscrollChildren();// FIXME-3 move to BranchWrapper
    void unselectAll();
    void unsetFlagByName(const QString &s);// FIXME-3 move to BranchWrapper
    int xlinkCount();// FIXME-3 move to BranchWrapper

    // FIXME-2 commands below are duplicated in BranchWrapper  (cleanup WIP)
    // Before removing here from VymModelWrapper, 
    //   - rework calls for saveState in VymModel
    //   - rework tests in test/vym-selftest.vys
    int branchCount();
    void clearFlags();
    void colorBranch(const QString &color);
    void colorSubtree(const QString &color);
    QString getHeadingPlainText();          // in BranchWrapper: headingText()
    bool isScrolled();
    bool relinkTo(const QString &parent);   // in BranchWrapper: relinkTo
    void scroll();
    void toggleScroll();
    bool unscroll();

  private:
    VymModel *model;
};

#endif
