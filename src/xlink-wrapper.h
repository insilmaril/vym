#ifndef XLINK_WRAPPER_H
#define XLINK_WRAPPER_H

#include "scripting.h"

class Link;
class VymModel;

class XLinkWrapper : public VymScriptContext {
    Q_OBJECT
  public:
    XLinkWrapper(Link*);
    ~XLinkWrapper();
    VymModel* model();
    Link* xlink();

  public slots:
    QString headingText();  

  private:
    Link *xlinkInt;
};

#endif
