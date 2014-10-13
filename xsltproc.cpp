#include "xsltproc.h"

#include <iostream>
#include <QDebug>
#include <QMessageBox>

#include "vymprocess.h"


extern bool debug;

XSLTProc::XSLTProc ()
{
    xsltprocessor="xsltproc";
    showOutput=false;
    dia=new ShowTextDialog;
}

XSLTProc::~XSLTProc ()
{
    delete (dia);
}

void XSLTProc::addStringParam (const QString & k, const QString &v)
{
    stringParamKey.append (k);
    stringParamVal.append (v);
}

void XSLTProc::setOutputFile    (const QString &s)
{
    outputFile=s;
}

void XSLTProc::setXSLFile(const QString &s)
{
    xslFile=s;
}

void XSLTProc::setInputFile     (const QString &s)
{
    inputFile=s;
}

void XSLTProc::addOutput (const QString &s)
{
    dia->append (s);
}

void XSLTProc::process()
{
    ShowTextDialog dia;
    dia.useFixedFont (true);
    QStringList args;
    VymProcess *xsltProc=new VymProcess ();

    QStringList::Iterator itk;
    QStringList::Iterator itv=stringParamVal.begin();

    for ( itk = stringParamKey.begin(); itk != stringParamKey.end(); ++itk ) 
    {
	args << "--stringparam";
	args << *itk;
	args << *itv;
	++itv;
    }
    
    args << "--output";
    args << outputFile;
    args << xslFile;
    args << inputFile;
    QString com=xsltprocessor+" "+args.join(" "); 
    if (debug) qDebug() <<"xsltproc executing:\n"<<qPrintable(com);
    dia.append ("vym is executing: \n" + com );	
    xsltProc->start(xsltprocessor,args);
    if (!xsltProc->waitForStarted() )
    {
	QMessageBox::critical( 0, QObject::tr( "Critical Error" ),
		       QObject::tr("Could not start %1").arg(xsltprocessor) );
    } else
    {
	if (!xsltProc->waitForFinished())
	{
	    QMessageBox::critical( 0, QObject::tr( "Critical Error" ),
	       QObject::tr("%1 didn't exit normally").arg(xsltprocessor) +
	       xsltProc->getErrout() );
	    if (xsltProc->exitStatus()>0) showOutput=true;
	}      
	    
    }	
    dia.append ("\n");
    dia.append (xsltProc->getErrout());
    dia.append (xsltProc->getStdout());
    
    if (showOutput) dia.exec();
}

