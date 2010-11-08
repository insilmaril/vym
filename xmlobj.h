#ifndef XMLOBJ_H
#define XMLOBJ_H

class QString;

QString quotemeta(const QString&);  
QString dequotemeta(const QString&);	
QString quoteUmlaut(const QString&);	

/////////////////////////////////////////////////////////////////////////////
class XMLObj
{
public:
    XMLObj();
    virtual ~XMLObj();
    QString singleElement(QString,QString);	    // name,attr
    QString beginElement(QString,QString);	    // name,attr
    QString beginElement(QString);		    // name
    QString endElement  (QString);		    // name
    QString attribut    (QString,QString);	    // name, val
    QString valueElement(QString,QString);	    // name, val
    QString valueElement(QString,QString,QString);  // name, attr, val
    void incIndent();
    void decIndent();
    static int actindent;

protected:  
    QString indent();
    int indentwidth;
};

#endif
