#include "jiraagent.h"

#include "branchitem.h"
#include "mainwindow.h"
#include "vymmodel.h"

#include <QHash>

extern Main *mainWindow;
extern QDir vymBaseDir;
extern bool debug;


JiraAgent::JiraAgent (BranchItem *bi,const QString &u)
{
    if (!bi) 
    {
	qWarning ("Const JiraAgent: bi == NULL");
	delete (this);
	return;
    }
    branchID=bi->getID();
    VymModel *model = bi->getModel();
    modelID = model->getModelID();

    //qDebug()<<"Constr. JiraAgent for "<<branchID;

    url = u;

    QStringList args;

    if (url.contains("/browse/")) 
    {
        // Extract ID from URL first:

	missionType = SingleTicket;
	QRegExp rx("browse/(.*)$");
        rx.setMinimal(true);
	if (rx.indexIn(url) != -1)
	{
	    ticketID = rx.cap(1);
	    args << ticketID;
            qDebug() << "JiraAgent:  ticket id: "<< ticketID;   // TODO debugging
	} else
	{
	    qWarning() << "JiraAgent: No ticketID found in: " << url;
	    delete (this);
	    return;
	}

    } else if (u.contains("fixme-filter"))   // FIXME-0 not supported yet for jira
    {
	missionType = Query; //FIXME-0 query not supported yet by new bugger
	args << "--query";
	args << url;
    } else
    {
        // Try to pass string or ID directly
        if (url.length() > 15) 
        {
            // For security limit length    
            qWarning() << "JiraAgent: URL too long, aborting!";
            return;
        }

	missionType = SingleTicket;
        args << url;
        ticketID = url;
    }
	

    ticketScript = vymBaseDir.path() + "/scripts/jigger";

    p = new VymProcess;

    connect (p, SIGNAL (finished(int, QProcess::ExitStatus) ), 
	this, SLOT (processFinished(int, QProcess::ExitStatus) ));

    p->start (ticketScript, args);
    if (!p->waitForStarted())
    {
	qWarning() << "JiraAgent::getJiraData couldn't start " << ticketScript;
	return; 
        // FIXME-0 cleanup and delete JiraAgent missing
    }	 

    // Visual hint that we are doing something  // FIXME-4 show spinner instead?
    if (missionType == SingleTicket)
        model->setHeading ("Updating: " + bi->getHeadingPlain(), bi ); //FIXME-4 translation needed?
	
}

JiraAgent::~JiraAgent ()
{
    //qDebug()<<"Destr. JiraAgent for "<<branchID;
    delete p;
}

void JiraAgent::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::NormalExit)
    {
	result=p->getStdout().split("\n");
	QString err = p->getErrout();
	if (!err.isEmpty())
	    qWarning() << "JiraAgent Error: " << err;
	else 
	    processJiraData ();

    } else	
	qWarning() << "JiraAgent: Process finished with exitCode=" << exitCode;
    deleteLater();
}


void JiraAgent::processJiraData()
{
    // Find model from which we had been started
    VymModel *model = mainWindow->getModel (modelID);
    if (model)
    {
	// and find branch which triggered this mission
	BranchItem *missionBI = (BranchItem*)(model->findID (branchID));	    
	if (missionBI)
	{
	    // Here we go...

	    QRegExp re("(.*):(\\S*):\"(.*)\"");
	    re.setMinimal(false);
	    ticket_desc.clear();
	    ticket_prio.clear();
	    ticket_status.clear();

	    QStringList tickets; 
	    foreach (QString line,result)
	    {
		if (debug) qDebug() << "JiraAgent::processJiraData  line=" << line;
		if (re.indexIn(line) != -1) 
		{
		    if (re.cap(2) == "short_desc") 
		    {
			tickets.append(re.cap(1));
			ticket_desc[re.cap(1)] = re.cap(3).replace("\\\"","\"");
		    }	
		    else if (re.cap(2) == "priority") 
                    {
			ticket_prio[re.cap(1)] = re.cap(3).replace("\\\"","\"");
                    }
		    else if (re.cap(2) == "status") 
                    {
			ticket_status[re.cap(1)] = re.cap(3).replace("\\\"","\"");
                    }
		    else if (re.cap(2) == "resolution") 
                    {
			ticket_resolution[re.cap(1)] = re.cap(3).replace("\\\"","\"");
                    }
		    else if (re.cap(2) == "created") 
                    {
			ticket_created[re.cap(1)] = re.cap(3).replace("\\\"","\"");
                    }
		    else if (re.cap(2) == "updated") 
                    {
			ticket_updated[re.cap(1)] = re.cap(3).replace("\\\"","\"");
                    }
		    else if (re.cap(2) == "assignee") 
                    {
			ticket_assignee[re.cap(1)] = re.cap(3).replace("\\\"","\"");
                    }
		    else if (re.cap(2) == "reporter") 
                    {
			ticket_reporter[re.cap(1)] = re.cap(3).replace("\\\"","\"");
                    }
		    else if (re.cap(2) == "url") 
                    {
			ticket_url[re.cap(1)] = re.cap(3).replace("\\\"","\"");
                    }
		}
	    }
	    if (ticket_desc.count() <= 0 )
		qWarning() << "JiraAgent: Couldn't find data";
	    else if (missionType == SingleTicket)
	    {
		// Only single ticket changed
		QString b = tickets.first();
		setModelJiraData (model, missionBI, b);
	    } else
	    {
		// Process results of query
		BranchItem *newbi;
		foreach (QString b,tickets)
		{
		    //qDeticket ()<<" -> "<<b<<" "<<ticket_desc[b];
		    newbi = model->addNewBranch(missionBI);    
		    newbi->setURL ("https://bugzilla.novell.com/show_bug.cgi?id=" + b); // FIXME-0
		    if (!newbi)
			qWarning() << "JiraAgent: Couldn't create new branch?!";
		    else
			setModelJiraData (model, newbi,b);
		}
	    } 
	} else
	    qWarning () << "JiraAgent: Found model, but not branch #" << branchID;
    } else
	qWarning () << "JiraAgent: Couldn't find model #" << modelID;
}

void JiraAgent::setModelJiraData (VymModel *model, BranchItem *bi, const QString &bugID)
{
    if (debug)
    {
        qDebug() << "JiraAgent::setModelJiraData for " << bugID;
    }

    QStringList solvedStates;
    /*
    solvedStates << "Open";
    solvedStates << "To Do";
    solvedStates << "In Analysis";
    solvedStates << "Prepared";
    solvedStates << "Implemented";
    solvedStates << "In Overall Integration";	
    solvedStates << "Overall Integration Done";
    */
    solvedStates << "Verification Done";
    solvedStates << "Resolved";
    solvedStates << "Closed";

    // FIXME-1 check if model still exists (better use modelID)

    QString idName = ticketID;

    if (solvedStates.contains( ticket_status[ticketID] ) )
    {
        idName = "(" + idName + ")";
	model->colorSubtree (Qt::blue, bi);
    }

    model->setHeadingPlainText (idName + " - " + ticket_desc[ticketID], bi);

    // Save current selections  // FIXME-0 No multiselection yet (cleanup IDs vs UUIDs in treeitem)
    QString oldSelection = model->getSelectString();

    model->select(bi);

    BranchItem *timestampBranch = model->addTimestamp();
    BranchItem *infoBranch;

    model->select(timestampBranch);
    infoBranch = model->addNewBranch();
    if (infoBranch) model->setHeadingPlainText( "Prio: " + ticket_prio[ticketID], infoBranch);

    infoBranch = model->addNewBranch();
    if (infoBranch) model->setHeadingPlainText( "Status: " + ticket_status[ticketID], infoBranch);

    infoBranch = model->addNewBranch();
    if (infoBranch) model->setHeadingPlainText( "Resolution: " + ticket_resolution[ticketID], infoBranch);

    infoBranch = model->addNewBranch();
    if (infoBranch) model->setHeadingPlainText( "Assignee: " + ticket_assignee[ticketID], infoBranch);

    infoBranch = model->addNewBranch();
    if (infoBranch) model->setHeadingPlainText( "Created: " + ticket_created[ticketID], infoBranch);

    infoBranch = model->addNewBranch();
    if (infoBranch) model->setHeadingPlainText( "Updated: " + ticket_updated[ticketID], infoBranch);
    
    if (bi->getURL().isEmpty() )
    {
        model->select(bi);
        model->setURL(ticket_url[ticketID]); 
    }

    // Selected previous objects
    model->select(oldSelection);
}

