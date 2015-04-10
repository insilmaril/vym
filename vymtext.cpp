#include "vymtext.h"
#include "misc.h"

#include <QRegExp>
#include <QDebug>

/////////////////////////////////////////////////////////////////
// VymText  Base class for VymNotes and Headings
/////////////////////////////////////////////////////////////////

VymText::VymText()
{
    clear();
}
VymText::VymText(const VymText &other)
{
    clear();
    copy (other);
    return;
}

VymText::VymText(const QString &s)
{
    clear();
    setPlainText (s);
}

bool VymText::operator== (const VymText &other)
{
    if ( text == other.text &&
         fonthint == other.fonthint &&
        textmode == other.textmode &&
        filenamehint == other.filenamehint
    )
        return true;
    else
        return false;
}

void VymText::operator= (const VymText &other)
{
    copy (other);
}

void VymText::copy (const VymText &other)
{
    text = other.text;
    fonthint = other.fonthint;
    filenamehint = other.filenamehint;
    textmode = other.textmode;
    color = other.color;
}

void VymText::clear()
{
    text = "";
    fonthint = "undef";
    filenamehint = "";
    textmode = AutoText;
    color = Qt::black;
}

void VymText::setRichText(bool b)
{
    if (b)
        textmode = RichText;
    else
        textmode = PlainText;
}

bool VymText::isRichText()const
{
    if (textmode == RichText)
        return true;
    else
        return false;
}

void VymText::setText (const QString &s)
{
    text = s;
}

void VymText::setRichText (const QString &s)
{
    text = s;
    textmode = RichText;
}

void VymText::setPlainText (const QString &s)
{
    text = unquotemeta(s);      // FIXME-0 really unquote? Or better ascii?
    textmode = PlainText;
}

QString VymText::getText() const
{
    return text;
}

QString VymText::getTextASCII() const
{
    return getTextASCII ("",80);
}

QString VymText::getTextASCII(QString indent, const int &) const //FIXME-3 use width
{
    if (text.isEmpty()) return text;

    QString r=text;
    QRegExp rx;
    rx.setMinimal(true);

    r = richTextToPlain( r );

    // Indent everything
    rx.setPattern ("^\n");
    r.replace (rx,indent);
    r =indent + r;   // Don't forget first line

/* FIXME-3  wrap text at width
    if (fonthint !="fixed")
    {
    }
*/
    return r;
}

QString VymText::getTextOpenDoc()
{
    // Evil hack to transform QT Richtext into
    // something which can be used in OpenDoc format
    // 
    // TODO create clean XML transformation which also
    // considers fonts, colors, ...

    QString r=text;

    // Remove header
    QRegExp re("<head>.*</head>");
    re.setMinimal(true);
    r.replace (re,"");

    // convert all "<br*>"
    re.setPattern ("<br.*>");
    re.setMinimal(true);
    r.replace (re,"<text:line-break/>");

    // convert all "<p>" 
    re.setPattern ("<p>");
    r.replace (re,"<text:line-break/>");
    
    // Remove all other tags, e.g. paragraphs will be added in 
    // templates used during export
    re.setPattern ("</?html.*>");
    r.replace (re,"");
    re.setPattern ("</?body.*>");
    r.replace (re,"");
    re.setPattern ("</?meta.*>");
    r.replace (re,"");
    re.setPattern ("</?span.*>");
    r.replace (re,"");
    re.setPattern ("</?p.*>");
    r.replace (re,"");

    r="<text:span text:style-name=\"vym-notestyle\">"+r+"</text:span>";
    return r;
}

void VymText::setFontHint (const QString &s)
{
    // only for backward compatibility (pre 1.5 )
    fonthint=s;
}

QString VymText::getFontHint() const
{
    // only for backward compatibility (pre 1.5 )
    return fonthint;
}

void VymText::setFilenameHint (const QString &s)
{
    filenamehint=s;
}

QString VymText::getFilenameHint() const
{
    return filenamehint;
}

bool VymText::isEmpty ()
{
    return text.isEmpty();
}

void VymText::setColor(QColor col)
{
    color = col;
}

QColor VymText::getColor()
{
    return color;
}

QString VymText::getAttributes()
{
    QString ret;
    if (textmode == RichText)
        ret += attribut("textMode","richText");
    else
    {
        ret += attribut("textMode","plainText");
        ret += " " + attribut("fonthint", fonthint);
        ret += " " + attribut("textColor", color.name() );
    }
    return ret;
}

QString VymText::saveToDir ()
{
    return "<![CDATA[" + text + "]]>";
}
