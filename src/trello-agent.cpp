#include "trello-agent.h"

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

bool TrelloAgent::available()
{
    if (!QSslSocket::supportsSsl())
        return false;
    if ( settings.value("/atlassian/trello/token", "").toString().isEmpty())
        return false;
    if ( settings.value("/atlassian/trello/apiKey", "").toString().isEmpty())
        return false;

    return true;
}

TrelloAgent::TrelloAgent()
{
    qDebug ()<< "Constr. TrelloAgent";

    init();
}

TrelloAgent::~TrelloAgent()
{
    qDebug() << "Destr. TrelloAgent";

    if (killTimer)
        delete killTimer;
}

void TrelloAgent::init()
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
    apiKey =
        settings.value("/atlassian/trello/apiKey", "undefined").toString();
    token = settings.value("/atlassian/trello/token", "undefined").toString();

    // Set API rest point
    apiURL = "https://api.trello.com/";
}

void TrelloAgent::setJobType(JobType jt)
{
    jobType = jt;
}

bool TrelloAgent::setBranch(BranchItem *bi)
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

void TrelloAgent::startJob()
{
    if (jobStep > 0) {
        unknownStepWarning();
        finishJob();
    } else {
        jobStep = 0;
        continueJob();
    }
}

void TrelloAgent::continueJob()
{
    if (abortJob) {
        finishJob();
        return;
    }

    jobStep++;

    switch(jobType) {
        case GetBoardInfo:
            switch(jobStep) {
                case 1:
                    // if (!requestedURL.toString().startsWith("http"))
                    //    requestedURL.setPath("https://" + requestedURL.path());
                    startGetBoardRequest();
                    break;
                case 2: {
                    // Insert references to original branch and model
                    // FIXME-2 not needed jsobj["vymModelID"] = QString::number(modelID);
                    // FIXME-0 jsobj["vymBranchID"] = QJsonValue(branchID);
                    
                    emit (trelloBoardDataReady(jsdoc));
                    finishJob();
                    }
                    break;
                default:
                    unknownStepWarning();
                    break;
            };
            break;
        default:
            qWarning() << "TrelloAgent::continueJob   unknown jobType " << jobType;
    }
}

void TrelloAgent::finishJob()
{
    deleteLater();
}

void TrelloAgent::unknownStepWarning()
{
    qWarning() << "TrelloAgent::contJob  unknow step in jobType = " 
        << jobType 
        << "jobStep = " << jobStep;
}

void TrelloAgent::startGetBoardRequest()
{
    // //FIXME-2 boardID still hardcoded:
    QString boardID = "627a201d7d74b04fba065a07";
    QString auth = QString("?key=%1&token=%2").arg(apiKey).arg(token);
    QUrl url = QUrl(apiURL + "1/boards/" + boardID + "/lists" + auth);

    QNetworkRequest request = QNetworkRequest(url);

    if (debug)
    {
        qDebug() << "TA::startGetBoardRequest: url = " + request.url().toString();
    }

    killTimer->start();

    connect(networkManager, &QNetworkAccessManager::finished,
        this, &TrelloAgent::dataReceived);

    networkManager->get(request);
}

void TrelloAgent::dataReceived(QNetworkReply *reply)
{
    killTimer->stop();

    networkManager->disconnect();

    QString r = reply->readAll();

    if (reply->error()) {
        if (reply->error() == QNetworkReply::AuthenticationRequiredError)
            QMessageBox::warning(
                nullptr, tr("Warning"),
                tr("Authentication problem when contacting trello"));

        qWarning() << "TrelloAgent::ticketRReveived reply error";
        qWarning() << "Error: " << reply->error();
        vout << "reply: " << endl << r << endl;
        finishJob();
        return;
    }

    jsdoc = QJsonDocument::fromJson(r.toUtf8());
    continueJob();
}

void TrelloAgent::timeout() 
{
    qWarning() << "TrelloAgent timeout!!   jobType = " << jobType;
}

#ifndef QT_NO_SSL
void TrelloAgent::sslErrors(QNetworkReply *reply, const QList<QSslError> &errors)
{
    QString errorString;
    foreach (const QSslError &error, errors) {
        if (!errorString.isEmpty())
            errorString += '\n';
        errorString += error.errorString();
    }

    reply->ignoreSslErrors();
    qWarning() << "TrelloAgent: One or more SSL errors has occurred: " << errorString;
    qWarning() << "Errors ignored.";
}
#endif
