#ifndef HEADING_CONTAINER_H
#define HEADING_CONTAINER_H

#include "container.h"

class HeadingObj;

#include <QFont>

class HeadingContainer : public Container {
  public:
    HeadingContainer (QGraphicsItem *parent = nullptr);
    virtual ~HeadingContainer();
    virtual void init();

  private:
    QGraphicsTextItem *newLine(QString); // generate new textline

  public:
    virtual void setVisibility(bool);
    void setHeading(QString);
    QString getHeading();
    void clearHeading();
    void setHeadingColor(const QColor &);
    QColor getHeadingColor();
    void setFont(const QFont &);
    QFont getFont();
    void setColor(const QColor &);
    QColor getColor();

    virtual QString getName();

    void setScrollOpacity(qreal);
    qreal getScrollOpacity();

    virtual void reposition();

  protected:
    QString headingText;
    QList<QGraphicsTextItem *> headingLines;
    QColor headingColor;
    QFont headingFont;
};

#endif
