#include "jira-agent.h"

#include "branchitem.h"
#include "mainwindow.h"
#include "vymmodel.h"

#include <QHash>
#include <QMessageBox>

extern Main *mainWindow;
extern QDir vymBaseDir;
extern QString jiraPassword;
extern Settings settings;
extern QTextStream vout;
extern bool debug;

bool JiraAgent::available()
{
    if ( settings.value("/jira/username", "").toString().isEmpty())
        return false;
    if ( settings.value("/jira/servers/size", 0) < 1)
        return false;

    return true;
}

JiraAgent::JiraAgent()
{
    //qDebug ()<< "Constr. JiraAgent";

    init();
}

JiraAgent::~JiraAgent()
{
    //qDebug() << "Destr. JiraAgent";

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
    password = settings.value("/jira/password", jiraPassword).toString();

    // Set API rest point. baseURL later on depends on different JIRA system
    apiURL = "/rest/api/2";
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
    // Find ID part in parameter:
    QRegExp re("(\\w+[-|\\s]\\d+)");
    if (re.indexIn(id) < 0) {
        qWarning() << "JiraAgent::setTicket invalid ID: " << id;
        abortJob = true;
        return false;
    }

    ticketID = re.cap(1);
    ticketID.replace(" ", "-");

    settings.beginGroup("jira");

    int size = settings.beginReadArray("servers");
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        foreach (QString p, settings.value("pattern").toString().split(",")) {
            if (ticketID.contains(p)) {
                baseURL = settings.value("baseURL","-").toString();
                break;
            }
        }
    }
    settings.endArray();
    settings.endGroup();

    return true;
}

QString JiraAgent::getURL()
{
    return baseURL + "/browse/" + ticketID;
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

    // qDebug() << "JA::contJob " << jobType << " Step: " << jobStep << "TicketID: " << ticketID;

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

                    // Insert references to original branch and model
                    // DIXME-0 not needed jsobj["vymModelID"] = QString::number(modelID);
                    jsobj["vymBranchID"] = QJsonValue(branchID);
                    
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
    QUrl url = QUrl(baseURL + apiURL + "/issue/" + ticketID);

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
    // qDebug() << "JA::ticketReceived";

    killTimer->stop();

    networkManager->disconnect();

    QString r = reply->readAll();

    if (reply->error()) {
        if (reply->error() == QNetworkReply::AuthenticationRequiredError)
            QMessageBox::warning(
                nullptr, tr("Warning"),
                tr("Authentication problem when contacting JIRA"));

        qWarning() << "JiraAgent::ticketRReveived reply error";
        qWarning() << "Error: " << reply->error();
        vout << "reply: " << endl << r << endl;
        finishJob();
        return;
    }

    QJsonDocument jsdoc;
    jsdoc = QJsonDocument::fromJson(r.toUtf8());
    jsobj = jsdoc.object();
    continueJob();
}

void JiraAgent::timeout() 
{
    qWarning() << "JiraAgent timeout!!   jobType = " << jobType;
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
