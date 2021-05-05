#include <QDebug>
#include <QGraphicsScene>
#include <QRegExp>

#include "headingobj.h"

extern bool debug;

/////////////////////////////////////////////////////////////////
// HeadingObj
/////////////////////////////////////////////////////////////////
HeadingObj::HeadingObj(QGraphicsItem *parent) : MapObj(parent)
{
    qDebug() << "Const HeadingObj (s) ";
    init();
}

HeadingObj::~HeadingObj()
{
    qDebug() << "Destr. HeadingObj " << heading;
    while (!textline.isEmpty())
        delete textline.takeFirst();
}

void HeadingObj::init()
{
    textwidth = 40;
    color = QColor("black");
    font = QFont();
    heading = "";
    angle = 0;
}

void HeadingObj::copy(HeadingObj *other)
{
    MapObj::copy(other);
    textwidth = other->textwidth;
    color = other->color;
    font = other->font;
    setText(other->text());
}

void HeadingObj::move(double x, double y)
{
    MapObj::move(x, y);

    qreal h;  // height of a textline
    qreal ho; // offset of height while drawing all lines

    if (!textline.isEmpty())
        h = textline.first()->boundingRect().height();
    else
        h = 2;
    ho = 0;
    for (int i = 0; i < textline.size(); ++i) {
        textline.at(i)->setPos(x, y + ho);
        ho = ho + h;
    }
}

void HeadingObj::moveBy(double x, double y)
{
    move(x + absPos.x(), y + absPos.y());
}

void HeadingObj::positionBBox()
{
    bbox.setX(absPos.x());
    bbox.setY(absPos.y());
}

void HeadingObj::calcBBoxSize()
{
    qreal w = 0;
    qreal h = 0;
    // Using Backspace an empty heading might easily be created, then there
    // would be textline.first()==NULL This can be worked around by the
    // following, but then no selection would be visible, thus we prevent it in
    // ::setText()
    if (!textline.isEmpty()) {
        for (int i = 0; i < textline.size(); i++) {
            h += textline.at(i)->boundingRect().height();
            if (w < textline.at(i)->boundingRect().width())
                w = textline.at(i)->boundingRect().width();
        }
    }
    bbox.setSize(QSizeF(w, h));
}

QGraphicsTextItem *HeadingObj::newLine(QString s)
{
    QGraphicsTextItem *t = new QGraphicsTextItem(s, parentItem());
    t->setFont(font);
    t->setZValue(dZ_TEXT);
    t->setDefaultTextColor(color);
    t->setRotation(angle);
    return t;
}

void HeadingObj::setTransformOriginPoint(const QPointF &p)
{
    for (int i = 0; i < textline.size(); i++) {
        textline.at(i)->setTransformOriginPoint(p);
    }
}

void HeadingObj::setRotation(const qreal &a)
{
    angle = a;
    for (int i = 0; i < textline.size(); i++)
        textline.at(i)->setRotation(angle);
}

qreal HeadingObj::getRotation() { return angle; }

void HeadingObj::setText(QString s)
{
    heading = s;

    // remove old textlines and prepare generating new ones
    while (!textline.isEmpty())
        delete textline.takeFirst();

    if (s.startsWith("<html>") ||
        s.startsWith("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" "
                     "\"http://www.w3.org/TR/REC-html40/strict.dtd\">")) {
        QGraphicsTextItem *t = new QGraphicsTextItem();
        t->setFont(font);
        t->setZValue(dZ_TEXT);
        t->setHtml(s);
        t->setDefaultTextColor(color);
        t->setRotation(angle);
        scene()->addItem(t);
        textline.append(t);
    }
    else {
        // prevent empty textline, so at least a small selection stays
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
                textline.append(newLine(s));
                s = "";
            }
            else {
                if (i < 0 && j > 0) { // no ws found in actual search
                    if (s.length() <= textwidth) {
                        textline.append(newLine(s));
                        s = "";
                    }
                    else {
                        textline.append(newLine(s.left(j)));
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
                            textline.append(newLine(s.left(i)));
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
                                textline.append(newLine(s.left(j)));
                                s = s.mid(j + 1, s.length());
                                i = 0;
                                j = 0;
                            }
                            else { // a ws out of text, but none in
                                textline.append(newLine(s.left(i)));
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
}

QString HeadingObj::text() { return heading; }

void HeadingObj::setFont(QFont f)
{
    if (font != f) {
        font = f;
        setText(text());
    }
}

QFont HeadingObj::getFont() { return font; }

void HeadingObj::setColor(QColor c)
{
    if (color != c) {
        color = c;
        for (int i = 0; i < textline.size(); ++i)
            // TextItem
            textline.at(i)->setDefaultTextColor(c);
        // SimpleTextItem
        // textline.at(i)->setBrush(c);
    }
}

QColor HeadingObj::getColor() { return color; }

void HeadingObj::setZValue(double z)
{
    for (int i = 0; i < textline.size(); ++i)
        textline.at(i)->setZValue(z);
}

void HeadingObj::setVisibility(bool v)
{
    MapObj::setVisibility(v);
    for (int i = 0; i < textline.size(); ++i)
        if (v)
            textline.at(i)->show();
        else
            textline.at(i)->hide();
}

qreal HeadingObj::getHeight() { return bbox.height(); }

qreal HeadingObj::getWidth() { return bbox.width(); }
