#include "xmlobj.h"

#include <QRegExp>
#include <QString>


// returns masked "<" ">" "&"
QString quotemeta(const QString &s)
{
    QString r=s;
    QRegExp  rx("&(?!amp;)");
    r.replace ( rx,"&amp;");
    rx.setPattern( ">");
    r.replace ( rx,"&gt;");
    rx.setPattern( "<");
    r.replace ( rx,"&lt;");
    rx.setPattern( "\"");
    r.replace ( rx,"&quot;");
    return r;
}

QString unquotemeta(const QString &s)
{
    QString r=s;
    QRegExp  rx("&amp;)");
    r.replace ( rx,"&");
    rx.setPattern( "&gt;");
    r.replace ( rx,">");
    rx.setPattern( "&lt;");
    r.replace ( rx,"<");
    rx.setPattern( "&quot;");
    r.replace ( rx,"\"");
    return r;
}

QString quoteUmlaut(const QString &s)
{
    QString r=s;
    QRegExp rx( "ü");
    r.replace ( rx,"&uuml;");
    rx.setPattern( "Ü");
    r.replace ( rx,"&Uuml;");
    rx.setPattern( "ö");
    r.replace ( rx,"&ouml;");
    rx.setPattern( "Ö");
    r.replace ( rx,"&Ouml;");
    rx.setPattern( "ä");
    r.replace ( rx,"&auml;");
    rx.setPattern( "Ö");
    r.replace ( rx,"&Auml;");
    rx.setPattern( "ß");
    r.replace ( rx,"&szlig;");
    rx.setPattern( "€");
    r.replace ( rx,"&euro;");
    return r;
}

int XMLObj::curindent=0;	// make instance of curindent

XMLObj::XMLObj()
{
    indentwidth=4;
}

XMLObj::~XMLObj()
{
}


// returns <s at />
QString XMLObj::singleElement(QString s, QString at)
{
    return indent() + "<" + s +" " + at +" " + "/>";
}

// returns <s>
QString XMLObj::beginElement(QString s)
{
    return indent() + "<" + s + ">";
}

// returns <s at>
QString XMLObj::beginElement(QString s, QString at)
{
    return indent() + "<" + s + " " + at + ">";
}

// returns </s>
QString XMLObj::endElement(QString s)
{
    return indent() + "</" + s + ">";
}

// returns  at="val"
QString XMLObj::attribut(QString at, QString val)
{
    return " " + at + "=\"" + val + "\"";
}

// returns <s> val </s>
QString XMLObj::valueElement(QString el, QString val)
{
    return indent() + "<" + el + ">" + val + "</" +el + ">";
}

// returns <s at> val </s>
QString XMLObj::valueElement(QString el, QString val, QString at)
{
    return indent() + "<" + el + " " + at + ">" + val + "</" +el + ">";
}

void XMLObj::incIndent()
{
    curindent++;
}   

void XMLObj::decIndent()
{
    curindent--;
    if (curindent<0) curindent=0;
}   

QString XMLObj::indent()
{
    QString s = "\n";
    int i;
    for (i=0; i < curindent*indentwidth; i++)
    {
        s= s + " ";
    }
    return s;
}   

