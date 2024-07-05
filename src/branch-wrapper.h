#ifndef BRANCH_WRAPPER_H
#define BRANCH_WRAPPER_H

#include "scripting.h"

class BranchItem;
class VymModel;

class BranchWrapper : public VymScriptContext {
    Q_OBJECT
  public:
    BranchWrapper(BranchItem*);
    ~BranchWrapper();
    BranchItem* branchItem();
    VymModel* model();

  public slots:
    void addBranch();
    void addBranchAt(int pos);
    void addBranchBefore();
    bool addMapInsert(QString filename, int pos);
    int attributeAsInt(const QString &key);
    QString attributeAsString(const QString &key);
    int branchCount();
    void clearFlags();
    void colorBranch(const QString &color);
    void colorSubtree(const QString &color);
    bool cycleTask(bool reverse = false);
    int getFramePadding(const bool & useInnerFrame);
    int getFramePenWidth(const bool & useInnerFrame);
    QString getFrameType(const bool & useInnerFrame);
    QString getUid();
    void getJiraData(bool subtree);
    QString getNoteText();
    QString getNoteXML();
    int getNum();
    qreal getPosX();            // FIXME-3 copy for image
    qreal getPosY();            // FIXME-3 copy for image
    int getTaskPriorityDelta();
    QString getTaskSleep();
    int getTaskSleepDays();
    QString getTaskStatus();
    QString getUrl();
    QString getVymLink();
    bool hasActiveFlag(const QString &flag);
    bool hasNote();
    bool hasRichTextHeading();
    bool hasRichTextNote();
    bool hasTask();
    QString headingText();  
    int imageCount();
    bool isScrolled();
    bool loadImage(const QString &filename);
    bool loadNote(const QString &filename);
    void moveDown();
    void moveUp();
    bool relinkToBranch(BranchWrapper*);
    bool relinkToBranchAt(BranchWrapper*, int pos);
    void removeChildren();
    void removeChildrenBranches();
    void scroll();
    void select();
    bool selectFirstBranch();
    bool selectLastBranch();
    bool selectParent();
    void setAttribute(const QString &key, const QString &value);
    void setFlagByName(const QString &);
    void setFrameBrushColor(const bool & useInnerFrame, const QString &color);
    void setFramePadding(const bool & useInnerFrame, int padding);
    void setFramePenColor(const bool & useInnerFrame, const QString &color);
    void setFramePenWidth(const bool & useInnerFrame, int w);
    void setFrameType(const bool & useInnerFrame, const QString &type);
    void setHeadingRichText(const QString &);
    void setHeadingText(const QString &);
    void setNoteRichText(const QString &);
    void setNoteText(const QString &);
    void setPos(qreal x, qreal y);// FIXME-2 copy to ImageWrapper
    void setTaskPriorityDelta(const int &n);
    bool setTaskSleep(const QString &s);
    void setUrl(const QString &s);
    void setVymLink(const QString &s);
    void toggleFlagByName(const QString &);
    void toggleFlagByUid(const QString &);
    void toggleScroll();
    void toggleTask();
    void unscroll();
    void unsetFlagByName(const QString &);

  private:
    BranchItem *branchItemInt;
};

#endif
