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
    void setHeading(const VymText &);  // FIXME-0 switch to Heading instead of QString. Also remove color, font, textWidth here
    void setHeading(QString);  // FIXME-0 switch to Heading instead of QString. Also remove color, font, textWidth here
    QString heading();  // FIXME-0 Really a getter needed?
    void clearHeading();
    void setFont(const QFont &);
    QFont font();
    void setColor(const QColor &);
    void setColumnWidth(const int &);
    int columnWidth();  // FIXME-0 really a getter needed?

    virtual QString getName();

    void setScrollOpacity(qreal);
    qreal getScrollOpacity();

    virtual void reposition();

  protected:
    VymText headingInt;
    QString headingTextInt;         // FIXME-0 remove
    QList<QGraphicsTextItem *> headingLines;
    QColor headingColorInt;         // FIXME-0 remove
    QFont headingFontInt;           // FIXME-0 remove
    int columnWidthInt;             // FIXME-0 remove
};

#endif
