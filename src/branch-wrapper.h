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
    QString headingText();  
    void setFlagByName(const QString &);
    void toggleFlagByName(const QString &);
    void unsetFlagByName(const QString &);

  private:
    BranchItem *branchItem;
};

#endif
