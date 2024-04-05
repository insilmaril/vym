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
    QString headingText();  
    bool isScrolled();
    bool relinkTo(BranchWrapper*);
    void remove();
    void scroll();
    void select();
    void setFlagByName(const QString &);
    void toggleFlagByName(const QString &);
    void toggleScroll();
    void unscroll();
    void unsetFlagByName(const QString &);

  private:
    BranchItem *branchItem;
};

#endif
