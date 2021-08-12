#include "jira-agent.h"

#include "branchitem.h"
#include "mainwindow.h"
#include "vymmodel.h"

#include <QHash>

extern Main *mainWindow;
extern QDir vymBaseDir;
extern bool debug;

JiraAgent::JiraAgent(BranchItem *bi, const QString &u)
{
    // qDebug()<<"Constr. JiraAgent for "<<branchID;

    p = NULL;
    killTimer = NULL;

    if (!bi) {
        qWarning("Const JiraAgent: bi == NULL");
        delete (this);
        return;
    }
    branchID = bi->getID();
    VymModel *model = bi->getModel();
    modelID = model->getModelID();

    QString ticketID;

    url = u;

    QStringList args;

    if (url.contains("/browse/") || url.contains("servicedesk")) {
        // Extract ID from URL first:

        missionType = SingleTicket;
        ticketID = url.section('/', -1, -1);
        if (ticketID.isEmpty()) {
            qWarning() << "JiraAgent: No ticketID found in: " << url;
            delete (this);
            return;
        }
        args << ticketID;
    }
    else if (u.contains("fixme-filter")) // FIXME-4 not supported yet for jira
    {
        missionType = Query;
        args << "--query";
        args << url;
    }
    else {
        // Try to pass string or ID directly
        if (url.length() > 15) {
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

    connect(p, SIGNAL(finished(int, QProcess::ExitStatus)), this,
            SLOT(processFinished(int, QProcess::ExitStatus)));

    if (debug)
        qDebug() << "JiraAgent:  " << ticketScript << "  args: " << args;

    p->start(ticketScript, args);
    if (!p->waitForStarted()) {
        qWarning() << "JiraAgent::getJiraData couldn't start " << ticketScript;
        return;
    }

    // Visual hint that we are doing something  // FIXME-4 show spinner instead?
    if (missionType == SingleTicket) {
        oldHeading = bi->getHeading();
        model->setHeadingPlainText("Updating: " + bi->getHeadingPlain(),
                                   bi); // FIXME-4 translation needed?
    }

    killTimer = new QTimer(this);
    killTimer->setInterval(10000);
    killTimer->setSingleShot(true);

    QObject::connect(killTimer, SIGNAL(timeout()), this, SLOT(timeout()));
    killTimer->start();
}

JiraAgent::~JiraAgent()
{
    if (p)
        delete p;
    if (killTimer)
        delete killTimer;
}

void JiraAgent::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::NormalExit) {
        result = p->getStdout().split("\n");
        QString err = p->getErrout();
        if (!err.isEmpty()) {
            qWarning() << "JiraAgent Error: \n" << err;
            undoUpdateMessage();
        }
        else
            processJiraData();
    }
    else
        qWarning() << "JiraAgent: Process finished with exitCode=" << exitCode;
    deleteLater();
}

void JiraAgent::timeout() { undoUpdateMessage(); }

void JiraAgent::processJiraData()
{
    // Find model from which we had been started
    VymModel *model = mainWindow->getModel(modelID);
    if (model) {
        // and find branch which triggered this mission
        BranchItem *missionBI = (BranchItem *)(model->findID(branchID));
        if (missionBI) {
            // Here we go...

            QRegExp re("(.*):(\\S*):\"(.*)\"");
            re.setMinimal(false);
            ticket_desc.clear();
            ticket_prio.clear();
            ticket_status.clear();

            QStringList ticketIDs;
            foreach (QString line, result) {
                if (debug)
                    qDebug() << "JiraAgent::processJiraData  line=" << line;
                if (re.indexIn(line) != -1) {
                    if (re.cap(2) == "short_desc") {
                        ticketIDs.append(re.cap(1));
                        ticket_desc[re.cap(1)] =
                            re.cap(3).replace("\\\"", "\"");
                    }
                    else if (re.cap(2) == "type") {
                        ticket_type[re.cap(1)] =
                            re.cap(3).replace("\\\"", "\"");
                    }
                    else if (re.cap(2) == "priority") {
                        ticket_prio[re.cap(1)] =
                            re.cap(3).replace("\\\"", "\"");
                    }
                    else if (re.cap(2) == "status") {
                        ticket_status[re.cap(1)] =
                            re.cap(3).replace("\\\"", "\"");
                    }
                    else if (re.cap(2) == "resolution") {
                        ticket_resolution[re.cap(1)] =
                            re.cap(3).replace("\\\"", "\"");
                    }
                    else if (re.cap(2) == "created") {
                        ticket_created[re.cap(1)] =
                            re.cap(3).replace("\\\"", "\"");
                    }
                    else if (re.cap(2) == "updated") {
                        ticket_updated[re.cap(1)] =
                            re.cap(3).replace("\\\"", "\"");
                    }
                    else if (re.cap(2) == "assignee") {
                        ticket_assignee[re.cap(1)] =
                            re.cap(3).replace("\\\"", "\"");
                    }
                    else if (re.cap(2) == "reporter") {
                        ticket_reporter[re.cap(1)] =
                            re.cap(3).replace("\\\"", "\"");
                    }
                    else if (re.cap(2) == "url") {
                        ticket_url[re.cap(1)] = re.cap(3).replace("\\\"", "\"");
                    }
                }
            }
            if (ticket_desc.count() <= 0) {
                qWarning() << "JiraAgent: Couldn't find data";
                undoUpdateMessage(missionBI);
            }
            else if (missionType == SingleTicket) {
                // Only single ticket changed
                QString t = ticketIDs.first();
                setModelJiraData(model, missionBI, t);
            }
            else {
                // Process results of query
                BranchItem *newbi;
                foreach (QString b, ticketIDs) {
                    // qDeticket ()<<" -> "<<b<<" "<<ticket_desc[b];
                    newbi = model->addNewBranch(missionBI);
                    newbi->setURL("https://" + b); // FIXME-4 no filters yet
                    if (!newbi)
                        qWarning() << "JiraAgent: Couldn't create new branch?!";
                    else
                        setModelJiraData(model, newbi, b);
                }
            }
        }
        else
            qWarning() << "JiraAgent: Found model, but not branch #"
                       << branchID;
    }
    else
        qWarning() << "JiraAgent: Couldn't find model #" << modelID;
}

void JiraAgent::setModelJiraData(VymModel *model, BranchItem *bi,
                                 const QString &ticketID)
{
    if (debug) {
        qDebug() << "JiraAgent::setModelJiraData for ticketID: " << ticketID;
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
    solvedStates << "Done";

    QString idName = ticketID;

    if (solvedStates.contains(ticket_resolution[ticketID])) {
        idName = "(" + idName + ")";
        model->colorSubtree(Qt::blue, bi);
    }

    model->setHeadingPlainText(idName + " - " + ticket_desc[ticketID], bi);

    // Save current selections  // FIXME-4 No multiselection yet (cleanup IDs vs
    // UUIDs in treeitem)
    QString oldSelection = model->getSelectString();

    // Try to find subbranch named "JIRA log"

    BranchItem *logBranch;
    bool logBranchAvailable = false;
    for (int n = 0; n < bi->branchCount(); n++) {
        logBranch = bi->getBranchNum(n);
        if (logBranch->getHeadingPlain() == "JIRA log") {
            model->select(logBranch);
            logBranchAvailable = true;
            break;
        }
    }
    if (!logBranchAvailable) {
        logBranch = model->addNewBranch(bi, -2);
        model->setHeadingPlainText("JIRA log", logBranch);
        model->select(logBranch);
    }

    BranchItem *timestampBranch = model->addTimestamp();
    BranchItem *infoBranch;

    model->select(timestampBranch);
    infoBranch = model->addNewBranch();
    if (infoBranch)
        model->setHeadingPlainText("Prio: " + ticket_prio[ticketID],
                                   infoBranch);

    infoBranch = model->addNewBranch();
    if (infoBranch)
        model->setHeadingPlainText("Type: " + ticket_type[ticketID],
                                   infoBranch);

    infoBranch = model->addNewBranch();
    if (infoBranch)
        model->setHeadingPlainText("Status: " + ticket_status[ticketID],
                                   infoBranch);

    infoBranch = model->addNewBranch();
    if (infoBranch)
        model->setHeadingPlainText("Resolution: " + ticket_resolution[ticketID],
                                   infoBranch);

    infoBranch = model->addNewBranch();
    if (infoBranch)
        model->setHeadingPlainText("Assignee: " + ticket_assignee[ticketID],
                                   infoBranch);

    infoBranch = model->addNewBranch();
    if (infoBranch)
        model->setHeadingPlainText("Created: " + ticket_created[ticketID],
                                   infoBranch);

    infoBranch = model->addNewBranch();
    if (infoBranch)
        model->setHeadingPlainText("Updated: " + ticket_updated[ticketID],
                                   infoBranch);

    if (bi->getURL().isEmpty()) {
        model->select(bi);
        model->setURL(ticket_url[ticketID]);
    }

    // Scroll log branch
    model->select(logBranch);
    logBranch = model->getSelectedBranch();

    if (logBranch && !logBranch->isScrolled()) {
        model->toggleScroll();
    }

    // Selected previous objects
    model->select(oldSelection);
}

void JiraAgent::undoUpdateMessage(BranchItem *bi)
{
    VymModel *model = mainWindow->getModel(modelID);
    if (model) {
        if (!bi)
            bi = (BranchItem *)(model->findID(branchID));
        if (bi) {
            model->setHeading(oldHeading, bi);
        }
        else
            qWarning()
                << "JiraAgent::undoUpdateMessage couldn't find branch item!";
    }
}
