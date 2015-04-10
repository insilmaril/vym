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
    return; //FIXME-000000000000000000
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
        textmode == other.textmode
        // ignore for now: filenamehint = other.filenamehint;
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
}

void VymText::clear()
{
    text = "";
    fonthint = "undef";
    filenamehint = "";
    textmode = AutoText;
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

QString VymText::saveToDir ()
{
    QString n = text;

    /* if (textmode == RichText) // FIXME-0 should no longer be necessary with use of CDATA
    {
        // Remove the doctype, which will confuse parsing
        // with XmlReader in Qt >= 4.4
        QRegExp rx("<!DOCTYPE.*>");
        rx.setMinimal(true);
        n.replace (rx,"");

        // QTextEdit may generate fontnames with unquoted &, like
        // in "Lucida B&H". This is invalid in XML and thus would crash
        // the XML parser

        // More invalid XML is generated with bullet lists:
        // There are 2 <style> tags in one <li>, so we merge them here
        int pos=0;
        bool inbracket=false;
        int begin_bracket=0;
        bool inquot=false;

        while (pos<n.length())
        {
            if (n.mid(pos,1)=="<")
            {
                inbracket=true;
                begin_bracket=pos;
            }
            if (n.mid(pos,1)==">")
            {
                inbracket=false;
                QString s=n.mid(begin_bracket,pos-begin_bracket+1);
                int sl=s.length();
                if (s.count("style=\"")>1)
                {
                    rx.setPattern ("style=\\s*\"(.*)\"\\s*style=\\s*\"(.*)\"");
                    s.replace(rx,"style=\"\\1 \\2\"");
                    n.replace (begin_bracket,sl,s);
                    pos=pos-(sl-s.length());
                }
            }
            if (n.mid(pos,1)=="\"" && inbracket)
            {
                if (!inquot)
                    inquot=true;
                else
                    inquot=false;
            }
            if (n.mid(pos,1)=="&" && inquot)
            {
                // Now we are inside  <  "  "  >
                n.replace(pos,1,"&amp;");
                pos=pos+3;
            }
            pos++;
        }
    }
    */
    return "<![CDATA[" + n + "]]>";
}
