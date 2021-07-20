#include "confluence-agent.h"

#include "branchitem.h"
#include "file.h"
#include "mainwindow.h"
#include "misc.h"
#include "vymmodel.h"

extern Main *mainWindow;
extern Settings settings;
extern QDir vymBaseDir;
extern bool debug;
extern QString confluencePassword;

ConfluenceAgent::ConfluenceAgent() { 
    qDebug() << "Constr. ConfluenceAgent";
    init(); 
}

ConfluenceAgent::ConfluenceAgent(BranchItem *bi)
{
    qDebug() << "Constr. ConfluenceAgent selbi = " << bi;

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
    qDebug() << "CA::Destr.";
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

    vymProcess = // FIXME-0 not needed soon
        nullptr; // Only one process may be active at any time in this agent

    QObject::connect(killTimer, SIGNAL(timeout()), this, SLOT(timeout()));

    succ = false;

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

void ConfluenceAgent::test()
{
    QStringList args;

    args << "-h";

    qWarning() << "ConfluenceAgent::test() called";

    vymProcess->start(confluenceScript, args);

    if (!vymProcess->waitForStarted()) {
        qWarning() << "ConfluenceAgent::test()  couldn't start "
                   << confluenceScript;
        return;
    }

    killTimer->start();
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

    qDebug() << "CA::contJob " << jobType << " Step: " << jobStep;

    switch(jobType) {
        case CopyPagenameToHeading:
            switch(jobStep) {
                case 1:
                    startGetPageSourceRequest(pageURL);
                    break;
                case 2:
                    startGetPageDetailsRequest("/content/" + pageID +
                                               "?expand=metadata.labels,version");
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
            qDebug() << "CA::continueJob NewPage: step " << jobStep;
            switch(jobStep) {
                case 1:
                    if (pageURL.isEmpty()) {
                        qDebug() << "CA::contJob NewPage: pageURL is empty";
                        finishJob();
                        return;
                    }
                    if (newPageTitle.isEmpty()) {
                        qDebug() << "CA::contJob NewPage: newPageTitle is empty";
                        finishJob();
                        return;
                    }

                    // Check if parent page with url already exists
                    startGetPageSourceRequest(pageURL);
                    break;
                case 2:
                    // Create new page with parent url
                    startGetPageDetailsRequest("/content/" + pageID +
                                               "?expand=metadata.labels,version");
                    break;
                case 3:
                    startUploadContentRequest();
                    break;
                case 4:
                    qDebug() << "CA::finished  Created page with ID: " << jsobj["id"].toString();
                    finishJob();
                    break;
                default:
                    unknownStepWarning();
            };

            break;
        default:
            qDebug() << "ConfluenceAgent::startJob   unknown jobType " << jobType;
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

bool ConfluenceAgent::getUsers(const QString &name)
{
    QStringList args; // FIXME-3 refactor so that args are passed to one
                      // function starting the process

    args << "-s";
    args << name;

    if (debug)
        qDebug().noquote() << QString("ConfluenceAgent::getUsers\n%1 %2")
                                  .arg(confluenceScript)
                                  .arg(args.join(" "));

    vymProcess = new VymProcess;

    connect(vymProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this,
            SLOT(dataReceived(int, QProcess::ExitStatus)));

    vymProcess->start(confluenceScript, args);

    if (!vymProcess->waitForStarted()) {
        qWarning() << "ConfluenceAgent::getUsers  couldn't start "
                   << confluenceScript;
        return false;
    }

    return true;
}

void ConfluenceAgent::waitForResult()
{
    if (!vymProcess) {
        qWarning() << "ConfluenceAgent: No running vymProces";
        return;
    }
    if (!vymProcess->waitForFinished(10000)) {
        qWarning() << "ConfluenceAgent: Timeout.";
        return;
    }
}

bool ConfluenceAgent::success() { return succ; }

QString ConfluenceAgent::getResult() { return result; }

void ConfluenceAgent::dataReceived( // FIXME-0 remove, obsolete!
    int exitCode,
    QProcess::ExitStatus exitStatus) // FIXME-3  return value???   // FIXME-3
                                     // name correct? used by all functions...
{
    if (exitStatus == QProcess::NormalExit) {
        result = vymProcess->getStdout();

        QString err = vymProcess->getErrout();
        if (!err.isEmpty()) {
            qWarning() << "ConfluenceAgent process error: \n" << err;
        }
        else {
            if (!result.startsWith("Error"))
                succ = true;
        }
    }
    else
        qWarning() << "ConfluenceAgent: Process finished with exitCode="
                   << exitCode;
    vymProcess = nullptr;
}

void ConfluenceAgent::timeout()
{
    qWarning() << "ConfluenceAgent timeout!!   jobType = " << jobType;
}

void ConfluenceAgent::startGetPageSourceRequest(QUrl requestedURL)
{
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
        qDebug() << "CA::startGetPageSourceRequest:" + request.url().toString();
    }

    killTimer->start();

    connect(networkManager, &QNetworkAccessManager::finished,
        this, &ConfluenceAgent::pageSourceReceived);

    networkManager->get(request);
}

void ConfluenceAgent::startGetPageDetailsRequest(QString query)
{
    qDebug() << "CA::startGetPageDetailsRequest" << query;

    httpRequestAborted = false;

    // Authentication in URL  (only SSL!)
    // maybe switch to token later:
    // https://developer.atlassian.com/cloud/confluence/basic-auth-for-rest-apis/
    QString concatenated = username + ":" + password;

    query = "https://" + concatenated + "@" + apiURL + query;

    QNetworkRequest request = QNetworkRequest(QUrl(query));

    connect(networkManager, &QNetworkAccessManager::finished,
        this, &ConfluenceAgent::pageDetailsReceived);

    killTimer->start();

    networkManager->get(request);
}

void ConfluenceAgent::startUploadContentRequest()
{
    qDebug() << "CA::startUploadContentRequest";

    httpRequestAborted = false;

    QString concatenated = username + ":" + password;

    QString url = "https://" + concatenated + "@" + apiURL + "/content";

    // FIXME-0 add JSON payload, see confluence script

    QNetworkRequest request = QNetworkRequest(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // https://stackoverflow.com/questions/2599423/how-can-i-post-data-to-a-url-using-qnetworkaccessmanager
    // https://stackoverflow.com/questions/60107604/how-to-send-a-post-request-in-qt-with-the-json-body

    //QByteArray postData = query.toString(QUrl::FullyEncoded).toUtf8();

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

void ConfluenceAgent::pageSourceReceived(QNetworkReply *reply)
{
    qDebug() << "CA::pageSourceReceived";

    killTimer->stop();

    disconnect(networkManager, &QNetworkAccessManager::finished,
        this, &ConfluenceAgent::pageSourceReceived);

    QString r = reply->readAll();

    // Check for page id
    QRegExp rx("\\sname=\"ajs-page-id\"\\scontent=\"(\\d*)\"");
    rx.setMinimal(true);

    if (rx.indexIn(r, 0) != -1) {
        pageID = rx.cap(1);
    }
    else {
        qWarning()
            << "ConfluenceAgent::pageSourceReveived Couldn't find page ID";
        return;
    }

    // Check for space key
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

    if (httpRequestAborted) {
        qWarning() << "ConfluenceAgent::pageSourceReveived aborted";
        finishJob();
        return;
    }

    if (reply->error()) {
        qWarning() << "ConfluenceAgent::pageSoureReveived reply error";
        qWarning() << "Error: " << reply->error();
        finishJob();
        return;
    }

    const QVariant redirectionTarget =
        reply->attribute(QNetworkRequest::RedirectionTargetAttribute);

    continueJob();
}

void ConfluenceAgent::pageDetailsReceived(QNetworkReply *reply)
{
    qDebug() << "CA::pageDetailsReceived";

    killTimer->stop();

    disconnect(networkManager, &QNetworkAccessManager::finished,
        this, &ConfluenceAgent::pageDetailsReceived);

    if (httpRequestAborted) {
        qWarning() << "ConfluenceAgent::pageDetailsReveived aborted error";
        finishJob();
        return;
    }

    if (reply->error()) {
        qWarning() << "ConfluenceAgent::pageDetailsReveived reply error";
        qDebug() << reply->error();
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
    qDebug() << "CA::contentUploaded";

    killTimer->stop();

    disconnect(networkManager, &QNetworkAccessManager::finished,
        this, &ConfluenceAgent::pageDetailsReceived);

    if (httpRequestAborted) {
        qWarning() << "ConfluenceAgent::pageDetailsReveived aborted error";
        finishJob();
        return;
    }

    if (reply->error()) {
        qWarning() << "ConfluenceAgent::pageDetailsReveived reply error";
        qDebug() << reply->error();
        finishJob();
        return;
    }

    QJsonDocument jsdoc;
    jsdoc = QJsonDocument::fromJson(reply->readAll());
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
