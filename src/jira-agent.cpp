#include "jira-agent.h"

#include "branchitem.h"
#include "mainwindow.h"
#include "vymmodel.h"

#include <QHash>
#include <QMessageBox>
#include <QSslSocket>

#include <iostream> // FIXME-5 for debugging...

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
    jobTypeInt = Undefined;
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

    queryInt.clear();

    // Set API rest point. baseUrlInt later on depends on different JIRA system
    apiUrl = "/rest/api/2";

}

bool JiraAgent::setJiraServer(int n)
{
    bool foundServer = false;

    bool usePAT = settings.value("authUsingPAT", true).toBool();
    QString url = settings.value("baseUrl", "").toString();
    if (!url.isEmpty()) {
        baseUrlInt = url;
        serverNameInt = settings.value("name","-").toString();
        if (usePAT) {
            QString pat = settings.value("PAT", "").toString();
            if (!pat.isEmpty()) {
                // Use PAT
                authUsingPATInt = true;
                personalAccessTokenInt = pat;
                foundServer = true;
            }
        } else {
            // Looking for username and password
            QString user = settings.value("username", "").toString();
            if (!user.isEmpty()) {
                QString pass = settings.value("password", "").toString();
                if (!pass.isEmpty()) {
                    userNameInt = user;
                    passwordInt = pass;
                    foundServer = true;
                }
            }
        }
    }

    return foundServer;
}

void JiraAgent::setJobType(JobType jt)
{
    jobTypeInt = jt;
}

JiraAgent::JobType JiraAgent::jobType()
{
    return jobTypeInt;
}

bool JiraAgent::setBranch(BranchItem *bi)
{
    if (!bi) {
        abortJob = true;
        return false;
    } else {
        branchID = bi->getID();
        modelID = bi->getModel()->modelId();
        return true;
    }
}


QString JiraAgent::key()
{
    return keyInt;
}

bool JiraAgent::setTicket(const QString &text)
{
    bool foundTicket = false;

    bool searchBaseUrl = text.startsWith("https");

    QString ticketKey;
    // Find ticket key in text
    QRegularExpression re("(\\w+[-|\\s]\\d+)");
    QRegularExpressionMatch match = re.match(text);
    if (match.hasMatch()) {
        ticketKey = match.captured(1);
        ticketKey.replace(" ", "-");
    } else {
        qWarning() << "JiraAgent::jobTypeFromText failed for text=" << text;
        return false;
    }

    settings.beginGroup("/atlassian/jira");

    // Try to find server by looking through baseUrls or patterns
    int size = settings.beginReadArray("servers");
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        foreach (QString pattern, settings.value("pattern").toString().split(",")) {
            bool ok = ticketKey.contains(pattern);

            if (ok && setJiraServer(i)) {
                foundTicket = true;
                keyInt = ticketKey; // Might still be empty if looking for Url...
                i = size;
                break;
            }
        }
    }
    settings.endArray();
    settings.endGroup();

    if (foundTicket) 
        jobTypeInt = JiraAgent::GetTicketInfo;
    else
        jobTypeInt = JiraAgent::Undefined;

    return foundTicket;
}

bool JiraAgent::setQuery(const QString &s)
{
    queryInt = s;

    bool foundServer = false; // FIXME-3 For now try only first server for queries. Better: 
                              // Search for project = PATTERN and use resulting server

    settings.beginGroup("/atlassian/jira/servers/1");
    bool usePAT = settings.value("authUsingPAT", true).toBool();
    QString url = settings.value("baseUrl", "").toString();
    if (!url.isEmpty()) {
        baseUrlInt = url;
        qDebug() << "JA::setQuery  url=" <<url;
        if (usePAT) {
            QString pat = settings.value("PAT", "").toString();
            if (!pat.isEmpty()) {
                // Use PAT
                personalAccessTokenInt = pat;
                foundServer = true;
            }
        } else {
            // Looking for username and password
            QString user = settings.value("username", "").toString();
            if (!user.isEmpty()) {
                QString pass = settings.value("password", "").toString();
                if (!pass.isEmpty()) {
                    userNameInt = user;
                    passwordInt = pass;
                    foundServer = true;
                }
            }
        }
    }
    settings.endGroup();

    return foundServer;
}

QString JiraAgent::query()
{
    return queryInt;
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
    return baseUrlInt + "/browse/" + keyInt;
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

    //qDebug() << "JA::contJob " << jobTypeInt << " Step: " << jobStep << "keyInt: " << keyInt << " this=" << this;

    switch(jobTypeInt) {
        case GetTicketInfo:
            switch(jobStep) {
                case 1:
                    startGetTicketRequest();
                    break;
                case 2: {
                    QJsonDocument jsdoc = QJsonDocument (jsobj);

                    // Insert references to original branch
                    jsobj["vymBranchId"] = QJsonValue(branchID);
                    jsobj["vymJiraTicketUrl"] = QJsonValue(url());

                    emit (jiraTicketReady(QJsonObject(jsobj)));
                    finishJob();
                    }
                    break;
                default:
                    unknownStepWarning();
                    break;
            };
            break;
        case Query:
            switch(jobStep) {
                case 1:
                    // if (!requestedURL.toString().startsWith("http"))
                    //    requestedURL.setPath("https://" + requestedURL.path());
                    startQueryRequest();
                    break;
                case 2: {
                    QJsonDocument jsdoc = QJsonDocument (jsobj);

                    // Insert references to original branch and Jira server
                    jsobj["vymBranchId"] = QJsonValue(branchID);
                    jsobj["vymJiraServer"] = baseUrlInt;
                    jsobj["vymJiraLastQuery"] = queryInt;

                    emit (jiraQueryReady(QJsonObject(jsobj)));
                    finishJob();
                    }
                    break;
                default:
                    unknownStepWarning();
                    break;
            }
            break;
        default:
            qWarning() << "JiraAgent::continueJob   unknown jobType " << jobTypeInt;
    }
}

void JiraAgent::finishJob()
{
    deleteLater();
}

void JiraAgent::unknownStepWarning()
{
    qWarning() << "JA::contJob  unknow step in jobType = " 
        << jobTypeInt 
        << "jobStep = " << jobStep;
}

void JiraAgent::startGetTicketRequest()
{
    QUrl u = QUrl(baseUrlInt + apiUrl + "/issue/" + keyInt);

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

    if (debug) {
        qDebug() << "JA::startGetTicketRequest: url = " + request.url().toString();
        qDebug() << "                  authUsingPAT = " << authUsingPATInt;
    }

    killTimer->start();

    connect(networkManager, &QNetworkAccessManager::finished,
        this, &JiraAgent::ticketReceived);

    networkManager->get(request);
}

void JiraAgent::ticketReceived(QNetworkReply *reply)
{
    //qDebug() << "JA::ticketReceived";

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

    // vout << jsdoc.toJson(QJsonDocument::Indented) << Qt::endl;

    continueJob();
}

void JiraAgent::startQueryRequest()
{
    //QUrl u = QUrl(baseUrlInt + apiUrl + "/search/id");
    QUrl u = QUrl(baseUrlInt + "/rest/api/2" + "/search");

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

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    //queryInt = "project = OKRTEST";
    QString s = QString(
    "{" 
      "\"jql\": \"%1\", "
//      "\"expand\": [ \"names\", \"schema\", \"operations\" ],"
      "\"fields\": ["
                                            // For now use fixed server
         "\"assignee\","
         "\"components\","
         "\"description\","
         "\"fixVersions\","
         "\"issuetype\","
         "\"issuelinks\","
         "\"parent\","
         "\"resolution\","
         "\"reporter\","
         "\"status\","
         "\"subtasks\","
         "\"summary\""
      "],"
      "\"maxResults\": 200,"
      "\"startAt\": 0"
    "}").arg(queryInt);

    QJsonDocument doc = QJsonDocument::fromJson(s.toUtf8());
    QByteArray data = doc.toJson();

    if (debug) {
        qDebug() << "JA::startQueryRequest: url = " + request.url().toString();
        qDebug() << "s=" << s;
        std::cout << doc.toJson(QJsonDocument::Indented).toStdString() << Qt::endl;
    }

    killTimer->start();

    connect(networkManager, &QNetworkAccessManager::finished,
        this, &JiraAgent::queryFinished);

    qDebug() << "JA::starting query";
    qDebug() << "  s=" << s;
    networkManager->post(request, data);
}

void JiraAgent::queryFinished(QNetworkReply *reply)
{
    qDebug() << "JA::queryFinished";

    killTimer->stop();

    networkManager->disconnect();
    reply->deleteLater();

    QByteArray fullReply = reply->readAll();

    if (reply->error()) {
        if (reply->error() == QNetworkReply::AuthenticationRequiredError)
            QMessageBox::warning(
                nullptr, tr("Warning"),
                tr("Authentication problem when contacting JIRA"));

        qWarning() << "JiraAgent::queryFinished reply error";

        qWarning() << "        Error: " << reply->error();
        qWarning() << "  Errorstring: " <<  reply->errorString();
        qDebug() << "    Request Url: " << reply->url() ;
        qDebug() << "      Operation: " << reply->operation() ;

        qDebug() << "      readAll: ";
        QJsonDocument jsdoc;
        jsdoc = QJsonDocument::fromJson(fullReply);
        QString fullReplyFormatted = QString(jsdoc.toJson(QJsonDocument::Indented));
        vout << fullReplyFormatted;

        finishJob();
        return;
    }

    QJsonDocument jsdoc;
    jsdoc = QJsonDocument::fromJson(fullReply);
    jsobj = jsdoc.object();

    continueJob();
}

void JiraAgent::timeout() 
{
    qWarning() << "JiraAgent timeout!!   jobType = " << jobTypeInt;
    deleteLater();
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
