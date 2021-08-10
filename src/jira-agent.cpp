#include "jira-agent.h"

#include "branchitem.h"
#include "mainwindow.h"
#include "vymmodel.h"

#include <QHash>

extern Main *mainWindow;
extern QDir vymBaseDir;
extern QString jiraPassword;
extern Settings settings;
extern bool debug;

JiraAgent::JiraAgent()
{
    qDebug ()<< "Constr. JiraAgent";

    init();

    /* FIXME-0 old stuff, remove
    ticketScript = vymBaseDir.path() + "/scripts/jigger";

    p = new VymProcess;

    connect(p, SIGNAL(finished(int, QProcess::ExitStatus)), this,
            SLOT(processFinished(int, QProcess::ExitStatus)));

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
    */
}

JiraAgent::~JiraAgent()
{
    qDebug() << "Destr. JiraAgent";

    if (killTimer)
        delete killTimer;
}

void JiraAgent::init()
{
    jobType = Undefined;
    jobStep = -1;
    abortJob = false;

    killTimer = nullptr;

    networkManager = new QNetworkAccessManager(this);

    modelID = 0;    // invalid ID

    killTimer = new QTimer(this);
    killTimer->setInterval(15000);
    killTimer->setSingleShot(true);

    QObject::connect(killTimer, SIGNAL(timeout()), this, SLOT(timeout()));

    // Read credentials    
    username =
        settings.value("/jira/username", "user_johnDoe").toString();
    password = jiraPassword;

    // Set API rest point. baseURL later on depends on different JIRA system
    apiURL = baseURL + "/rest/api/2";
}

void JiraAgent::setJobType(JobType jt)
{
    jobType = jt;
}

bool JiraAgent::setBranch(BranchItem *bi)
{
    if (!bi) {
        abortJob = true;
        return false;
    } else {
        branchID = bi->getID();
        VymModel *model = bi->getModel();
        modelID = model->getModelID();
        return true;
    }
}


bool JiraAgent::setTicket(const QString &id)
{
    // FIXME-0 Decide, which JIRA to use, based on ticketID

    // Find ID part in parameter:
    QRegExp re("(\\w+[-|\\s]\\d+)");
    if (re.indexIn(id) < 0) {
        qWarning() << "JiraAgent::setTicket invalid ID: " << id;
        abortJob = true;
        return false;
    }

    ticketID = re.cap(1);
    ticketID.replace(" ", "-");

    // FIXME-0 For now hardcoded JIRA server:
    baseURL = "https://jira.elektrobit.com";

    qDebug() << "JiraAgent::setTicket ticketID: " << ticketID << "baseURL: " << baseURL;

    settings.beginGroup("jira");
    qDebug() << settings.childKeys();

    int size = settings.beginReadArray("servers");
    qDebug() << "size" << size;
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        qDebug() << settings.value("baseURL","-").toString();
    }
    settings.endArray();

    settings.endGroup();
    //abortJob = true;
    return true;

    
    
    // FIXME-0 old code below
    /*
    if (id.contains("/browse/") || id.contains("servicedesk")) {
        // Extract ID from URL first:

        ticketID = id.section('/', -1, -1);
        if (ticketID.isEmpty()) {
            qWarning() << "JiraAgent: No ticketID found in: " << id;
            abortJob = true;
            return;
        }
        //args << ticketID;
    }
    else {
        // Try to pass string or ID directly
        if (id.length() > 35) {
            // For security limit length
            qWarning() << "JiraAgent: URL too long, aborting!";
            return;
        }

        //args << url;
        ticketID = id;
    }
    */
}

QString JiraAgent::getURL()
{
}

void JiraAgent::startJob()
{
    if (jobStep > 0) {
        unknownStepWarning();
        finishJob();
    } else {
        jobStep = 0;
        continueJob();
    }
}

void JiraAgent::continueJob()
{
    if (abortJob) {
        finishJob();
        return;
    }

    jobStep++;

    VymModel *model;

    qDebug() << "JA::contJob " << jobType << " Step: " << jobStep << "TicketID: " << ticketID;

    switch(jobType) {
        case GetTicketInfo:
            switch(jobStep) {
                case 1:
                    // if (!requestedURL.toString().startsWith("http"))
                    //    requestedURL.setPath("https://" + requestedURL.path());
                    startGetTicketRequest();
                    break;
                case 2: {
                    QJsonDocument jsdoc = QJsonDocument (jsobj);
                    emit (jiraTicketReady(QJsonObject(jsobj)));
                    finishJob();
                    }
                    break;
                default:
                    unknownStepWarning();
                    break;
            };
            break;
        default:
            qWarning() << "JiraAgent::continueJob   unknown jobType " << jobType;
    }
}

void JiraAgent::finishJob()
{
    deleteLater();
}

void JiraAgent::unknownStepWarning()
{
    qWarning() << "JA::contJob  unknow step in jobType = " 
        << jobType 
        << "jobStep = " << jobStep;
}

void JiraAgent::startGetTicketRequest()
{
    qDebug() << "JA::startGetTicketRequest " << ticketID;

   //FIXME-0  QUrl url = requestedURL;
    QUrl url = QUrl(baseURL + apiURL + "/issue/" + ticketID);
    qDebug() << "  url=" << url;
    httpRequestAborted = false;

    QNetworkRequest request = QNetworkRequest(url);

    // Basic authentication in header
    QString concatenated = username + ":" + password;
    QByteArray data = concatenated.toLocal8Bit().toBase64();
    QString headerData = "Basic " + data;
    request.setRawHeader("Authorization", headerData.toLocal8Bit());

    if (debug)
    {
        qDebug() << "JA::startGetTicketRequest: url = " + request.url().toString();
    }

    killTimer->start();

    connect(networkManager, &QNetworkAccessManager::finished,
        this, &JiraAgent::ticketReceived);

    networkManager->get(request);
}

void JiraAgent::ticketReceived(QNetworkReply *reply)
{
    qDebug() << "JA::ticketReceived";

    killTimer->stop();

    networkManager->disconnect();

    QString r = reply->readAll();

    if (httpRequestAborted) {
        qWarning() << "JiraAgent::ticketReveived aborted";
        finishJob();
        return;
    }

    if (reply->error()) {
        qWarning() << "JiraAgent::ticketRReveived reply error";
        qWarning() << "Error: " << reply->error();
        qWarning() << "reply: " << r;
        finishJob();
        return;
    }

    QJsonDocument jsdoc;
    jsdoc = QJsonDocument::fromJson(r.toUtf8());
    jsobj = jsdoc.object();
    continueJob();
}

void JiraAgent::timeout() { undoUpdateMessage(); }

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

#ifndef QT_NO_SSL
void JiraAgent::sslErrors(QNetworkReply *reply, const QList<QSslError> &errors)
{
    QString errorString;
    foreach (const QSslError &error, errors) {
        if (!errorString.isEmpty())
            errorString += '\n';
        errorString += error.errorString();
    }

    reply->ignoreSslErrors();
    qWarning() << "JiraAgent: One or more SSL errors has occurred: " << errorString;
    qWarning() << "Errors ignored.";
}
#endif
