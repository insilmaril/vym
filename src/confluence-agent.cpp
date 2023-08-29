#include "confluence-agent.h"

#include <QMessageBox>
#include <QSslSocket>

#include <iostream> // FIXME-2 for debugging...

#include "branchitem.h"
#include "confluence-user.h"
#include "file.h"
#include "mainwindow.h"
#include "misc.h"
#include "vymmodel.h"
#include "warningdialog.h"

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
    //qDebug() << "Constr. ConfluenceAgent jobType=";
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
    // qDebug() << "Destr ConfluenceAgent." << jobType;
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
    
    // Attachments
    attachmentsAgent = nullptr;
    currentUploadAttachmentIndex = -1;

    // Read credentials 
    authUsingPAT = 
        settings.value("/atlassian/confluence/authUsingPAT", true).toBool();
    if (authUsingPAT)
        personalAccessToken =
            settings.value("/atlassian/confluence/PAT", "undefined").toString();
    else {
        username =
            settings.value("/atlassian/confluence/username", "user_johnDoe").toString();
        if (!confluencePassword.isEmpty())
            password = confluencePassword;
        else
            password = 
                settings.value("/atlassian/confluence/password", "").toString();
    }

    if (!authUsingPAT && password.isEmpty()) {
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

void ConfluenceAgent::setUploadPagePath(const QString &fp)
{
    uploadPagePath = fp;
}

void ConfluenceAgent::addUploadAttachmentPath(const QString &fp)
{
    uploadAttachmentPaths << fp;
}

void ConfluenceAgent::startJob()
{
    if (jobStep > 0) {
        unknownStepWarningFinishJob();
    } else {
        jobStep = 0;
        continueJob();
    }
}

void ConfluenceAgent::continueJob(int nextStep)
{
    if (abortJob) {
        finishJob();
        return;
    }

    if (nextStep < 0)
        jobStep++;
    else
        jobStep = nextStep;

    VymModel *model;

    // qDebug() << "CA::contJob " << jobType << " Step: " << jobStep;

    switch(jobType) {
        case CopyPagenameToHeading:
            if (jobStep == 1) {
                startGetPageSourceRequest(pageURL);
                return;
            }
            if (jobStep == 2) {
                startGetPageDetailsRequest();
                return;
            }
            if (jobStep == 3) {
                model = mainWindow->getModel(modelID);
                if (model) {
                    BranchItem *bi = (BranchItem *)(model->findID(branchID));

                    if (bi) {
                        QString h = spaceKey + ": " + pageObj["title"].toString();
                        model->setHeading(h, bi);
                    } else
                        qWarning() << "CA::continueJob couldn't find branch "
                                   << branchID;
                } else
                    qWarning() << "CA::continueJob couldn't find model " << modelID;
                finishJob();
                return;
            }
            unknownStepWarningFinishJob();
            return;

        case CreatePage:
            if (jobStep == 1) {
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
                return;
            }
            if (jobStep == 2) {
                // Create new page with parent url
                startCreatePageRequest();
                return;
            }
            if (jobStep == 3) {

                pageID = pageObj["id"].toString();

                // Upload attachments?
                if (uploadAttachmentPaths.count() > 0) {
                    attachmentsAgent = new ConfluenceAgent;
                    attachmentsAgent->setJobType(ConfluenceAgent::UploadAttachments);
                    attachmentsAgent->pageID = pageID;
                    attachmentsAgent->uploadAttachmentPaths = uploadAttachmentPaths;

                    connect(attachmentsAgent, &ConfluenceAgent::attachmentsSuccess,
                        this, &ConfluenceAgent::attachmentsUploadSuccess);
                    connect(attachmentsAgent, &ConfluenceAgent::attachmentsFailure,
                        this, &ConfluenceAgent::attachmentsUploadFailure);
                    attachmentsAgent->startJob();
                    return;
                } else
                    // Proceed to next step
                    jobStep = 4;
            }
            if (jobStep == 4) {
                //qDebug() << "CA::finished  Created page with ID: " << pageObj["id"].toString();
                mainWindow->statusMessage(
                    QString("Created Confluence page %1").arg(pageURL));
                finishJob();
                return;
            }
            unknownStepWarningFinishJob();
            return;

        case UpdatePage:
            if (jobStep == 1) {
                if (pageURL.isEmpty()) {
                    qWarning() << "CA::contJob UpdatePage: pageURL is empty";
                    finishJob();
                    return;
                }

                mainWindow->statusMessage(
                    QString("Starting to update Confluence page %1").arg(pageURL));

                // Check if page with url already exists and get pageID, spaceKey
                startGetPageSourceRequest(pageURL);
                return;
            }
            if (jobStep == 2) {
                // Get title, which is required by Confluence to update a page
                startGetPageDetailsRequest();
                return;
            }
            if (jobStep == 3) {
                // Upload attachments?
                if (uploadAttachmentPaths.count() > 0) {
                    attachmentsAgent = new ConfluenceAgent;
                    attachmentsAgent->setJobType(ConfluenceAgent::UploadAttachments);
                    attachmentsAgent->pageID = pageID;
                    attachmentsAgent->uploadAttachmentPaths = uploadAttachmentPaths;

                    connect(attachmentsAgent, &ConfluenceAgent::attachmentsSuccess,
                        this, &ConfluenceAgent::attachmentsUploadSuccess);
                    connect(attachmentsAgent, &ConfluenceAgent::attachmentsFailure,
                        this, &ConfluenceAgent::attachmentsUploadFailure);
                    attachmentsAgent->startJob();
                    return;
                }
            }
            if (jobStep == 4) {
                // Update page with parent url
                if (newPageName.isEmpty())
                        newPageName = pageObj["title"].toString();
                startUpdatePageRequest();
                return;
            }
            if (jobStep == 5) {
                //qDebug() << "CA::finished  Updated page with ID: " << pageObj["id"].toString();
                mainWindow->statusMessage(
                    QString("Updated Confluence page %1").arg(pageURL));
                finishJob();
                return;
            }
            unknownStepWarningFinishJob();
            return;

        case GetUserInfo:
            if (jobStep == 1) {
                // qDebug() << "CA:: begin getting UserInfo";
                startGetUserInfoRequest();
                return;
            }
            if (jobStep == 2) {
                QJsonArray array = pageObj["results"].toArray();
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
                return;
            }
            unknownStepWarningFinishJob();
            return;

        case UploadAttachments:
            if (jobStep == 1) {

                if (uploadAttachmentPaths.count() <= 0) {
                    qWarning() << "ConfluenceAgent: No attachments to upload!";
                    emit(attachmentsFailure());
                    finishJob();
                    return;
                }

                // Prepare to upload first attachment in list
                currentUploadAttachmentIndex = 0;

                // Try to get info for attachments
                startGetAttachmentsInfoRequest();
                return;
            }
            if (jobStep == 2) {
                // Entry point for looping over list of attachments to upload

                if (currentUploadAttachmentIndex >= uploadAttachmentPaths.count()) {
                    // All uploaded, let's finish uploading
                    emit(attachmentsSuccess());
                    finishJob();
                } else {
                    currentAttachmentPath = uploadAttachmentPaths.at(currentUploadAttachmentIndex);
                    currentAttachmentTitle = basename(currentAttachmentPath);

                    // Create attachment with image of map, if required
                    if (attachmentsTitles.count() == 0 || 
                        !attachmentsTitles.contains(currentAttachmentTitle)) {
                        // Create new attachment
                        startCreateAttachmentRequest();
                    } else {
                        // Update existing attachment
                        startUpdateAttachmentRequest();
                    }
                }
                return;
            }
            unknownStepWarningFinishJob();
            return;

        default:
            qWarning() << "ConfluenceAgent::continueJob   unknown jobType " << jobType;
    }
}

void ConfluenceAgent::finishJob()
{
    deleteLater();
}

void ConfluenceAgent::unknownStepWarningFinishJob()
{
    qWarning() << "CA::contJob  unknow step in jobType = " 
        << jobType 
        << "jobStep = " << jobStep;
    finishJob();
}

void ConfluenceAgent::getUsers(const QString &usrQuery)
{
    userQuery = usrQuery;
    if (usrQuery.contains(QRegExp("\\W+"))) {
        qWarning() << "ConfluenceAgent::getUsers  Forbidden characters in " << usrQuery;
        return;
    }

    setJobType(GetUserInfo);
    startJob();
}

QNetworkRequest ConfluenceAgent::createRequest(const QUrl &url)
{
    QNetworkRequest request = QNetworkRequest(url);

    QString headerData;
    if (authUsingPAT)
        headerData = QString("Bearer %1").arg(personalAccessToken);
    else {
        QString concatenated = username + ":" + password;
        QByteArray data = concatenated.toLocal8Bit().toBase64();
        headerData = "Basic " + data;
    }
    request.setRawHeader("Authorization", headerData.toLocal8Bit());

    return request;
}

void ConfluenceAgent::startGetPageSourceRequest(QUrl requestedURL)
{
    //qDebug() << "CA::startGetPageSourceRequest " << requestedURL;
    if (!requestedURL.toString().startsWith("http"))
        requestedURL.setPath("https://" + requestedURL.path());

    QUrl url = requestedURL;

    QNetworkRequest request = createRequest(url);

    if (debug)
        qDebug() << "CA::startGetPageSourceRequest: url = " + request.url().toString();

    killTimer->start();

    connect(networkManager, &QNetworkAccessManager::finished,
        this, &ConfluenceAgent::pageSourceReceived);

    networkManager->get(request);
}

void ConfluenceAgent::pageSourceReceived(QNetworkReply *reply)
{
    if (debug) qDebug() << "CA::pageSourceReceived";

    killTimer->stop();
    networkManager->disconnect();
    reply->deleteLater();

    QByteArray fullReply = reply->readAll();
    if (!wasRequestSuccessful(reply, "receive page source", fullReply))
        return;

    // Find pageID
    QRegExp rx("\\sname=\"ajs-page-id\"\\scontent=\"(\\d*)\"");
    rx.setMinimal(true);

    if (rx.indexIn(fullReply, 0) != -1) {
        pageID = rx.cap(1);
    }
    else {
        qWarning()
            << "ConfluenceAgent::pageSourceReveived Couldn't find page ID";
        //qWarning() << fullReply;
        return;
    }

    // Find spaceKey 
    rx.setPattern("meta\\s*id=\"confluence-space-key\"\\s* "
                  "name=\"confluence-space-key\"\\s*content=\"(.*)\"");
    if (rx.indexIn(fullReply, 0) != -1) {
        spaceKey = rx.cap(1);
    }
    else {
        qWarning() << "ConfluenceAgent::pageSourceReveived Couldn't find "
                      "space key in response";
        qWarning() << fullReply;
        finishJob();
        return;
    }

    const QVariant redirectionTarget =
        reply->attribute(QNetworkRequest::RedirectionTargetAttribute);

    continueJob();
}

void ConfluenceAgent::startGetPageDetailsRequest()
{
    if (debug) qDebug() << "CA::startGetPageDetailsRequest" << pageID;

    // Authentication in URL  (only SSL!)
    QString url = "https://" 
        + baseURL + apiURL 
        + "/content/" + pageID + "?expand=metadata.labels,version";

    QNetworkRequest request = createRequest(url);

    connect(networkManager, &QNetworkAccessManager::finished,
        this, &ConfluenceAgent::pageDetailsReceived);

    killTimer->start();

    networkManager->get(request);
}

void ConfluenceAgent::pageDetailsReceived(QNetworkReply *reply)
{
    if (debug) qDebug() << "CA::pageDetailsReceived";

    killTimer->stop();
    networkManager->disconnect();
    reply->deleteLater();

    QByteArray fullReply = reply->readAll();
    if (!wasRequestSuccessful(reply, "receive page details", fullReply))
        return;

    QJsonDocument jsdoc;
    jsdoc = QJsonDocument::fromJson(fullReply);

    pageObj = jsdoc.object();
    // cout << jsdoc.toJson(QJsonDocument::Indented).toStdString();

    continueJob();
}

void ConfluenceAgent::startCreatePageRequest()
{
    // qDebug() << "CA::startCreatePageRequest";

    QString url = "https://" + baseURL + apiURL + "/content";

    QNetworkRequest request = createRequest(url);
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
    if (!loadStringFromDisk(uploadPagePath, body))
    {
        qWarning() << "ConfluenceAgent: Couldn't read file to upload:" << uploadPagePath;
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
        this, &ConfluenceAgent::pageUploaded);

    killTimer->start();

    networkManager->post(request, data);
}

void ConfluenceAgent::startUpdatePageRequest()
{
    if (debug) qDebug() << "CA::startUpdatePageRequest";

    QString url = "https://" + baseURL + apiURL + "/content" + "/" + pageID;

    QNetworkRequest request = createRequest(url);

    request.setHeader(
        QNetworkRequest::ContentTypeHeader,
        "application/json; charset=utf-8");

    QJsonObject payload;
    payload["id"] = pageID;
    payload["type"] = "page";
    payload["title"] = newPageName;

    // Build version object
    QJsonObject newVersionObj;
    QJsonObject oldVersionObj = pageObj["version"].toObject();

    newVersionObj["number"] = oldVersionObj["number"].toInt() + 1;
    payload["version"] = newVersionObj;

    // Build object with space key
    QJsonObject skey;
    skey["key"] = spaceKey;
    payload["space"] = skey;

    // Build body
    QString body;
    if (!loadStringFromDisk(uploadPagePath, body))
    {
        qWarning() << "ConfluenceAgent: Couldn't read file to upload:" << uploadPagePath;
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
        this, &ConfluenceAgent::pageUploaded);

    killTimer->start();

    networkManager->put(request, data);
}

void ConfluenceAgent::pageUploaded(QNetworkReply *reply)
{
    if (debug) qDebug() << "CA::pageUploaded";

    killTimer->stop();
    networkManager->disconnect();
    reply->deleteLater();

    QByteArray fullReply = reply->readAll();
    if (!wasRequestSuccessful(reply, "upload page", fullReply))
        return;

    QJsonDocument jsdoc;
    jsdoc = QJsonDocument::fromJson(fullReply);
    pageObj = jsdoc.object();
    //cout << jsdoc.toJson(QJsonDocument::Indented).toStdString();
    continueJob();
}

void ConfluenceAgent::startGetUserInfoRequest()
{
    if (debug) qDebug() << "CA::startGetInfoRequest for " << userQuery;

    QString url = "https://" + baseURL + apiURL
        + "/search?cql=user.fullname~" + userQuery;

    networkManager->disconnect();

    QNetworkRequest request = createRequest(url);

    connect(networkManager, &QNetworkAccessManager::finished,
        this, &ConfluenceAgent::userInfoReceived);

    killTimer->start();

    networkManager->get(request);
}

void ConfluenceAgent::userInfoReceived(QNetworkReply *reply)
{
    if (debug) qDebug() << "CA::UserInfopageReceived";

    killTimer->stop();
    networkManager->disconnect();
    reply->deleteLater();

    QByteArray fullReply = reply->readAll();
    if (!wasRequestSuccessful(reply, "receive user info", fullReply))
        return;

    QJsonDocument jsdoc;
    jsdoc = QJsonDocument::fromJson(fullReply);
    pageObj = jsdoc.object();
    continueJob();
}

void ConfluenceAgent::startGetAttachmentsInfoRequest()
{
    if (debug) qDebug() << "CA::startGetAttachmentIdRequest";

    QString url = "https://" + baseURL + apiURL + "/content" + "/" + pageID + "/child/attachment";

    QNetworkRequest request = createRequest(url);
    request.setRawHeader("X-Atlassian-Token", "no-check");

    connect(networkManager, &QNetworkAccessManager::finished,
        this, &ConfluenceAgent::attachmentsInfoReceived);

    killTimer->start();

    QNetworkReply *reply = networkManager->get(request);
}

void ConfluenceAgent::attachmentsInfoReceived(QNetworkReply *reply)
{
    if (debug) qDebug() << "CA::attachmentsInfoReceived";

    killTimer->stop();
    networkManager->disconnect();
    reply->deleteLater();

    QByteArray fullReply = reply->readAll();
    if (!wasRequestSuccessful(reply, "get attachment info", fullReply))
        return;

    QJsonDocument jsdoc;
    jsdoc = QJsonDocument::fromJson(fullReply);

    attachmentObj = jsdoc.object();
    int attachmentsCount = jsdoc["size"].toInt();
    //cout << jsdoc.toJson(QJsonDocument::Indented).toStdString();
    for (int i = 0; i < attachmentsCount; i++) {
        attachmentsTitles << jsdoc["results"][i]["title"].toString();
        attachmentsIds    << jsdoc["results"][i]["id"].toString();
        //qDebug() << " Title: " << attachmentsTitles.last() << 
        //            " Id: " << attachmentsIds.last();
    }

    continueJob();
}

void ConfluenceAgent::startCreateAttachmentRequest()
{
    if (debug) qDebug() << "CA::startCreateAttachmentRequest";

    QString url = "https://" + baseURL + apiURL + "/content" + "/" + pageID + "/child/attachment";

    QNetworkRequest request = createRequest(url);
    request.setRawHeader("X-Atlassian-Token", "no-check");

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);


    QHttpPart imagePart;
    imagePart.setHeader(
            QNetworkRequest::ContentDispositionHeader,

            // Name must be "file"
            QVariant(
                QString("form-data; name=\"file\"; filename=\"%1\"")
                    .arg(currentAttachmentTitle)));
    imagePart.setHeader(
            QNetworkRequest::ContentTypeHeader,
            QVariant("image/jpeg"));

    QFile *file = new QFile(currentAttachmentPath);
    if (!file->open(QIODevice::ReadOnly)) {
        qWarning() << "Problem opening attachment: " << currentAttachmentPath;
        QMessageBox::warning(
            nullptr, tr("Warning"),
            QString("Could not open attachment file \"%1\" in page with ID: %2").arg(currentAttachmentTitle).arg(pageID));
        finishJob();
        return;
    }
    imagePart.setBodyDevice(file);
    /*
    qDebug() << "      title=" << currentAttachmentTitle;
    qDebug() << "       path=" << currentAttachmentPath;
    qDebug() << "        url=" << url;
    qDebug() << "  file size=" << file->size();
    */
    multiPart->append(imagePart);
    file->setParent(multiPart); // delete later with the multiPart

    connect(networkManager, &QNetworkAccessManager::finished,
        this, &ConfluenceAgent::attachmentCreated);

    killTimer->start();

    QNetworkReply *reply = networkManager->post(request, multiPart);

    multiPart->setParent(reply);
}

void ConfluenceAgent::attachmentCreated(QNetworkReply *reply)
{
    if (debug) qDebug() << "CA::attachmentCreated";

    killTimer->stop();
    networkManager->disconnect();
    reply->deleteLater();

    QByteArray fullReply = reply->readAll();
    if (reply->error() == QNetworkReply::ProtocolInvalidOperationError) {
        if (fullReply.contains(
                    QString("Cannot add a new attachment with same file name as an existing attachment").toLatin1())) {
            // Replace existing attachment
            qWarning() << "Attachment with name " << currentAttachmentTitle << " already exists.";
            qWarning() << "AttachmentID unknown, stopping now"; 

            finishJob();
            return;
        }
        if (!wasRequestSuccessful(reply, "create attachment", fullReply))
            return;
    }

    QJsonDocument jsdoc;
    jsdoc = QJsonDocument::fromJson(fullReply);
    attachmentObj = jsdoc.object();

    //qDebug() << "CA::attachmentCreated Successful:";
    //cout << jsdoc.toJson(QJsonDocument::Indented).toStdString();
    //cout << attachmentObj["results"].toArray().toStdString();

    currentUploadAttachmentIndex++;

    continueJob(2);
}

void ConfluenceAgent::startUpdateAttachmentRequest()
{
    if (debug) qDebug() << "CA::startUpdateAttachmentRequest";

    for (int i = 0; i < attachmentsTitles.count(); i++) {
        // qDebug() << "     - " << attachmentsTitles.at(i);
        if (attachmentsTitles.at(i) == currentAttachmentTitle) {
            currentAttachmentId = attachmentsIds.at(i);
            break;
        }
    }

    if (currentAttachmentId.isEmpty()) {
        QMessageBox::warning(
            nullptr, tr("Warning"),
            QString("Could not find existing attachment \"%1\" in page with ID: %2").arg(currentAttachmentTitle).arg(pageID));
        finishJob();
        return;
    }

    QString url = "https://" + baseURL + apiURL + "/content" + "/" + pageID + "/child/attachment/" + currentAttachmentId + "/data";

    QNetworkRequest request = createRequest(url);
    request.setRawHeader("X-Atlassian-Token", "no-check");

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart imagePart;
    imagePart.setHeader(
            QNetworkRequest::ContentDispositionHeader,

            // Name must be "file"
            QVariant(
                QString("form-data; name=\"file\"; filename=\"%1\"")
                    .arg(currentAttachmentTitle)));
    imagePart.setHeader(
            QNetworkRequest::ContentTypeHeader,
            QVariant("image/jpeg"));

    QFile *file = new QFile(currentAttachmentPath);
    if (!file->open(QIODevice::ReadOnly)) {
        qWarning() << "Problem opening attachment: " << currentAttachmentPath;
        QMessageBox::warning(
            nullptr, tr("Warning"),
            QString("Could not open attachment file \"%1\" in page with ID: %2").arg(currentAttachmentTitle).arg(pageID));
        finishJob();
        return;
    }
    imagePart.setBodyDevice(file);
    /*
    qDebug() << "      title=" << currentAttachmentTitle;
    qDebug() << "       path=" << currentAttachmentPath;
    qDebug() << "        url=" << url;
    qDebug() << "  file size=" << file->size();
    */
    multiPart->append(imagePart);
    file->setParent(multiPart);

    connect(networkManager, &QNetworkAccessManager::finished,
        this, &ConfluenceAgent::attachmentUpdated);

    killTimer->start();

    QNetworkReply *reply = networkManager->post(request, multiPart);

    multiPart->setParent(reply);
}

void ConfluenceAgent::attachmentUpdated(QNetworkReply *reply)
{
    if (debug) qDebug() << "CA::attachmentUpdated";

    killTimer->stop();
    networkManager->disconnect();
    reply->deleteLater();

    QByteArray fullReply = reply->readAll();
    if (!wasRequestSuccessful(reply, "update attachment", fullReply))
        return;

    QJsonDocument jsdoc;
    jsdoc = QJsonDocument::fromJson(fullReply);
    attachmentObj = jsdoc.object();

    //cout << jsdoc.toJson(QJsonDocument::Indented).toStdString();

    currentUploadAttachmentIndex++;

    continueJob(2);
}

void ConfluenceAgent::attachmentsUploadSuccess() // slot called from attachmentsAgent
{
    continueJob();
}

void ConfluenceAgent::attachmentsUploadFailure() // slot called from attachmentsAgent
{
    qWarning() << "CA::attachmentsUpload failed";
    finishJob();
}

bool ConfluenceAgent::wasRequestSuccessful(QNetworkReply *reply, const QString &requestDesc, const QByteArray &fullReply)
{
    if (reply->error()) {

        // Additionally print full error on console
        qWarning() << "         Step: " << requestDesc;
        qWarning() << "        Error: " << reply->error();
        qWarning() << "  Errorstring: " <<  reply->errorString();

        qDebug() << "    Request Url: " << reply->url() ;
        qDebug() << "      Operation: " << reply->operation() ;

        qDebug() << "      readAll: ";
        QJsonDocument jsdoc;
        jsdoc = QJsonDocument::fromJson(fullReply);
        QString fullReplyFormatted = QString(jsdoc.toJson(QJsonDocument::Indented));
        cout << fullReplyFormatted.toStdString();

        /*
        qDebug() << "Request headers: ";
	QList<QByteArray> reqHeaders = reply->rawHeaderList();
	foreach( QByteArray reqName, reqHeaders )
        {
            QByteArray reqValue = reply->rawHeader( reqName );
            qDebug() << "  " << reqName << ": " << reqValue;
	}
        */

        if (reply->error() == QNetworkReply::AuthenticationRequiredError)
            QMessageBox::warning(
                nullptr, tr("Warning"),
                tr("Authentication problem when contacting Confluence") + "\n\n" + 
                requestDesc);
        else {
            QString msg = QString("QNetworkReply error when trying to \"%1\"\n\n").arg(requestDesc);
            WarningDialog warn;
            warn.setText(msg + "\n\n" + fullReplyFormatted);
            warn.showCancelButton(false);
            warn.exec();
        }

        finishJob();
        return false;
    } else
        return true;
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
