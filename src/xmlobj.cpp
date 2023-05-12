#include "xmlobj.h"

#include <QRegExp>
#include <QStringList>

// returns masked '<' '>' '&'  '"'
QString quoteMeta(const QString &s)
{
    QString r = s;
    QRegExp rx("&(?!amp;)");
    r.replace(rx, "&amp;");
    rx.setPattern(">");
    r.replace(rx, "&gt;");
    rx.setPattern("<");
    r.replace(rx, "&lt;");
    rx.setPattern("\"");
    r.replace(rx, "&quot;");
    rx.setPattern("\n");
    r.replace(rx, "\\n");
    return r;
}

QString unquoteMeta(const QString &s)
{
    QString r = s;
    QRegExp rx("&amp;)");
    r.replace(rx, "&");
    rx.setPattern("&gt;");
    r.replace(rx, ">");
    rx.setPattern("&lt;");
    r.replace(rx, "<");
    rx.setPattern("&quot;");
    r.replace(rx, "\"");
    rx.setPattern("\\\\n");
    r.replace(rx, "\n");
    return r;
}

QString quoteQuotes(const QString &s)
{
    QString r = s;

    QRegExp rx("\"");
    r.replace(rx, "\\\"");

    rx.setPattern("\n");
    r.replace(rx, "\\n");

    return r;
}

QString unquoteQuotes(const QString &s)
{
    QString r = s;

    QRegExp rx("\\\\\"");
    r.replace(rx, "\"");

    rx.setPattern("\\\\n");
    r.replace(rx, "\n");

    return r;
}

QString getCDATA(const QString &s)
{
    // Check, if we need to use CDATA after all
    if (s.contains("<") || s.contains(">") || s.contains("\"") ||
        s.contains("&")) {
        QStringList list = s.split("]]>");
        int i = s.indexOf("]]>");
        if (i >= 0)
            // split up and calculate recursively
            return getCDATA(s.left(i + 2)) +
                   getCDATA(s.right(s.length() - i - 2));
        else
            return "<![CDATA[" + s + "]]>";
    }
    else
        return s;
}

int XMLObj::curIndent = 0; // make instance of curIndent

XMLObj::XMLObj() { indentWidth = 4; }

XMLObj::~XMLObj() {}

// returns <s at />
QString XMLObj::singleElement(QString s, QString at)
{
    return indent() + "<" + s + " " + at + " />";
}

// returns <s at at at at ... />
QString XMLObj::singleElement(QString s, QStringList attributes)
{
    return indent() + "<" + s + " " + attributes.join(" ") + " />";
}

// returns <s>
QString XMLObj::beginElement(QString s) { return indent() + "<" + s + ">"; }

// returns <s at>
QString XMLObj::beginElement(QString s, QString at)
{
    return indent() + "<" + s + " " + at + ">";
}

// returns </s>
QString XMLObj::endElement(QString s) { return indent() + "</" + s + ">"; }

// returns  at="val"
QString XMLObj::attribut(QString at, QString val) const // FIXME-2 rename to "attribute" to fix spelling
{
    return " " + at + "=\"" + quoteMeta(val) + "\"";
}

// returns <s> val </s>
QString XMLObj::valueElement(QString el, QString val)
{
    return indent() + "<" + el + ">" + val + "</" + el + ">";
}

// returns <s at> val </s>
QString XMLObj::valueElement(QString el, QString val, QString at)
{
    return "<" + el + " " + at + ">" + val + "</" + el + ">";
}

void XMLObj::incIndent() { curIndent++; }

void XMLObj::decIndent()
{
    curIndent--;
    if (curIndent < 0)
        curIndent = 0;
}

QString XMLObj::indent()
{
    QString s = "\n";
    for (int i = 0; i < curIndent * indentWidth; i++)
        s += " ";
    return s;
}
