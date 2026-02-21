#ifndef HEADING_CONTAINER_H
#define HEADING_CONTAINER_H

#include "container.h"
#include "vymtext.h"

#include <QFont>

class HeadingContainer : public Container {
  public:
    HeadingContainer ();
    virtual ~HeadingContainer();
    virtual void init();

  private:
    QGraphicsTextItem *newLine(QString); // generate new textline

  public:
    void setHeading(const VymText &);
    void clearHeading();
    void setFont(const QFont &);
    QFont font();
    void setColor(const QColor &);
    void setColumnWidth(const int &);
    int columnWidth();

    virtual QString getName();

    void setScrollOpacity(qreal);
    qreal getScrollOpacity();

    virtual void reposition();

  protected:
    VymText headingInt;
    QList<QGraphicsTextItem *> headingLines;
    QColor headingColorInt;
    QFont headingFontInt;
    int columnWidthInt;
};

#endif
