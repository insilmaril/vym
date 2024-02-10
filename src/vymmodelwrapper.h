#ifndef VYMMODELWRAPPER_H
#define VYMMODELWRAPPER_H

#include "scripting.h"
#include <QColor>
#include <QObject>
#include <QJSValue>
#include <QVariant>

class BranchItem;
class VymModel;

class VymModelWrapper : public VymScriptContext {
    Q_OBJECT
  public:
    VymModelWrapper(VymModel *m);

  private:
    BranchItem *getSelectedBranch();
    QVariant getParameter(bool &ok, const QString &key,
                          const QStringList &parameters);

  public slots:
    void addBranch();
    void addBranchAt(int pos);
    void addBranchBefore();
    void addMapCenterAtPos(qreal x, qreal y);
    void addMapInsert(QString filename, int pos, int contentFilter);
    void addMapInsert(const QString &filename, int pos);
    void addMapInsert(const QString &filename);
    void addMapReplace(QString filename);
    void addSlide();
    void addXLink(const QString &begin, const QString &end, int width,
                  const QString &color, const QString &penstyle);
    int branchCount();
    int centerCount();
    void centerOnID(const QString &id); // tested: ok
    void clearFlags();
    void colorBranch(const QString &color);
    void colorSubtree(const QString &color);
    void copy();
    void cut();
    void cycleTask();
    int depth();
    void detach();
    bool exportMap(QJSValueList args);
    QString getStringAttribute(const QString &key);
    int getIntAttribute(const QString &key);
    int getBranchIndex();
    QString getDestPath();
    QString getFileDir();
    QString getFileName();
    QString getFrameType(const bool & useInnerFrame);
    QString getHeadingPlainText();
    QString getHeadingXML();
    QString getMapAuthor();
    QString getMapComment();
    QString getMapTitle();
    QString getNotePlainText();
    QString getNoteXML();
    qreal getPosX();
    qreal getPosY();
    qreal getScenePosX();
    qreal getScenePosY();
    int getRotationHeading();
    int getRotationSubtree();
    QString getSelectionString();
    int getTaskPriorityDelta();
    QString getTaskSleep();
    int getTaskSleepDays();
    QString getURL();
    QString getVymLink();
    QString getXLinkColor();
    int getXLinkWidth();
    QString getXLinkPenStyle();
    QString getXLinkStyleBegin();
    QString getXLinkStyleEnd();
    bool hasActiveFlag(const QString &flag);
    bool hasNote();
    bool hasRichTextNote();
    bool hasTask();
    void importDir(const QString &path);
    bool initIterator(const QString &iname, bool deepLevelsFirst = false);
    bool isScrolled();
    void loadImage(const QString &filename);
    void loadNote(const QString &filename);
    void moveDown();
    void moveUp();
    void moveSlideDown(int n);
    void moveSlideDown();
    void moveSlideUp(int n);
    void moveSlideUp();
    bool nextIterator(const QString &iname);
    void nop();
    void note2URLs();
    bool parseVymText(const QString &text);
    void paste();
    void redo();
    bool relinkTo(const QString &parent, int num);
    bool relinkTo(const QString &parent);
    void remove();
    void removeChildren();
    void removeKeepChildren();
    void removeSlide(int n);
    QVariant repeatLastCommand();
    void saveImage(const QString &filename);
    void saveNote(const QString &filename);
    void saveSelection(const QString &filename);
    void scroll();
    bool select(const QString &s);
    bool selectID(const QString &s);
    bool selectFirstBranch();
    bool selectFirstChildBranch();
    bool selectLastBranch();
    bool selectLastChildBranch();
    bool selectLastImage();
    bool selectParent();
    bool selectLatestAdded();
    bool selectToggle(const QString &selectString);
    bool selectXLink(int n);
    bool selectXLinkOtherEnd(int n);
    void setDefaultLinkColor(const QString &color); // FIXME-4 maybe also rename other setMap* methods?
    void setAttribute(const QString &key, const QString &value);
    void setFlagByName(const QString &s);
    void setHeadingConfluencePageName();
    void setHeadingPlainText(const QString &s);
    void setHideExport(bool b);
    void setHideLinkUnselected(bool b);
    void setMapAnimCurve(int n);
    void setMapAnimDuration(int n);
    void setMapAuthor(const QString &s);
    void setMapBackgroundColor(const QString &color);
    void setMapComment(const QString &s);
    void setMapLinkStyle(const QString &style);
    void setMapRotation(float a); // tested: ok
    void setMapTitle(const QString &s);
    void setMapZoom(float z); // tested: ok
    void setNotePlainText(const QString &s);
    void setPos(qreal x, qreal y);
    void setFramePenWidth(const bool & useInnerFrame, int w);
    void setFrameBrushColor(const bool & useInnerFrame, const QString &color);
    void setFramePadding(const bool & useInnerFrame, int padding);
    void setFramePenColor(const bool & useInnerFrame, const QString &color);
    void setFrameType(const bool & useInnerFrame, const QString &type);
    void setRotationHeading(const int &i);
    void setRotationSubtree(const int &i);
    void setRotationsAutoDesign(const bool b);
    void setScale(qreal f);
    void setScaleSubtree(qreal f);
    void setScalingAutoDesign(const bool b);
    void setSelectionBrushColor(const QString &color);
    void setSelectionPenColor(const QString &color);
    void setSelectionPenWidth(const qreal &);
    void setTaskPriorityDelta(const int &n);
    bool setTaskSleep(const QString &s);
    void setURL(const QString &s);
    void setVymLink(const QString &s);
    void setXLinkColor(const QString &color);
    void setXLinkStyle(const QString &style);
    void setXLinkStyleBegin(const QString &style);
    void setXLinkStyleEnd(const QString &style);
    void setXLinkWidth(int w);
    void sleep(int n);
    int slideCount();
    void sortChildren(bool b);
    void sortChildren();
    void toggleFlagByUid(const QString &s);
    void toggleFlagByName(const QString &s);
    void toggleScroll();
    void toggleTarget();
    void toggleTask();
    void undo();
    bool unscroll();
    void unscrollChildren();
    void unselectAll();
    void unsetFlagByName(const QString &s);
    int xlinkCount();

  private:
    VymModel *model;
};

#endif
