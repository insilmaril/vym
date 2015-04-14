#include "vymtext.h"
#include "misc.h"

#include <QRegExp>
#include <QDebug>

/////////////////////////////////////////////////////////////////
// VymText  Base class for Vymnotes and Headings
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
    clear();
    text = s;
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

    int width = 80;
    QString s;
    QRegExp rx;
    rx.setMinimal(true);

    if (isRichText())
        s = text;
    else
    {
        if ( fonthint == "fixed")
        {
            s = text;
        } else
        {
            // Wordwrap

            QString newnote;
            QString curline;
            uint n=0;
            while ( n < text.length() )
            {
                curline = curline + text.at(n);
                if ( text.at(n) == '\n' )
                {
                    s = s + curline ;
                    curline = "";
                }

                if (curline.length() > width)
                {
                    // Try to find last previous whitespace in curline
                    uint i = curline.length() - 1;
                    while ( i> 0 )
                    {
                        if ( curline.at(i) == ' ' )
                        {
                            s = s + curline.left(i) + '\n';
                            curline = curline.right( curline.length() - i - 1 );
                            break;
                        }
                        i--;
                        if ( i == 0 )
                        {
                            // Cannot break this line into smaller parts
                            s = s + curline;
                            curline = "";
                        }
                    }
                }
                n++;
            }
            s = s + curline;
        }

        // Indent lines
        rx.setPattern("^");
        s = s.replace (rx,indent);
        rx.setPattern("\n");
        s = s.replace (rx, "\n" + indent) + "\n";

        return s;
    }

    // Remove all <style...> ...</style>
    rx.setPattern("<style.*>.*</style>");
    s.replace (rx,"");

    // convert all "<br*>" to "\n"
    rx.setPattern ("<br.*>");
    s.replace (rx,"\n");

    // convert all "</p>" to "\n"
    rx.setPattern ("</p>");
    s.replace (rx,"\n");

    // remove all remaining tags
    rx.setPattern ("<.*>");
    s.replace (rx,"");

    // If string starts with \n now, remove it.
    // It would be wrong in an OOo export for example
    while ( s.at(0) == '\n' ) s.remove (0,1);

    // convert "&", "<" and ">"
    rx.setPattern ("&gt;");
    s.replace (rx,">");
    rx.setPattern ("&lt;");
    s.replace (rx,"<");
    rx.setPattern ("&amp;");
    s.replace (rx,"&");
    rx.setPattern ("&quot;");
    s.replace (rx,"\"");

    // Indent everything
    rx.setPattern ("^\n");
    s.replace (rx,indent);
    s = indent + s;   // Don't forget first line

/* FIXME-3  wrap text at width
    if (fonthint !="fixed")
    {
    }
*/
    return s;
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
