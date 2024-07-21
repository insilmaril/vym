#ifndef FOO_WRAPPER_H
#define FOO_WRAPPER_H

#include "scripting.h"

class VymModel;
class XLink;

class FooWrapper : public VymScriptContext {
    Q_OBJECT
  public:
    FooWrapper();
    ~FooWrapper();
    VymModel* model();
    XLink* xlink();

  public slots:
    int getWidth();

  private:
    XLink* xlinkInt;
};

#endif
