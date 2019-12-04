#include "confluence-agent.h"

#include "branchitem.h"
#include "mainwindow.h"
#include "vymmodel.h"


extern Main *mainWindow;
extern QDir vymBaseDir;
extern bool debug;

ConfluenceAgent::ConfluenceAgent (VymModel *m)
{
    p = NULL;
    killTimer = NULL;

    modelID = m->getModelID();

    //qDebug()<<"Constr. ConfluenceAgent for "<<branchID;

    confluenceScript = vymBaseDir.path() + "/scripts/confluence.rb";

    p = new VymProcess;

    connect (p, SIGNAL (finished(int, QProcess::ExitStatus) ), 
	this, SLOT (processFinished(int, QProcess::ExitStatus) ));


    killTimer = new QTimer(this); 
    killTimer->setInterval(10000); 
    killTimer->setSingleShot(true); 

    QObject::connect(killTimer, SIGNAL(timeout()), this, SLOT(timeout()));
}

ConfluenceAgent::~ConfluenceAgent ()
{
    if (p) delete p;
    if (killTimer) delete killTimer;
}

void ConfluenceAgent::test()
{
    QStringList args;

    args << "-h";

    p->start (confluenceScript, args);

    if (!p->waitForStarted())
    {
	qWarning() << "ConfluenceAgent::test()  couldn't start " << confluenceScript;
	return; 
    }

    killTimer->start();
}

void ConfluenceAgent::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::NormalExit)
    {
	QString result = p->getStdout();
        qDebug() << "Result:\n" << result; // FIXME-0 process JSON

	QString err = p->getErrout();
	if (!err.isEmpty())
        {
	    qWarning() << "ConfluenceAgent Error: \n" << err;
        } //else 
            // FIXME-0 process received data
    } else	
	qWarning() << "ConfluenceAgent: Process finished with exitCode=" << exitCode;
    deleteLater();
}

void ConfluenceAgent::timeout()
{
    // FIXME-0 needed?  undoUpdateMessage();
}
    
void ConfluenceAgent::processData()
{
    // Find model from which we had been started
    VymModel *model = mainWindow->getModel (modelID);
    if (model)
    {
	// and find branch which triggered this mission
	BranchItem *missionBI = (BranchItem*)(model->findID (branchID));	    
    }
}

