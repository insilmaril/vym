#include "confluence-agent.h"

#include "branchitem.h"
#include "mainwindow.h"
#include "misc.h"
#include "vymmodel.h"

extern Main *mainWindow;
extern QSettings settings;
extern QDir vymBaseDir;
extern bool debug;
extern QString confluencePassword;

ConfluenceAgent::ConfluenceAgent ()
{ 
    init();
}

ConfluenceAgent::ConfluenceAgent(BranchItem *bi)
{
    init();
    if (!bi)
    {
	qWarning ("Const ConfluenceAgent: bi == nullptr");
	delete (this);
	return;
    }

    branchID = bi->getID();
    VymModel *model = bi->getModel();
    modelID = model->getModelID();
}

void ConfluenceAgent::init()
{
    killTimer = nullptr;

    confluenceScript = vymBaseDir.path() + "/scripts/confluence.rb";

    killTimer = new QTimer(this); 
    killTimer->setInterval(10000); 
    killTimer->setSingleShot(true); 

    vymProcess = nullptr;  // Only one process may be active at any time in this agent

    QObject::connect(killTimer, SIGNAL(timeout()), this, SLOT(timeout()));

    succ = false;

    // Read credentials // FIXME-2 dialog to set credentials, if not saved before
    username = settings.value("/confluence/username","user_johnDoe").toString();
    password = confluencePassword;
    baseURL = settings.value("/confluence/url","baseURL").toString();

    apiURL = baseURL + "/rest/api";
}

ConfluenceAgent::~ConfluenceAgent ()
{
    if (killTimer) delete killTimer;
}

void ConfluenceAgent::test()
{
    QStringList args;

    args << "-h";

    qWarning() << "ConfluenceAgent::test() called";

    vymProcess->start (confluenceScript, args);

    if (!vymProcess->waitForStarted())
    {
	qWarning() << "ConfluenceAgent::test()  couldn't start " << confluenceScript;
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
        qDebug().noquote() << QString("ConfluenceAgent::getPageDetails\n%1 %2").arg(confluenceScript).arg(args.join(" "));

    vymProcess = new VymProcess;

    connect (vymProcess, SIGNAL (finished(int, QProcess::ExitStatus) ), 
	this, SLOT (dataReceived(int, QProcess::ExitStatus) ));

    vymProcess->start (confluenceScript, args);

    if (!vymProcess->waitForStarted())
    {
	qWarning() << "ConfluenceAgent::getPageDetails  couldn't start " << confluenceScript;
	return false; 
    } 

    return true;
}

bool ConfluenceAgent::getPageDetailsNative(const QString &u)
{
    QUrl url (u);
    if (!url.isValid()) {
        qWarning() << "ConfluenceAgent: Invalid URL: " << u;
        return false;
    }

    pageURL = u;

    // schedule the request
    startGetPageSourceRequest(url);

    killTimer->start();

    return true;
}

bool ConfluenceAgent::uploadContent(const QString &url, const QString &title, const QString &fpath, const bool &newPage)
{
    QStringList args;

    if (newPage)
        args << "-c";
    else
        args << "-u";
    args << url;
    args << "-f";
    args << fpath;
    if (!title.isEmpty())
    {
        args << "-t";
        args << title;
    }

        qDebug().noquote() << QString("ConfluenceAgent::uploadContent\n%1 %2").arg(confluenceScript).arg(args.join(" "));

    if (debug)  qDebug() << "  newPage: " << newPage;  

    vymProcess = new VymProcess;

    connect (vymProcess, SIGNAL (finished(int, QProcess::ExitStatus) ), 
	this, SLOT (dataReceived(int, QProcess::ExitStatus) ));

    vymProcess->start (confluenceScript, args);

    if (!vymProcess->waitForStarted())
    {
	qWarning() << "ConfluenceAgent::uploadContent  couldn't start " << confluenceScript;
	return false; 
    } 

    return true;
}

bool ConfluenceAgent::updatePage(const QString &url, const QString &title, const QString &fpath)
{
    return uploadContent (url, title, fpath, false);
}

bool ConfluenceAgent::createPage(const QString &url, const QString &title, const QString &fpath)
{
    return uploadContent (url, title, fpath, true);
}

bool ConfluenceAgent::getUsers(const QString &name)
{
    QStringList args;   // FIXME-3 refactor so that args are passed to one function starting the process

    args << "-s";
    args << name;

    if (debug)  
        qDebug().noquote() << QString("ConfluenceAgent::getUsers\n%1 %2").arg(confluenceScript).arg(args.join(" "));

    vymProcess = new VymProcess;

    connect (vymProcess, SIGNAL (finished(int, QProcess::ExitStatus) ), 
	this, SLOT (dataReceived(int, QProcess::ExitStatus) ));

    vymProcess->start (confluenceScript, args);

    if (!vymProcess->waitForStarted())
    {
	qWarning() << "ConfluenceAgent::getUsers  couldn't start " << confluenceScript;
	return false; 
    } 

    return true;
}

void ConfluenceAgent::waitForResult()   
{
    if (!vymProcess) 
    {
        qWarning() << "ConfluenceAgent: No running vymProces";
        return;
    }
    if (!vymProcess->waitForFinished( 10000 ) )
    {
        qWarning() << "ConfluenceAgent: Timeout.";
        return;
    }
}

bool ConfluenceAgent::success()
{
    return succ;
}

QString ConfluenceAgent::getResult()
{
    return result;
}


void ConfluenceAgent::dataReceived(int exitCode, QProcess::ExitStatus exitStatus)    // FIXME-3  return value???   // FIXME-3  name correct? used by all functions...
{
    if (exitStatus == QProcess::NormalExit)
    {
	result = vymProcess->getStdout();

	QString err = vymProcess->getErrout();
	if (!err.isEmpty())
        {
	    qWarning() << "ConfluenceAgent process error: \n" << err;
        } else 
        {
            if (!result.startsWith("Error")) succ = true;
        }
            
    } else	
	qWarning() << "ConfluenceAgent: Process finished with exitCode=" << exitCode;
    vymProcess = nullptr;
}

void ConfluenceAgent::timeout()
{
    qWarning() << "ConfluenceAgent timeout!!";
    if (vymProcess)
    {
        // delete (vymProcess);  // FIXME-3  crashes in ConfluenceAgent -  deleteLater()?
        vymProcess = nullptr;
    }

    if (reply)
    {
        reply->abort();
        reply->deleteLater();
        reply = nullptr;
    }


}
    
void ConfluenceAgent::startGetPageSourceRequest(QUrl requestedURL)
{
    if (!requestedURL.toString().startsWith("http"))
        requestedURL.setPath("https://" + requestedURL.path() );

    QUrl url = requestedURL;
    httpRequestAborted = false;

    QNetworkRequest request = QNetworkRequest(url);
    
    // Basic authentication in header
    QString concatenated = username + ":" + password;
    QByteArray data = concatenated.toLocal8Bit().toBase64();
    QString headerData = "Basic " + data;
    request.setRawHeader("Authorization", headerData.toLocal8Bit());

    if (debug) qDebug() << "CA::startGetPageSourceRequest=" << request.url().toString();

    reply = qnam.get(request);

    disconnect();
    connect(reply, &QNetworkReply::finished, this, &ConfluenceAgent::pageSourceReceived);
}

void ConfluenceAgent::startGetPageDetailsRequest(QString query)
{
    if (debug) qDebug() << "CA::startGetPageDetailsRequest" << query;

    httpRequestAborted = false;

    // Authentication in URL  (only SSL!)
    // maybe switch to token later:
    // https://developer.atlassian.com/cloud/confluence/basic-auth-for-rest-apis/
    QString concatenated = username + ":" + password;

    query = "https://" + concatenated + "@" + apiURL + query;

    QNetworkRequest request = QNetworkRequest(QUrl(query));
    
    reply = qnam.get(request);

    disconnect();

    connect(reply, &QNetworkReply::finished, this, &ConfluenceAgent::pageDetailsReceived);
}

void ConfluenceAgent::pageSourceReceived()
{
    if (debug) qDebug() << "CA::pageSourceReceived";

    QString r = reply->readAll();

    // Check for page id
    QRegExp rx("\\sname=\"ajs-page-id\"\\scontent=\"(\\d*)\"");
    rx.setMinimal(true);

    if (rx.indexIn(r, 0) != -1) 
    {
        pageID = rx.cap(1);
    } else
    {
        qWarning() << "ConfluenceAgent::pageSourceReveived Couldn't find page ID";
        reply->deleteLater();
        reply = nullptr;
        return;
    }

    // Check for space key
    rx.setPattern("meta\\s*id=\"confluence-space-key\"\\s* name=\"confluence-space-key\"\\s*content=\"(.*)\"");
    if (rx.indexIn(r, 0) != -1) 
    {
        spaceKey = rx.cap(1);
    } else
    {
        qWarning() << "ConfluenceAgent::pageSourceReveived Couldn't find page space key";
        reply->deleteLater();
        reply = nullptr;
        return;
    }

    if (httpRequestAborted) {
        qWarning() << "ConfluenceAgent::pageSoureReveived aborted";
        reply->deleteLater();
        reply = nullptr;
        return;
    }

    if (reply->error()) {
        qWarning() << "ConfluenceAgent::pageSoureReveived reply error";
        reply->deleteLater();
        reply = nullptr;
        return;
    }

    const QVariant redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);

    startGetPageDetailsRequest( "/content/" + pageID + "?expand=metadata.labels,version");
}

void ConfluenceAgent::pageDetailsReceived()
{
    if (debug) qDebug() << "CA::pageDetailsReceived";

    if (httpRequestAborted) {
        qWarning() << "ConfluenceAgent::pageDetailsReveived aborted error";
        reply->deleteLater();
        reply = nullptr;
        return;
    }

    if (reply->error()) {
        qWarning() << "ConfluenceAgent::pageDetailsReveived reply error";
        qDebug() << reply->error();
        reply->deleteLater();
        reply = nullptr;
        return;
    }

    //const QVariant redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);

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

    VymModel *model = mainWindow->getModel (modelID);
    if (model)
    {
        BranchItem *bi = (BranchItem*)(model->findID (branchID));	    

	if (bi)
	{
            QString h = spaceKey + ": " + jsobj["title"].toString();
            model->setHeading(h, bi);
        } else
            qWarning() << "CA::pageDetailsReceived couldn't find branch " << branchID;
    }
    else
        qWarning() << "CA::pageDetailsReceived couldn't find model " << modelID;


    reply->deleteLater();
    reply = nullptr;
}

#ifndef QT_NO_SSL   //FIXME-2 make sure to use SSL!!!
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
