#include "confluence-agent.h"

#include <QMessageBox>
#include <QSslSocket>

#include "branchitem.h"
#include "confluence-user.h"
#include "file.h"
#include "mainwindow.h"
#include "misc.h"
#include "vymmodel.h"

extern Main *mainWindow;
extern QDir vymBaseDir;
extern QString confluencePassword;
extern Settings settings;
extern bool debug;

bool ConfluenceAgent::available() 
{ 
    if (!QSslSocket::supportsSsl())
        return false;
    if ( settings.value("/atlassian/confluence/username", "").toString().isEmpty())
        return false;

    if ( settings.value("/atlassian/confluence/url", "").toString().isEmpty())
        return false;

    return true;
}

ConfluenceAgent::ConfluenceAgent() { 
    //qDebug() << "Constr. ConfluenceAgent";
    init(); 
}

ConfluenceAgent::ConfluenceAgent(BranchItem *bi)
{
    //qDebug() << "Constr. ConfluenceAgent selbi = " << bi;

    if (!bi) {
        qWarning("Const ConfluenceAgent: bi == nullptr");
        // This will leave the agent hanging around undeleted...
        return;
    }

    init();

    setBranch(bi);
}

ConfluenceAgent::~ConfluenceAgent()
{
    //qDebug() << "Destr ConfluenceAgent.";
    if (killTimer)
        delete killTimer;
}

void ConfluenceAgent::init()
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

    apiURL = baseURL + "/rest/api";
    baseURL = settings.value("/atlassian/confluence/url", "baseURL").toString();
    
    // Read credentials 
    username =
        settings.value("/atlassian/confluence/username", "user_johnDoe").toString();
    password = settings.value("/atlassian/confluence/password", confluencePassword).toString();

    if (password.isEmpty()) {
        // Set global password
        if (!mainWindow->settingsConfluence()) 
            abortJob = true;
    }
}

void ConfluenceAgent::setJobType(JobType jt)
{
    jobType = jt;
}

void ConfluenceAgent::setBranch(BranchItem *bi)
{
    if (!bi) {
        qWarning() << "ConfluenceAgent::setBranch  bi == nullptr";
        abortJob = true;
    } else {
        branchID = bi->getID();
        VymModel *model = bi->getModel();
        modelID = model->getModelID();
    }
}

void ConfluenceAgent::setModelID(uint id)
{
    modelID = id;
}

void ConfluenceAgent::setPageURL(const QString &u)
{
    pageURL = u;
}

void ConfluenceAgent::setNewPageName(const QString &t)
{
    newPageName = t;
}

void ConfluenceAgent::setUploadFilePath(const QString &fp)
{
    uploadFilePath = fp;
}

void ConfluenceAgent::startJob()
{
    if (jobStep > 0) {
        unknownStepWarning();
        finishJob();
    } else {
        jobStep = 0;
        continueJob();
    }
}

void ConfluenceAgent::continueJob()
{
    if (abortJob) {
        finishJob();
        return;
    }

    jobStep++;

    VymModel *model;

    //qDebug() << "CA::contJob " << jobType << " Step: " << jobStep;

    switch(jobType) {
        case CopyPagenameToHeading:
            switch(jobStep) {
                case 1:
                    startGetPageSourceRequest(pageURL);
                    break;
                case 2:
                    startGetPageDetailsRequest();
                    break;
                case 3:
                    model = mainWindow->getModel(modelID);
                    if (model) {
                        BranchItem *bi = (BranchItem *)(model->findID(branchID));

                        if (bi) {
                            QString h = spaceKey + ": " + jsobj["title"].toString();
                            model->setHeading(h, bi);
                        }
                        else
                            qWarning() << "CA::continueJob couldn't find branch "
                                       << branchID;
                    }
                    else
                        qWarning() << "CA::continueJob couldn't find model " << modelID;
                    break;
                default:
                    unknownStepWarning();
                    break;
            };
            break;

        case NewPage:
            switch(jobStep) {
                case 1:
                    if (pageURL.isEmpty()) {
                        qWarning() << "CA::contJob NewPage: pageURL is empty";
                        finishJob();
                        return;
                    }
                    if (newPageName.isEmpty()) {
                        qWarning() << "CA::contJob NewPage: newPageName is empty";
                        finishJob();
                        return;
                    }

                    mainWindow->statusMessage(
                        QString("Starting to create Confluence page %1").arg(pageURL));

                    // Check if parent page with url already exists and get pageID, spaceKey
                    startGetPageSourceRequest(pageURL);
                    break;
                case 2:
                    // Create new page with parent url
                    startCreatePageRequest();
                    break;
                case 3:
                    //qDebug() << "CA::finished  Created page with ID: " << jsobj["id"].toString();
                    mainWindow->statusMessage(
                        QString("Created Confluence page %1").arg(pageURL));
                    finishJob();
                    break;
                default:
                    unknownStepWarning();
            };

            break;

        case UpdatePage:
            switch(jobStep) {
                case 1:
                    if (pageURL.isEmpty()) {
                        qWarning() << "CA::contJob UpdatePage: pageURL is empty";
                        finishJob();
                        return;
                    }

                    mainWindow->statusMessage(
                        QString("Starting to update Confluence page %1").arg(pageURL));
                    //
                    // Check if page with url already exists and get pageID, spaceKey
                    startGetPageSourceRequest(pageURL);
                    break;
                case 2:
                    // Get title, which is required by Confluence to update a page
                    startGetPageDetailsRequest();
                    break;
                case 3:
                    // Update page with parent url
                    if (newPageName.isEmpty())
                            newPageName = jsobj["title"].toString();
                    startUpdatePageRequest();
                    break;
                case 4:
                    //qDebug() << "CA::finished  Updated page with ID: " << jsobj["id"].toString();
                    mainWindow->statusMessage(
                        QString("Updated Confluence page %1").arg(pageURL));
                    finishJob();
                    break;
                default:
                    unknownStepWarning();
            };

            break;
        case UserInfo:
            switch(jobStep) {
                case 1:
                    // qDebug() << "CA:: begin getting UserInfo";
                    startGetUserInfoRequest();
                    break;
                case 2: {
                        QJsonArray array = jsobj["results"].toArray();
                        QJsonObject userObj;
                        QJsonObject u;
                        ConfluenceUser user;
                        userList.clear();
                        for (int i = 0; i < array.size(); ++i) {
                            userObj = array[i].toObject();

                            u = userObj["user"].toObject();
                            user.setTitle( userObj["title"].toString());
                            user.setURL( "https://" + baseURL + "/"
                                    + "display/~" + u["username"].toString());
                            user.setUserKey( u["userKey"].toString());
                            user.setUserName( u["username"].toString());
                            user.setDisplayName( u["displayName"].toString());
                            userList << user;
                        }
                        emit (foundUsers(userList));
                        finishJob();
                    }
                    break;
                default:
                    unknownStepWarning();
            }
            break;
        default:
            qWarning() << "ConfluenceAgent::continueJob   unknown jobType " << jobType;
    }
}

void ConfluenceAgent::finishJob()
{
    deleteLater();
}

void ConfluenceAgent::unknownStepWarning()
{
    qWarning() << "CA::contJob  unknow step in jobType = " 
        << jobType 
        << "jobStep = " << jobStep;
}

void ConfluenceAgent::getUsers(const QString &usrQuery)
{
    userQuery = usrQuery;
    if (usrQuery.contains(QRegExp("\\W+"))) {
        qWarning() << "ConfluenceAgent::getUsers  Forbidden characters in " << usrQuery;
        return;
    }

    setJobType(UserInfo);
    startJob();
}

void ConfluenceAgent::startGetPageSourceRequest(QUrl requestedURL)
{
    //qDebug() << "CA::startGetPageSourceRequest " << requestedURL;
    if (!requestedURL.toString().startsWith("http"))
        requestedURL.setPath("https://" + requestedURL.path());

    QUrl url = requestedURL;

    QNetworkRequest request = QNetworkRequest(url);

    // Basic authentication in header
    QString concatenated = username + ":" + password;
    QByteArray data = concatenated.toLocal8Bit().toBase64();
    QString headerData = "Basic " + data;
    request.setRawHeader("Authorization", headerData.toLocal8Bit());

    if (debug)
    {
        qDebug() << "CA::startGetPageSourceRequest: url = " + request.url().toString();
    }

    killTimer->start();

    connect(networkManager, &QNetworkAccessManager::finished,
        this, &ConfluenceAgent::pageSourceReceived);

    networkManager->get(request);
}

void ConfluenceAgent::startGetPageDetailsRequest()
{
    if (debug) qDebug() << "CA::startGetPageDetailsRequest" << pageID;

    // Authentication in URL  (only SSL!)
    // maybe switch to token later:
    // https://developer.atlassian.com/cloud/confluence/basic-auth-for-rest-apis/
    QString concatenated = username + ":" + password;

    QString query = "https://" 
        + concatenated 
        + "@" + baseURL + apiURL 
        + "/content/" + pageID + "?expand=metadata.labels,version";

    QNetworkRequest request = QNetworkRequest(QUrl(query));

    connect(networkManager, &QNetworkAccessManager::finished,
        this, &ConfluenceAgent::pageDetailsReceived);

    killTimer->start();

    networkManager->get(request);
}

void ConfluenceAgent::startCreatePageRequest()
{
    // qDebug() << "CA::startCreatePageRequest";

    QString concatenated = username + ":" + password;

    QString url = "https://" + concatenated + "@" + baseURL + apiURL + "/content";

    QNetworkRequest request = QNetworkRequest(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject payload;
    payload["type"] = "page";
    payload["title"] = newPageName;

    // Build array with ID of parent page
    QJsonObject ancestorsID;
    ancestorsID["id"] = pageID;
    QJsonArray ancestorsArray;
    ancestorsArray.append(ancestorsID);
    payload["ancestors"] = ancestorsArray;

    // Build object with space key
    QJsonObject skey;
    skey["key"] = spaceKey;
    payload["space"] = skey;

    // Build body
    QString body;
    if (!loadStringFromDisk(uploadFilePath, body))
    {
        qWarning() << "ConfluenceAgent: Couldn't read file to upload:" << uploadFilePath;
        finishJob();
        return;
    }

    QJsonObject innerStorageObj
    {
        {"value", body},
        {"representation", "storage"}
    };
    QJsonObject outerStorageObj;
    outerStorageObj["storage"] = innerStorageObj;
    payload["body"] = outerStorageObj;

    QJsonDocument doc(payload);
    QByteArray data = doc.toJson();

    connect(networkManager, &QNetworkAccessManager::finished,
        this, &ConfluenceAgent::contentUploaded);

    killTimer->start();

    networkManager->post(request, data);
}

void ConfluenceAgent::startUpdatePageRequest()
{
    if (debug) qDebug() << "CA::startUpdatePageRequest";

    QString concatenated = username + ":" + password;

    QString url = "https://" + concatenated + "@" + baseURL + apiURL + "/content" + "/" + pageID;

    QNetworkRequest request = QNetworkRequest(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");

    QJsonObject payload;
    payload["id"] = pageID;
    payload["type"] = "page";
    payload["title"] = newPageName;

    // Build version object
    QJsonObject newVersionObj;
    QJsonObject oldVersionObj = jsobj["version"].toObject();

    newVersionObj["number"] = oldVersionObj["number"].toInt() + 1;
    payload["version"] = newVersionObj;

    // Build object with space key
    QJsonObject skey;
    skey["key"] = spaceKey;
    payload["space"] = skey;

    // Build body
    QString body;
    if (!loadStringFromDisk(uploadFilePath, body))
    {
        qWarning() << "ConfluenceAgent: Couldn't read file to upload:" << uploadFilePath;
        finishJob();
        return;
    }

    QJsonObject innerStorageObj
    {
        {"value", body},
        {"representation", "storage"}
    };
    QJsonObject outerStorageObj;
    outerStorageObj["storage"] = innerStorageObj;
    payload["body"] = outerStorageObj;

    QJsonDocument doc(payload);
    QByteArray data = doc.toJson();

    connect(networkManager, &QNetworkAccessManager::finished,
        this, &ConfluenceAgent::contentUploaded);

    killTimer->start();

    networkManager->put(request, data);
}

void ConfluenceAgent::startGetUserInfoRequest()
{
    if (debug) qDebug() << "CA::startGetInfoRequest for " << userQuery;

    QString concatenated = username + ":" + password;

    QString query = "https://" + baseURL + apiURL
    //    + concatenated 
    //    + "@" + apiURL 
        + "/search?cql=user.fullname~" + userQuery;
    // qDebug() << query;

    networkManager->disconnect();

    QNetworkRequest request = QNetworkRequest(QUrl(query));
//    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
//
    // Basic authentication in header
    QByteArray data = concatenated.toLocal8Bit().toBase64();
    QString headerData = "Basic " + data;
    request.setRawHeader("Authorization", headerData.toLocal8Bit());

    connect(networkManager, &QNetworkAccessManager::finished,
        this, &ConfluenceAgent::userInfoReceived);

    killTimer->start();

    networkManager->get(request);
}

bool ConfluenceAgent::requestSuccessful(QNetworkReply *reply, const QString &requestDesc)
{
    if (reply->error()) {
        QString stepDesc =QString("Step: Trying to %1").arg(requestDesc);
        QString readAll = reply->readAll();
        if (reply->error() == QNetworkReply::AuthenticationRequiredError)
            QMessageBox::warning(
                nullptr, tr("Warning") + ": " +
                tr("Authentication problem when contacting Confluence"), 
                stepDesc);
        else {
            QMessageBox::warning(
                nullptr, 
                tr("Warning") + ": " + QString("QNetworkReply error when trying to %1").arg(requestDesc),
                reply->error() + "\n\n" + readAll
                );
        }

        // Additionally print full error on console
        qWarning() << reply->error();
        qWarning() << reply->errorString();
        qWarning() << readAll;

        finishJob();
        return false;
    } else
        return true;
}

void ConfluenceAgent::pageSourceReceived(QNetworkReply *reply)
{
    if (debug) qDebug() << "CA::pageSourceReceived";

    killTimer->stop();

    networkManager->disconnect();

    if (!requestSuccessful(reply, "receive page source"))
        return;

    QString r = reply->readAll();

    // Find pageID
    QRegExp rx("\\sname=\"ajs-page-id\"\\scontent=\"(\\d*)\"");
    rx.setMinimal(true);

    if (rx.indexIn(r, 0) != -1) {
        pageID = rx.cap(1);
    }
    else {
        qWarning()
            << "ConfluenceAgent::pageSourceReveived Couldn't find page ID";
        //qWarning() << r;
        return;
    }

    // Find spaceKey 
    rx.setPattern("meta\\s*id=\"confluence-space-key\"\\s* "
                  "name=\"confluence-space-key\"\\s*content=\"(.*)\"");
    if (rx.indexIn(r, 0) != -1) {
        spaceKey = rx.cap(1);
    }
    else {
        qWarning() << "ConfluenceAgent::pageSourceReveived Couldn't find "
                      "space key in response";
        qWarning() << r;
        finishJob();
        return;
    }

    const QVariant redirectionTarget =
        reply->attribute(QNetworkRequest::RedirectionTargetAttribute);

    continueJob();
}

void ConfluenceAgent::pageDetailsReceived(QNetworkReply *reply)
{
    if (debug) qDebug() << "CA::pageDetailsReceived";

    killTimer->stop();

    networkManager->disconnect();

    if (!requestSuccessful(reply, "receive page details"))
        return;

    QJsonDocument jsdoc;
    jsdoc = QJsonDocument::fromJson(reply->readAll());

    jsobj = jsdoc.object();

    continueJob();
}

void ConfluenceAgent::contentUploaded(QNetworkReply *reply)
{
    if (debug) qDebug() << "CA::contentUploaded";

    killTimer->stop();

    networkManager->disconnect();

    if (!requestSuccessful(reply, "upload content"))
        return;

    QJsonDocument jsdoc;
    jsdoc = QJsonDocument::fromJson(reply->readAll());
    jsobj = jsdoc.object();
    continueJob();
}

void ConfluenceAgent::userInfoReceived(QNetworkReply *reply)
{
    if (debug) qDebug() << "CA::UserInfopageReceived";

    killTimer->stop();

    networkManager->disconnect();

    if (!requestSuccessful(reply, "receive user info"))
        return;

    QString r = reply->readAll();

    QJsonDocument jsdoc;
    jsdoc = QJsonDocument::fromJson(r.toUtf8());
    jsobj = jsdoc.object();
    continueJob();
}

void ConfluenceAgent::timeout()
{
    qWarning() << "ConfluenceAgent timeout!!   jobType = " << jobType;
}

#ifndef QT_NO_SSL
void ConfluenceAgent::sslErrors(QNetworkReply *reply, const QList<QSslError> &errors)
{
    QString errorString;
    foreach (const QSslError &error, errors) {
        if (!errorString.isEmpty())
            errorString += '\n';
        errorString += error.errorString();
    }

    reply->ignoreSslErrors();
    qWarning() << "ConfluenceAgent: One or more SSL errors has occurred: " << errorString;
    qWarning() << "Errors ignored.";
}
#endif
