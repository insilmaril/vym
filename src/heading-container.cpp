#include <QDebug>

#include "heading-container.h"

#include "headingobj.h" //FIXME-2 probably move content from HO here to HC later
#include "mapobj.h"     // FIXME-2 needed?

HeadingContainer::HeadingContainer(QGraphicsItem *parent) : Container(parent) 

{
    //qDebug() << "* Const HeadingContainer begin this = " << this;
    init();
}

HeadingContainer::~HeadingContainer()
{
    //qDebug() << "* Destr HeadingContainer" << name << this;

    delete headingObj;
}

void HeadingContainer::init()
{
    type = Container::Heading;

    headingObj = new HeadingObj(this);
    setHeading("");
    headingColor = QColor(Qt::black);
}

void HeadingContainer::setHeading(const QString &s)// FIXME-2 richtext has wrong position
{
    // Update heading in container  // FIXME-0 move to container here
    if (s != headingObj->text()) headingObj->setText(s);

    /* FIXME-0 refactor below from HO to container
    heading = s;

    // remove old textlines and prepare generating new ones
    while (!headingLines.isEmpty())
        delete headingLines.takeFirst();

    if (s.startsWith("<html>") ||
        s.startsWith("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" "
                     "\"http://www.w3.org/TR/REC-html40/strict.dtd\">")) {
        QGraphicsTextItem *t = new QGraphicsTextItem(parentItem());
        t->setFont(font);
        t->setZValue(dZ_TEXT);
        t->setHtml(s);
        t->setDefaultTextColor(color);
        t->setRotation(angle);
        headingLines.append(t);
    }
    else {
        // prevent empty headingLines, so at least a small selection stays
        // visible for this heading
        if (s.length() == 0)
            s = "  ";

        int i = 0;  // index for actual search for ws
        int j = 0;  // index of last ws
        int k = 0;  // index of "<br>" or similar linebreak
        int br = 0; // width of found break, e.g. for <br> it is 4
        */
        QRegExp re("<br.*/>");
        re.setMinimal(true);
        /*

        // set the text and wrap lines
        while (s.length() > 0) {
            // ok, some people wanted manual linebreaks, here we go
            k = re.indexIn(s, i);
            if (k >= 0) {
                br = re.cap(0).length();
                i = k;
            }
            else
                i = s.indexOf(" ", i);
            if (i < 0 && j == 0) { // no ws found at all in s
                // append whole s
                headingLines.append(newLine(s));
                s = "";
            }
            else {
                if (i < 0 && j > 0) { // no ws found in actual search
                    if (s.length() <= textwidth) {
                        headingLines.append(newLine(s));
                        s = "";
                    }
                    else {
                        headingLines.append(newLine(s.left(j)));
                        s = s.mid(j + 1, s.length());
                        j = 0;
                    }
                }
                else {
                    if (i >= 0 &&
                        i <= static_cast<int>(
                                 textwidth)) { // there is a ws in textwidth
                        if (br > 0) {
                            // here is a linebreak
                            headingLines.append(newLine(s.left(i)));
                            s = s.mid(i + br, s.length());
                            i = 0;
                            j = 0;
                            br = 0;
                        }
                        else {
                            j = i;
                            i++;
                        }
                    }
                    else {
                        if (i > static_cast<int>(textwidth)) {
                            if (j > 0) { // a ws out of textwidth, but we have
                                         // also one in
                                headingLines.append(newLine(s.left(j)));
                                s = s.mid(j + 1, s.length());
                                i = 0;
                                j = 0;
                            }
                            else { // a ws out of text, but none in
                                headingLines.append(newLine(s.left(i)));
                                s = s.mid(i + 1, s.length());
                                i = 0;
                            }
                        }
                    }
                }
            }
        }
    } // ASCII heading with multiple lines
    setVisibility(visible);
    move(absPos.x(), absPos.y());
    calcBBoxSize();
    */
    QRectF r = rect();
    if (!s.isEmpty())  {
        r.setWidth(headingObj->getBBox().width());
        r.setHeight(headingObj->getBBox().height());
    }
    setRect(r);

    setName(QString("HC (%1)").arg(s));
}

QString HeadingContainer::getHeading() { return headingText; }

void HeadingContainer::setHeadingColor(const QColor &col) 
{
    headingObj->setColor(col);  // FIXME-1 move code from HeadingObj to HeadingContainer

    headingColor = col;
}

QColor HeadingContainer::getHeadingColor() {
    return headingColor;
}

void HeadingContainer::setFont(const QFont &f)
{
    if (headingFont != f) {
        headingFont = f;
        setHeading(headingText);
    }
}

QFont HeadingContainer::getFont() {return headingFont;}

void HeadingContainer::setColor(const QColor &c)
{
    if (headingColor != c) {
        headingColor = c;
        for (int i = 0; i < headingLines.size(); ++i)
            // TextItem
            headingLines.at(i)->setDefaultTextColor(headingColor);
        // SimpleTextItem
        // headingLines.at(i)->setBrush(c);
    }
}

QColor HeadingContainer::getColor()
{
    return headingColor;
}

QString HeadingContainer::getName() {
    return Container::getName() + " '" + headingObj->text() + "'";
}

void HeadingContainer::reposition()
{
    // qDebug() << "HC::reposition " + info();
    return;
}

