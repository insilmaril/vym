#include "confluence-agent.h"

#include "branchitem.h"
#include "confluence-user.h"
#include "file.h"
#include "mainwindow.h"
#include "misc.h"
#include "vymmodel.h"

extern Main *mainWindow;
extern Settings settings;
extern QDir vymBaseDir;
extern bool debug;
extern QString confluencePassword;

////////////////////////////////////////////////////////////////////////////////
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

    confluenceScript = vymBaseDir.path() + "/scripts/confluence.rb";

    killTimer = new QTimer(this);
    killTimer->setInterval(15000);
    killTimer->setSingleShot(true);

    QObject::connect(killTimer, SIGNAL(timeout()), this, SLOT(timeout()));

    // Read credentials 
    username =
        settings.value("/confluence/username", "user_johnDoe").toString();
    password = confluencePassword;
    baseURL = settings.value("/confluence/url", "baseURL").toString();

    apiURL = baseURL + "/rest/api";
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

void ConfluenceAgent::setNewPageTitle(const QString &t)
{
    newPageTitle = t;
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
                    if (newPageTitle.isEmpty()) {
                        qWarning() << "CA::contJob NewPage: newPageTitle is empty";
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
                    if (newPageTitle.isEmpty())
                            newPageTitle = jsobj["title"].toString();
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

void ConfluenceAgent::timeout()
{
    qWarning() << "ConfluenceAgent timeout!!   jobType = " << jobType;
}

void ConfluenceAgent::startGetPageSourceRequest(QUrl requestedURL)
{
    //qDebug() << "CA::startGetPageSourceRequest " << requestedURL;
    if (!requestedURL.toString().startsWith("http"))
        requestedURL.setPath("https://" + requestedURL.path());

    QUrl url = requestedURL;
    httpRequestAborted = false;

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
    //qDebug() << "CA::startGetPageDetailsRequest" << pageID;

    httpRequestAborted = false;

    // Authentication in URL  (only SSL!)
    // maybe switch to token later:
    // https://developer.atlassian.com/cloud/confluence/basic-auth-for-rest-apis/
    QString concatenated = username + ":" + password;

    QString query = "https://" 
        + concatenated 
        + "@" + apiURL 
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

    httpRequestAborted = false;

    QString concatenated = username + ":" + password;

    QString url = "https://" + concatenated + "@" + apiURL + "/content";

    QNetworkRequest request = QNetworkRequest(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject payload;
    payload["type"] = "page";
    payload["title"] = newPageTitle;

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
    // qDebug() << "CA::startUpdatePageRequest";

    httpRequestAborted = false;

    QString concatenated = username + ":" + password;

    QString url = "https://" + concatenated + "@" + apiURL + "/content" + "/" + pageID;

    QNetworkRequest request = QNetworkRequest(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject payload;
    payload["id"] = pageID;
    payload["type"] = "page";
    payload["title"] = newPageTitle;

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
    // qDebug() << "CA::startGetInfoRequest for " << userQuery;

    httpRequestAborted = false;

    QString concatenated = username + ":" + password;

    QString query = "https://" + apiURL
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

void ConfluenceAgent::pageSourceReceived(QNetworkReply *reply)
{
    //qDebug() << "CA::pageSourceReceived";

    killTimer->stop();

    networkManager->disconnect();

    QString r = reply->readAll();

    if (httpRequestAborted) {
        qWarning() << "ConfluenceAgent::pageSourceReveived aborted";
        finishJob();
        return;
    }

    if (reply->error()) {
        qWarning() << "ConfluenceAgent::pageSourceReveived reply error";
        qWarning() << "Error: " << reply->error();
        finishJob();
        return;
    }

    // Find pageID
    QRegExp rx("\\sname=\"ajs-page-id\"\\scontent=\"(\\d*)\"");
    rx.setMinimal(true);

    if (rx.indexIn(r, 0) != -1) {
        pageID = rx.cap(1);
    }
    else {
        qWarning()
            << "ConfluenceAgent::pageSourceReveived Couldn't find page ID";
        qWarning() << r;
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
    // qDebug() << "CA::pageDetailsReceived";

    killTimer->stop();

    networkManager->disconnect();

    if (httpRequestAborted) {
        qWarning() << "ConfluenceAgent::pageDetailsReveived aborted error";
        finishJob();
        return;
    }

    if (reply->error()) {
        qWarning() << "ConfluenceAgent::pageDetailsReveived reply error";
        qWarning() << reply->error();
        finishJob();
        return;
    }

    QJsonDocument jsdoc;
    jsdoc = QJsonDocument::fromJson(reply->readAll());

    jsobj = jsdoc.object();

    continueJob();
}

void ConfluenceAgent::contentUploaded(QNetworkReply *reply)
{
    //qDebug() << "CA::contentUploaded";

    killTimer->stop();

    networkManager->disconnect();

    if (httpRequestAborted) {
        qWarning() << "ConfluenceAgent::contentUploaded aborted";
        finishJob();
        return;
    }

    if (reply->error()) {
        qWarning() << "ConfluenceAgent::contentUploaded reply error";
        qWarning() << reply->error();
        qWarning() << reply->errorString();
        qWarning() << reply->readAll();
        finishJob();
        return;
    }

    QJsonDocument jsdoc;
    jsdoc = QJsonDocument::fromJson(reply->readAll());
    jsobj = jsdoc.object();
    continueJob();
}

void ConfluenceAgent::userInfoReceived(QNetworkReply *reply)
{
    //qDebug() << "CA::UserInfopageReceived";

    killTimer->stop();

    networkManager->disconnect();

    QString r = reply->readAll();

    if (httpRequestAborted) {
        qWarning() << "ConfluenceAgent::UserInfoReveived aborted";
        finishJob();
        return;
    }

    if (reply->error()) {
        qWarning() << "ConfluenceAgent::UserInfoReveived reply error";
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
    qWarning() << "One or more SSL errors has occurred: " << errorString;
    qWarning() << "Errors ignored.";
}
#endif
