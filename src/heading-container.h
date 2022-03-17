#ifndef HEADING_CONTAINER_H
#define HEADING_CONTAINER_H

#include "container.h"

class HeadingObj;

#include <QFont>

class HeadingContainer : public Container {
  public:
    HeadingContainer (QGraphicsItem *parent = NULL);
    virtual ~HeadingContainer();
    virtual void init();

    void setHeading(const QString &);
    QString getHeading();
    void setHeadingColor(const QColor &);
    QColor getHeadingColor();
    void setFont(const QFont &);
    QFont getFont();
    void setColor(const QColor &);
    QColor getColor();

    virtual QString getName();
    virtual void reposition();

  protected:
    HeadingObj *headingObj; 

    QString headingText;
    QList<QGraphicsTextItem *> headingLines;
    QColor headingColor;
    QFont headingFont;
};

#endif
