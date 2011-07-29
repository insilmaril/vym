#include "xml-base.h"

#include <QMessageBox>
#include <QColor>
#include <QTextStream>
#include <iostream>

#include "misc.h"
#include "settings.h"
#include "linkablemapobj.h"


parseBaseHandler::parseBaseHandler() {}

parseBaseHandler::~parseBaseHandler() {}

QString parseBaseHandler::errorProtocol() { return errorProt; }


QString parseBaseHandler::parseHREF(QString href)
{
    QString type=href.section(":",0,0);
    QString path=href.section(":",1,1);
    if (!tmpDir.endsWith("/"))
	return tmpDir + "/" + path;
    else    
	return tmpDir + path;
}

bool parseBaseHandler::fatalError( const QXmlParseException& exception ) 
{
    errorProt += QString( "Fatal parsing error: %1 in line %2, column %3\n")
    .arg( exception.message() )
    .arg( exception.lineNumber() )
    .arg( exception.columnNumber() );
    // Try to read the bogus line
    errorProt+=QString("File is: %1\n").arg(inputFile);
    QString s;
    if (loadStringFromDisk (inputFile,s))
    {
	QStringList sl=s.split ("\n");
	int i=1;
	QStringList::Iterator it = sl.begin();
	while (i<exception.lineNumber())
	{
	    it++;
	    i++;
	}
	s=*it;
	s.insert (exception.columnNumber()-1,"<ERROR>");
	errorProt+=s;
    }
    return QXmlDefaultHandler::fatalError( exception );
}

void parseBaseHandler::setModel (VymModel *m)
{
    model=m;
}

void parseBaseHandler::setTmpDir (QString tp)
{
    tmpDir=tp;
}

void parseBaseHandler::setInputFile (QString f)
{
    inputFile=f;
}

void parseBaseHandler::setLoadMode (const LoadMode &lm, int p)
{
    loadMode=lm;
    insertPos=p;
}

