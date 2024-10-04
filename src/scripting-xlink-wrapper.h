#ifndef XLINK_WRAPPER_H
#define XLINK_WRAPPER_H

#include <QObject>

#include "scripting.h"

class XLink;
class VymModel;

class XLinkWrapper : public QObject {
    Q_OBJECT
  public:
    Q_INVOKABLE XLinkWrapper(XLink*);
    ~XLinkWrapper();
    VymModel* model();
    XLink* xlink();

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
    XLink *xlinkInt;
};

Q_DECLARE_METATYPE(XLinkWrapper)
Q_DECLARE_METATYPE(XLinkWrapper*)
#endif
