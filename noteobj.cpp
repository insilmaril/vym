#include "noteobj.h"

#include <QRegExp>
#include <QDebug>
#include <QTextDocument>    // for mightBeRichText

/////////////////////////////////////////////////////////////////
// NoteObj
/////////////////////////////////////////////////////////////////

NoteObj::NoteObj()  //FIXME-3 transform this to general "Text" Baseclass for both plain and richtext
{
    clear();
}

NoteObj::NoteObj(const QString &s)
{
    clear();
    note=s;
    setNote (s);
}

void NoteObj::operator= (const NoteObj &other)
{
    copy (other);
}

void NoteObj::copy (NoteObj other)
{
    note=other.note;
    fonthint=other.fonthint;
    filenamehint=other.filenamehint;
}

void NoteObj::clear()
{
    note="";
    fonthint="undef";
    filenamehint="";
}

void NoteObj::setNote (const QString &s) 
{
    note=s;
}

void NoteObj::setNoteMasked (const QString &s) 
{
    note=dequotemeta(s);
}

QString NoteObj::getNote() const
{
    return note;
}

QString NoteObj::getNoteASCII()
{
    return getNoteASCII ("",80);
}

QString NoteObj::getNoteASCII(QString indent, const int &)  //FIXME-3 use width
{
    if (note.isEmpty() || !isRichText() ) return note;
    QString r=note;

    // Remove all <style...> ...</style>
    QRegExp rx ("<style.*>.*</style>");
    rx.setMinimal(true);
    r.replace (rx,"");

    // convert all "<br*>" to "\n"
    rx.setPattern ("<br.*>");
    r.replace (rx,"\n");

    // convert all "</p>" to "\n"
    rx.setPattern ("</p>");
    r.replace (rx,"\n");
    
    // remove all remaining tags 
    rx.setPattern ("<.*>");
    r.replace (rx,"");

    // If string starts with \n now, remove it.
    // It would be wrong in an OOo export for example
    while (r.at(0)=='\n') r.remove (0,1);
    
    // convert "&", "<" and ">"
    rx.setPattern ("&gt;");
    r.replace (rx,">");
    rx.setPattern ("&lt;");
    r.replace (rx,"<");
    rx.setPattern ("&amp;");
    r.replace (rx,"&");
    rx.setPattern ("&quot;");
    r.replace (rx,"\"");

    // Indent everything
    rx.setPattern ("^\n");
    r.replace (rx,indent);
    r=indent + r;   // Don't forget first line

/* FIXME-3  wrap text at width
    if (fonthint !="fixed")
    {
    }
*/  
    r=indent+"\n"+r+indent+"\n\n";
    return r;
}

QString NoteObj::getNoteOpenDoc()
{
    // Evil hack to transform QT Richtext into
    // something which can be used in OpenDoc format
    // 
    // TODO create clean XML transformation which also
    // considers fonts, colors, ...

    QString r=note;

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

bool NoteObj::isRichText()
{
    return Qt::mightBeRichText(note);
}


void NoteObj::setFontHint (const QString &s)
{
    // only for backward compatibility (pre 1.5 )
    fonthint=s;
}

QString NoteObj::getFontHint() const
{
    // only for backward compatibility (pre 1.5 )
    return fonthint;
}

void NoteObj::setFilenameHint (const QString &s)
{
    filenamehint=s;
}

QString NoteObj::getFilenameHint() const
{
    return filenamehint;
}

bool NoteObj::isEmpty ()
{
    return note.isEmpty();
}

QString NoteObj::saveToDir ()
{
    if (isRichText () )
    {
	QString n=note;

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

	return beginElement ("vymnote",attribut("fonthint",fonthint)) + 
	    n+ 
	    "\n" +
	    endElement ("vymnote"); 
    } 
    return valueElement("vymnote", quotemeta(note));
}

