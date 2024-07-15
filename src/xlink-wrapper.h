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
    QString getColor();
    int getWidth();
    QString getPenStyle();
    QString getStyleBegin();
    QString getStyleEnd();
    void setColor(const QString &color);
    void setStyle(const QString &styGle);
    void setStyleBegin(const QString &style);
    void setStyleEnd(const QString &style);
    void setWidth(int w);

  private:
    Link *xlinkInt;
};

#endif
