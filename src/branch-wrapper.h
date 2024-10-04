#ifndef BRANCH_WRAPPER_H
#define BRANCH_WRAPPER_H

#include <QObject>

class BranchItem;
class VymModel;
class XLinkWrapper;

class BranchWrapper : public QObject {
    Q_OBJECT
  public:
    Q_INVOKABLE BranchWrapper(BranchItem*);
    ~BranchWrapper();
    BranchItem* branchItem();
    VymModel* model();

  public slots:
    QPointF v_anim();
    qreal v_animX();
    qreal v_animY();
    void setV_anim(qreal, qreal);

    BranchWrapper* addBranch();
    BranchWrapper* addBranchAt(int pos);
    BranchWrapper* addBranchBefore();
    XLinkWrapper* addXLink(BranchWrapper *bwEnd, int width,
                  const QString &color, const QString &penstyle);
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
    qreal getPosX();
    qreal getPosY();
    QPointF getScenePos();
    qreal getScenePosX();
    qreal getScenePosY();
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
    void importDir(const QString &path);
    bool isScrolled();
    bool loadBranchInsert(QString filename, int pos);
    bool loadImage(const QString &filename);
    bool loadNote(const QString &filename);
    void moveDown();
    void moveUp();
    BranchWrapper* parentBranch();
    bool relinkToBranch(BranchWrapper*);
    bool relinkToBranchAt(BranchWrapper*, int pos);
    void removeChildren();
    void removeChildrenBranches();
    void scroll();
    void select();
    bool selectFirstBranch();
    bool selectFirstChildBranch();
    bool selectLastBranch();
    bool selectLastChildBranch();
    bool selectParent();
    bool selectXLink(int n);
    bool selectXLinkOtherEnd(int n);
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
    void sortChildren(bool b);
    void sortChildren();
    void toggleFlagByName(const QString &);
    void toggleFlagByUid(const QString &);
    void toggleScroll();
    void toggleTask();
    void unscroll();
    void unscrollSubtree();
    void unsetFlagByName(const QString &);
    int xlinkCount();

  private:
    BranchItem *branchItemInt;
};

#endif
