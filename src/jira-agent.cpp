#include "jira-agent.h"

#include "branchitem.h"
#include "mainwindow.h"
#include "vymmodel.h"

#include <QHash>
#include <QMessageBox>
#include <QSslSocket>

extern Main *mainWindow;
extern QDir vymBaseDir;
extern Settings settings;
extern QTextStream vout;
extern bool debug;

bool JiraAgent::available()
{
    if (!QSslSocket::supportsSsl())
        return false;
    if ( settings.value("/atlassian/jira/servers/size", 0).toInt() < 1)
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

    // Reset credentials, these are server specific beginning in 2.9.18
    authUsingPATInt = true;
    personalAccessTokenInt = QString();
    userNameInt = QString();
    passwordInt = QString();
    serverNameInt = QString();

    // Set API rest point. baseUrlInt later on depends on different JIRA system
    apiUrl = "/rest/api/2";

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
        modelID = bi->getModel()->getModelID();
        return true;
    }
}


bool JiraAgent::setTicket(const QString &id)
{
    // Find ID part in parameter:
    QRegularExpression re("(\\w+[-|\\s]\\d+)");
    QRegularExpressionMatch match = re.match(id);
    if (!match.hasMatch()) {
        qWarning() << "JiraAgent::setTicket invalid ID: " << id;
        abortJob = true;
        return false;
    }

    ticketID = match.captured(1);

    ticketID.replace(" ", "-");

    bool foundPattern = false;

    settings.beginGroup("/atlassian/jira");

    // Try to find baseUrl of server by looking through patterns in ticket IDs:
    int size = settings.beginReadArray("servers");
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        foreach (QString p, settings.value("pattern").toString().split(",")) {
            if (ticketID.contains(p)) {
                foundPattern = true;

                baseUrlInt = settings.value("baseUrl","-").toString();
                serverNameInt = settings.value("name","-").toString();

                // Read credentials for this server   
                authUsingPATInt = 
                    settings.value("authUsingPAT", true).toBool();
                if (authUsingPATInt)
                    personalAccessTokenInt =
                        settings.value("PAT", "undefined").toString();
                else {
                    userNameInt =
                        settings.value("username", "user_johnDoe").toString();
                    passwordInt = 
                        settings.value("password", "").toString();
                }
                break;
            }
        }
    }
    settings.endArray();
    settings.endGroup();

    return foundPattern;
}

QString JiraAgent::serverName()
{
    if (baseUrlInt.isEmpty())
        return QString();
    else
        return serverNameInt;
}

QString JiraAgent::url()
{
    return baseUrlInt + "/browse/" + ticketID;
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
                    jsobj["vymBranchId"] = QJsonValue(branchID);
                    jsobj["vymTicketUrl"] = QJsonValue(url());

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
    QUrl u = QUrl(baseUrlInt + apiUrl + "/issue/" + ticketID);

    QNetworkRequest request = QNetworkRequest(u);

    // Basic authentication in header
    QString headerData;
    if (authUsingPATInt)
        headerData = QString("Bearer %1").arg(personalAccessTokenInt);
    else {
        QString concatenated = userNameInt + ":" + passwordInt;
        QByteArray data = concatenated.toLocal8Bit().toBase64();
        headerData = "Basic " + data;
    }

    request.setRawHeader("Authorization", headerData.toLocal8Bit());

    if (debug)
        qDebug() << "JA::startGetTicketRequest: url = " + request.url().toString();

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
        vout << "reply: " << Qt::endl << r << Qt::endl;
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
