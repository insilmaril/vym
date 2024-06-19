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

  public slots:
    void addBranch();
    void addBranchAt(int pos);
    void addBranchBefore();
    int attributeAsInt(const QString &key);
    QString attributeAsString(const QString &key);
    int branchCount();
    void clearFlags();
    void colorBranch(const QString &color);
    void colorSubtree(const QString &color);
    void getJiraData(bool subtree);
    QString getNoteText();
    QString getNoteXML();
    bool hasNote();
    bool hasRichTextNote();
    QString headingText();  
    bool isScrolled();
    bool relinkTo(BranchWrapper*);
    void remove();
    void scroll();
    void select();
    bool selectFirstBranch();// FIXME-3 move to BranchWrapper
    bool selectLastBranch();// FIXME-3 move to BranchWrapper
    bool selectParent();
    void setAttribute(const QString &key, const QString &value);
    void setFlagByName(const QString &);
    void setHeadingRichText(const QString &);
    void setHeadingText(const QString &);
    void setNoteRichText(const QString &);
    void setNoteText(const QString &);
    void setPos(qreal x, qreal y);// FIXME-2 copy to ImageWrapper
    void toggleFlagByName(const QString &);
    void toggleScroll();
    void unscroll();
    void unsetFlagByName(const QString &);

  private:
    BranchItem *branchItem;
};

#endif
