#include "process.h"

#include <QMessageBox>
#include <QDebug>

extern bool debug;

/////////////////////////////////////////////////////////////////
// Process
/////////////////////////////////////////////////////////////////
Process::Process()
{
    connect( this, SIGNAL(readyReadStandardError()),
	     this, SLOT(readProcErrout()) );
    connect( this, SIGNAL(readyReadStandardOutput()),
	     this, SLOT(readProcStdout()) );
    clear();	     
}

Process::~Process()
{
}

void Process::clear()
{
    errOut="";
    stdOut="";
}

void Process::runScript(QString spath, QString fpath)
{
    spath.replace ("%f",fpath);
    QStringList args=spath.split (' ');
    spath=args.takeFirst();
	
    if (debug)
	qDebug()<<"Process::runScript : " + spath+" "+args.join(" ");	

    start (spath,args);
    if (!waitForStarted() )
    {
	QMessageBox::critical( 0, tr( "Critical Error" ),
		       tr("Could not start %1").arg(spath) );
    } else
    {
	if (!waitForFinished())
	    QMessageBox::critical( 0, tr( "Critical Error" ),
	       tr("%1 didn't exit normally").arg(spath) +
	       getErrout() );
    //	else
    //	    if (exitStatus()>0) showOutput=true;
	    
    }	
    /* FIXME-3	output for Process::runScript
    qDebug()<<readAllStandardOutput();
    qDebug()<<getStdout();
    qDebug()<<getErrout();
    addOutput ("\n");
    addOutput (getErrout());
    addOutput (getStdout());
    */
}

void Process::readProcErrout()
{
    errOut+=readAllStandardError();
}

void Process::readProcStdout()
{
    stdOut+=readAllStandardOutput();
}

QString Process::getErrout()
{
    return errOut;
}

QString Process::getStdout()
{
    return stdOut;
}
