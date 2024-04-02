#ifndef HEADING_CONTAINER_H
#define HEADING_CONTAINER_H

#include "container.h"

class HeadingObj;

#include <QFont>

class HeadingContainer : public Container {
  public:
    HeadingContainer ();
    virtual ~HeadingContainer();
    virtual void init();

  private:
    QGraphicsTextItem *newLine(QString); // generate new textline

  public:
    void setHeading(QString);   // FIXME-3 use reference to TreeItem::heading()
    QString heading();
    void clearHeading();
    void setHeadingColor(const QColor &);
    QColor headingColor();
    void setFont(const QFont &);
    QFont font();
    void setColor(const QColor &);
    QColor color();
    void setTextWidth(const int &);
    int textWidth();

    virtual QString getName();

    void setScrollOpacity(qreal);
    qreal getScrollOpacity();

    virtual void reposition();

  protected:
    QString headingTextInt;
    QList<QGraphicsTextItem *> headingLines;
    QColor headingColorInt;
    QFont headingFontInt;
    int textWidthInt;
};

#endif
