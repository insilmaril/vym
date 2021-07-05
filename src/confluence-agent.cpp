#include "confluence-agent.h"

#include "branchitem.h"
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

    branchID = bi->getID();
    VymModel *model = bi->getModel();
    modelID = model->getModelID();
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

    killTimer = nullptr;

    confluenceScript = vymBaseDir.path() + "/scripts/confluence.rb";

    killTimer = new QTimer(this);
    killTimer->setInterval(15000);
    killTimer->setSingleShot(true);

    vymProcess =
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

void ConfluenceAgent::startJob()
{
    switch(jobType) {
        case CopyPagenameToHeading:
            qDebug() << "CA::startJob " << jobType << pageURL;
            getPageSource();
            break;
        default:
            qDebug() << "ConfluenceAgent::startJob   unknown jobType " << jobType;
    }
}

void ConfluenceAgent::finishJob()
{
    if (reply)
        reply->deleteLater();

    deleteLater();
}

void ConfluenceAgent::setPageURL(QString u)
{
    pageURL = u;
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

bool ConfluenceAgent::getPageDetails(const QString &url)
{
    QStringList args;

    args << "-d";
    args << url;

    if (debug)
        qDebug().noquote() << QString("ConfluenceAgent::getPageDetails\n%1 %2")
                                  .arg(confluenceScript)
                                  .arg(args.join(" "));

    vymProcess = new VymProcess;

    connect(vymProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this,
            SLOT(dataReceived(int, QProcess::ExitStatus)));

    vymProcess->start(confluenceScript, args);

    if (!vymProcess->waitForStarted()) {
        qWarning() << "ConfluenceAgent::getPageDetails  couldn't start "
                   << confluenceScript;
        return false;
    }

    return true;
}

bool ConfluenceAgent::getPageSource()
{
    qDebug() << "CA::getPageSource of " << pageURL; // FIXME-2 testing
    if (!QUrl(pageURL).isValid()) {
        qWarning() << "ConfluenceAgent: Invalid URL: " << pageURL;
        return false;
    }

    // schedule the request
    startGetPageSourceRequest(pageURL);

    return true;
}

bool ConfluenceAgent::uploadContent(const QString &url, const QString &title,
                                    const QString &fpath, const bool &newPage)
{
    QStringList args;

    if (newPage)
        args << "-c";
    else
        args << "-u";
    args << url;
    args << "-f";
    args << fpath;
    if (!title.isEmpty()) {
        args << "-t";
        args << title;
    }

    if (debug) {
        qDebug().noquote() << QString("ConfluenceAgent::uploadContent\n%1 %2")
                                  .arg(confluenceScript)
                                  .arg(args.join(" "));

        qDebug() << "  newPage: " << newPage;
    }

    vymProcess = new VymProcess;

    connect(vymProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this,
            SLOT(dataReceived(int, QProcess::ExitStatus)));

    vymProcess->start(confluenceScript, args);

    if (!vymProcess->waitForStarted()) {
        qWarning() << "ConfluenceAgent::uploadContent  couldn't start "
                   << confluenceScript;
        return false;
    }

    return true;
}

bool ConfluenceAgent::updatePage(const QString &url, const QString &title,
                                 const QString &fpath)
{
    return uploadContent(url, title, fpath, false);
}

bool ConfluenceAgent::createPage(const QString &url, const QString &title,
                                 const QString &fpath)
{
    return uploadContent(url, title, fpath, true);
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

void ConfluenceAgent::dataReceived(
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
    if (vymProcess) {
        // delete (vymProcess);  // FIXME-3  crashes in ConfluenceAgent -
        // deleteLater()?
        vymProcess = nullptr;
    }

    if (reply) {
        reply->abort();
        reply->deleteLater();
        reply = nullptr;
    }
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
        qDebug() << "CA::startGetPageSourceRequest="
                 << request.url().toString();

    killTimer->start();

    reply = qnam.get(request);

    disconnect(reply);
    connect(reply, &QNetworkReply::finished, this,
            &ConfluenceAgent::pageSourceReceived);
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

    reply = qnam.get(request);

    // Disconnect reply
    disconnect(reply);

    killTimer->start();

    connect(reply, &QNetworkReply::finished, this,
            &ConfluenceAgent::pageDetailsReceived);
}

void ConfluenceAgent::pageSourceReceived()
{
    qDebug() << "CA::pageSourceReceived";

    killTimer->stop();

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
        reply->deleteLater();
        reply = nullptr;
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

    startGetPageDetailsRequest("/content/" + pageID +
                               "?expand=metadata.labels,version");
}

void ConfluenceAgent::pageDetailsReceived()
{
    qDebug() << "CA::pageDetailsReceived";

    killTimer->stop();

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

    // const QVariant redirectionTarget =
    // reply->attribute(QNetworkRequest::RedirectionTargetAttribute);

    /*
    details.version = r['version']['number']
    details.labels = r['metadata']['labels']['results']
    */

    QJsonDocument jsdoc;
    jsdoc = QJsonDocument::fromJson(reply->readAll());

    QJsonObject jsobj = jsdoc.object();

    /*
    QJsonArray jsarr = jsobj[“feeds”].toArray();
    foreach (const QJsonValue &value, jsarr) {
        QJsonObject jsob = value.toObject();
        qDebug() << jsob[“entry_id”].toInt();
        qDebug() << jsob[“field1”].toString();
        qDebug() << jsob[“created_at”].toString();
    */

    VymModel *model = mainWindow->getModel(modelID);
    if (model) {
        BranchItem *bi = (BranchItem *)(model->findID(branchID));

        if (bi) {
            QString h = spaceKey + ": " + jsobj["title"].toString();
            model->setHeading(h, bi);
        }
        else
            qWarning() << "CA::pageDetailsReceived couldn't find branch "
                       << branchID;
    }
    else
        qWarning() << "CA::pageDetailsReceived couldn't find model " << modelID;

    finishJob();
}

#ifndef QT_NO_SSL
void ConfluenceAgent::sslErrors(QNetworkReply *, const QList<QSslError> &errors)
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
