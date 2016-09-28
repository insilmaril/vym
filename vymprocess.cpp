#include "vymprocess.h"
#include <cstdlib>

#include <QMessageBox>
#include <QDebug>

extern bool debug;

/////////////////////////////////////////////////////////////////
// Process
/////////////////////////////////////////////////////////////////
VymProcess::VymProcess()
{
    connect( this, SIGNAL(readyReadStandardError()),
	     this, SLOT(readProcErrout()) );
    connect( this, SIGNAL(readyReadStandardOutput()),
	     this, SLOT(readProcStdout()) );
    clear();	     
}

VymProcess::~VymProcess()
{
}

void VymProcess::clear()
{
    errOut="";
    stdOut="";
}

void VymProcess::runScript(QString spath, QString fpath)
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
    /* TODO output for Process::runScript
    qDebug()<<readAllStandardOutput();
    qDebug()<<getStdout();
    qDebug()<<getErrout();
    addOutput ("\n");
    addOutput (getErrout());
    addOutput (getStdout());
    */
}

void VymProcess::readProcErrout()
{
    errOut+=readAllStandardError();
}

void VymProcess::readProcStdout()
{
    stdOut+=readAllStandardOutput();
}

QString VymProcess::getErrout()
{
    return errOut;
}

QString VymProcess::getStdout()
{
    return stdOut;
}
