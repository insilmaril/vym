#include "confluence-agent.h"

#include "branchitem.h"
#include "mainwindow.h"
#include "misc.h"
#include "vymmodel.h"


extern Main *mainWindow;
extern QDir vymBaseDir;
extern bool debug;

ConfluenceAgent::ConfluenceAgent (VymModel *m)
{
    killTimer = NULL;

    modelID = m->getModelID();

    //qDebug() << "Constr. ConfluenceAgent for " << branchID;

    confluenceScript = vymBaseDir.path() + "/scripts/confluence.rb";

    killTimer = new QTimer(this); 
    killTimer->setInterval(10000); 
    killTimer->setSingleShot(true); 

    vymProcess = NULL;  // Only one process may be active at any time in this agent

    QObject::connect(killTimer, SIGNAL(timeout()), this, SLOT(timeout()));

    succ = false;
}

ConfluenceAgent::~ConfluenceAgent ()
{
    if (killTimer) delete killTimer;
}

void ConfluenceAgent::test()
{
    QStringList args;

    args << "-h";

    qWarning() << "ConfluenceAgent::test() called";

    vymProcess->start (confluenceScript, args);

    if (!vymProcess->waitForStarted())
    {
	qWarning() << "ConfluenceAgent::test()  couldn't start " << confluenceScript;
	return; 
    }

    killTimer->start();
}

bool ConfluenceAgent::getPageDetails(const QString &url)
{
    QStringList args;

    args << "-d";
    args << url;

    vymProcess = new VymProcess;

    connect (vymProcess, SIGNAL (finished(int, QProcess::ExitStatus) ), 
	this, SLOT (pageDetailsReceived(int, QProcess::ExitStatus) ));

    vymProcess->start (confluenceScript, args);

    if (!vymProcess->waitForStarted())
    {
	qWarning() << "ConfluenceAgent::test()  couldn't start " << confluenceScript;
	return false; 
    } 

    return true;
}

bool ConfluenceAgent::uploadContent(const QString &url, const QString &title, const QString &fpath, const bool &newPage)
{
    QStringList args;

    if (newPage)
        args << "-c";
    else
        args << "-u";
    args << url;
    args << "-f";
    args << fpath;
    args << "-t";
    args << title;

    qDebug() << "Calling confluence.rb:  " << args.join(" ");   // FIXME-0
    vymProcess = new VymProcess;

    connect (vymProcess, SIGNAL (finished(int, QProcess::ExitStatus) ), 
	this, SLOT (pageDetailsReceived(int, QProcess::ExitStatus) ));

    vymProcess->start (confluenceScript, args);

    if (!vymProcess->waitForStarted())
    {
	qWarning() << "ConfluenceAgent::test()  couldn't start " << confluenceScript;
	return false; 
    } 

    return true;
}

bool ConfluenceAgent::updatePage(const QString &url, const QString &title, const QString &fpath)
{
    return uploadContent (url, title, fpath, false);
}

bool ConfluenceAgent::createPage(const QString &url, const QString &title, const QString &fpath)
{
    return uploadContent (url, title, fpath, true);
}

void ConfluenceAgent::waitForResult()   
{
    if (!vymProcess) 
    {
        qWarning() << "ConfluenceAgent: No running vymProces";
        return;
    }
    if (!vymProcess->waitForFinished( 10000 ) )
    {
        qWarning() << "ConfluenceAgent: Timeout.";
        return;
    }
}

bool ConfluenceAgent::success()
{
    return succ;
}

QString ConfluenceAgent::getResult()
{
    return result;
}


bool ConfluenceAgent::pageDetailsReceived(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::NormalExit)
    {
	result = vymProcess->getStdout();
        qDebug() << result; // FIXME-0

	QString err = vymProcess->getErrout();
	if (!err.isEmpty())
        {
	    qWarning() << "ConfluenceAgent process error: \n" << err;
        } else 
        {
            if (!result.startsWith("Error")) succ = true;
        }
            
    } else	
	qWarning() << "ConfluenceAgent: Process finished with exitCode=" << exitCode;
    vymProcess = NULL;
}

void ConfluenceAgent::timeout()
{
    // FIXME-0 needed?  undoUpdateMessage();

    qWarning() << "ConfluenceAgent timeout!";
    delete (vymProcess);
    vymProcess = NULL;

}
    
