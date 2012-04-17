#include "bugagent.h"

#include "branchitem.h"
#include "mainwindow.h"
#include "vymmodel.h"

#include <QHash>

extern Main *mainWindow;
extern QDir vymBaseDir;
extern bool debug;


BugAgent::BugAgent (BranchItem *bi,const QString &u)
{
    if (!bi) 
    {
	qWarning ("Const BugAgent: bi==NULL");
	delete (this);
	return;
    }
    branchID=bi->getID();
    VymModel *model=bi->getModel();
    modelID=model->getModelID();

    //qDebug()<<"Constr. BugAgent for "<<branchID;

    url=u;

    QStringList args;

    if (url.contains("show_bug"))
    {
	missionType=SingleBug;
	QRegExp rx("(\\d+)");
	if (rx.indexIn(url) !=-1)
	{
	    bugID=rx.cap(1);
	    args<<bugID;
	} else
	{
	    qDebug()<<"BugAgent: No bugID found in: "<<url;
	    delete (this);
	    return;
	}

    } else if (u.contains("buglist.cgi"))
    {
	missionType=Query; //FIXME-2 query not supported yet by new bugger
	args<<"--query";
	args<<url;
    } else
    {
	qDebug()<<"Unknown Bugzilla command:\n"<<url;
	delete (this);
	return;
    }
	

    bugScript=vymBaseDir.path()+"/scripts/bugger";

    p=new Process;

    connect (p, SIGNAL (finished(int,QProcess::ExitStatus) ), 
	this, SLOT (processFinished(int,QProcess::ExitStatus) ));

    p->start (bugScript,args);
    if (!p->waitForStarted())
    {
	qWarning()<<"BugAgent::getBugzillaData couldn't start "<<bugScript;
	return;
    }	

    // Visual hint that we are doing something
    if (missionType==SingleBug)
	model->setHeading ("Updating: "+bi->getHeadingPlain(),bi );//FIXME-4 translation needed?
	
}

BugAgent::~BugAgent ()
{
    //qDebug()<<"Destr. BugAgent for "<<branchID;
    delete p;
}

void BugAgent::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus==QProcess::NormalExit)
    {
	result=p->getStdout().split("\n");
	QString err=p->getErrout();
	if (!err.isEmpty())
	    qWarning() << "BugAgent Error: "<<err;
	else 
	    processBugzillaData ();

    } else	
	qWarning()<< "BugAgent: Process finished with exitCode="<<exitCode;
    deleteLater();
}


void BugAgent::processBugzillaData()
{
    // Find model from which we had been started
    VymModel *model=mainWindow->getModel (modelID);
    if (model)
    {
	// and find branch which triggered this mission
	BranchItem *missionBI=(BranchItem*)(model->findID (branchID));	    
	if (missionBI)
	{
	    // Here we go...

	    QRegExp re("(\\d*):(\\S*):\"(.*)\"");
	    re.setMinimal(false);
	    bug_desc.clear();
	    bug_prio.clear();
	    bug_sev.clear();
	    bug_deltats.clear();
	    bug_status.clear();
	    bug_whiteboard.clear();

	    QStringList bugs; 
	    foreach (QString line,result)
	    {
		if (debug) qDebug()<<"BugAgent::procBugData  line="<<line;
		if (re.indexIn(line) !=-1) 
		{
		    if (re.cap(2)=="short_desc") 
		    {
			bugs.append(re.cap(1));
			bug_desc[re.cap(1)]=re.cap(3).replace("\\\"","\"");
		    }	
		    else if (re.cap(2)=="priority") 
			bug_prio[re.cap(1)]=re.cap(3).left(2);
		    else if (re.cap(2)=="bug_severity") 
		    {
			if (re.cap(3)=="Critical")
			    bug_sev[re.cap(1)]="S1";
			else if (re.cap(3)=="Major")
			    bug_sev[re.cap(1)]="S2";
			else if (re.cap(3)=="Normal")
			    bug_sev[re.cap(1)]="S3";
			else if (re.cap(3)=="Minor")
			    bug_sev[re.cap(1)]="S4";
			else if (re.cap(3)=="Enhancement")
			    bug_sev[re.cap(1)]="S5";
			else 
			{
			    qWarning()<<"BugAgent: Bugzilla returned severity "<<re.cap(3);
			    bug_sev[re.cap(1)]=re.cap(3);
			}
		    }	
		    else if (re.cap(2)=="delta_ts") 
			bug_deltats[re.cap(1)]=re.cap(3);
		    else if (re.cap(2)=="bug_status") 
			bug_status[re.cap(1)]=re.cap(3);
		    else if (re.cap(2)=="status_whiteboard") 
			bug_whiteboard[re.cap(1)]=re.cap(3);
		}
	    }
	    if (bug_desc.count()<=0)
		qWarning()<<"BugAgent: Couldn't find data";
	    else if (missionType==SingleBug)
	    {
		// Only single bug changed
		QString b=bugs.first();
		setModelBugzillaData (model, missionBI,b);
	    } else
	    {
		// Process results of query
		BranchItem *newbi;
		foreach (QString b,bugs)
		{
		    //qDebug ()<<" -> "<<b<<" "<<bug_desc[b];
		    newbi=model->addNewBranch(0,missionBI);    
		    newbi->setURL ("https://bugzilla.novell.com/show_bug.cgi?id="+b);
		    if (!newbi)
			qWarning()<<"BugAgent: Couldn't create new branch?!";
		    else
			setModelBugzillaData (model, newbi,b);
		}
	    } 
	} else
	    qWarning ()<<"BugAgent: Found model, but not branch #"<<branchID;
    } else
	qWarning ()<<"BugAgent: Couldn't find model #"<<modelID;


}

void BugAgent::setModelBugzillaData (VymModel *model, BranchItem *bi, const QString &bugID)
{
    QString ps=bug_prio[bugID];
    if (bug_whiteboard[bugID].contains ("PNEW")) ps=ps+"/"+bug_sev[bugID];
    if (bug_status[bugID]=="CLOSED" 
	|| bug_status[bugID]=="VERIFIED"
	|| bug_status[bugID]=="RESOLVED")
    {
	model->setHeading ("("+ps+") - " + bugID + " - " + bug_desc[bugID],bi);
	model->colorSubtree (Qt::blue,bi);
    }else   
	model->setHeading (ps+ " - " + bugID + " - " + bug_desc[bugID],bi);
}

