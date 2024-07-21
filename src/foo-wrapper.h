#ifndef FOO_WRAPPER_H
#define FOO_WRAPPER_H

#include "scripting.h"

class VymModel;
class XLink;

class XLinkWrapper : public VymScriptContext {
    Q_OBJECT
  public:
    XLinkWrapper();
    ~XLinkWrapper();
    VymModel* model();
    XLink* xlink();

  public slots:
    int getWidth();

  private:
    XLink* xlinkInt;
};

#endif
