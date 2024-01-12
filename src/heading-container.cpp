#include <QDebug>

#include "heading-container.h"

#define qdbg() qDebug().nospace().noquote()

HeadingContainer::HeadingContainer()

{
    //qDebug() << "* Const HeadingContainer begin this = ";
    init();
}

HeadingContainer::~HeadingContainer()
{
    //qDebug() << "* Destr HeadingContainer" << getName();

    clearHeading();
}

void HeadingContainer::init()
{
    containerType = Container::Heading;

    setHeading(" ");
    headingColor = QColor(Qt::black);

    layout = Vertical;
    horizontalAlignment = AlignedLeft;
}

QGraphicsTextItem *HeadingContainer::newLine(QString s)
{
    QGraphicsTextItem *t = new QGraphicsTextItem(s, this);
    t->setFont(headingFont);
    t->setDefaultTextColor(headingColor);

    headingLines.append(t);

    return t;
}

void HeadingContainer::setHeading(QString s)// FIXME-2 richtext has wrong position
{
    if (headingText == s) return;

    headingText = s;

    // Textwidth Hardcoded for now // FIXME-1   See also new setting BranchPropertyEditor
                                   // Maybe use both number of chars (for wordwrap) and 
                                   // also pixels (for size of OrnamentsContainer)
    int textWidth = 40;

    QGraphicsTextItem *t;

    // remove old textlines and prepare generating new ones
    clearHeading();

    if (s.startsWith("<html>") ||
        s.startsWith("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" "
                     "\"http://www.w3.org/TR/REC-html40/strict.dtd\">")) {
        t = new QGraphicsTextItem(this);
        t->setFont(headingFont);
        t->setHtml(s);
        t->setDefaultTextColor(headingColor);
        headingLines.append(t);

       // Translate line to move center to origin
        t->setPos(-t->boundingRect().center());
        setRect(t->boundingRect());
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

        QRegExp re("<br.*/>");
        re.setMinimal(true);

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
                t = newLine(s);
                s = "";
            }
            else {
                if (i < 0 && j > 0) { // no ws found in actual search
                    if (s.length() <= textWidth) {
                        t = newLine(s);
                        s = "";
                    }
                    else {
                        t = newLine(s.left(j));
                        s = s.mid(j + 1, s.length());
                        j = 0;
                    }
                }
                else {
                    if (i >= 0 &&
                        i <= static_cast<int>(
                                 textWidth)) { // there is a ws in textWidth
                        if (br > 0) {
                            // here is a linebreak
                            t = newLine(s.left(i));
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
                        if (i > static_cast<int>(textWidth)) {
                            if (j > 0) { // a ws out of textWidth, but we have
                                         // also one in
                                t = newLine(s.left(j));
                                s = s.mid(j + 1, s.length());
                                i = 0;
                                j = 0;
                            }
                            else { // a ws out of text, but none in
                                t = newLine(s.left(i));
                                s = s.mid(i + 1, s.length());
                                i = 0;
                            }
                        }
                    }
                }
            }
        }
    } // ASCII heading with multiple lines

    // Align headingLines vertically and find center
    qreal h = 0;
    qreal w = 0;
    qreal w_max = 0;
    foreach (QGraphicsTextItem *ti, headingLines) {
        ti->setPos(0, h);
        h += ti->boundingRect().height();
        w = ti->boundingRect().width();
        w_max = (w_max < w) ? w : w_max;
    }
    // Translate all lines to move center to origin
    QPointF v(-w_max / 2, - h / 2);

    foreach (QGraphicsTextItem *ti, headingLines)
        ti->setPos(ti->pos() + v);

    setRect(v.x(), v.y(), w_max, h);

    setName(QString("HC (%1)").arg(s));
}

QString HeadingContainer::getHeading() { return headingText; }

void HeadingContainer::clearHeading()
{
    while (!headingLines.isEmpty())
        delete headingLines.takeFirst();
    setRect(QRectF());
}

void HeadingContainer::setHeadingColor(const QColor &col)
{
    if (headingColor != col) {
        headingColor = col;
        for (int i = 0; i < headingLines.size(); ++i)
            // TextItem
            headingLines.at(i)->setDefaultTextColor(headingColor);
        // SimpleTextItem
        // headingLines.at(i)->setBrush(headingColor);
    }
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
    return Container::getName() + QString(" '%1'").arg(headingText);
}

void HeadingContainer::setScrollOpacity(qreal o) // FIXME-2 needed?
{
    qDebug() << "HC::setScrollOpacity o=" << o;
    foreach (QGraphicsTextItem *ti, headingLines)
        ti->setOpacity(o);
}

qreal HeadingContainer::getScrollOpacity() // FIXME-2 needed?
{
    return 0;
}

void HeadingContainer::reposition()
{
    // qdbg() << ind() << "HC::reposition " + info();
    return;
}

