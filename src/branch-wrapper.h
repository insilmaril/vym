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
    int attributeAsInt(const QString &key);
    QString attributeAsString(const QString &key);
    QString headingText();  
    bool relinkTo(BranchWrapper*);
    void select();
    void setFlagByName(const QString &);
    void toggleFlagByName(const QString &);
    void unsetFlagByName(const QString &);

  private:
    BranchItem *branchItem;
};

#endif
