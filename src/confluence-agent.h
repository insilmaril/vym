#ifndef CONFLUENCEAGENT_H
#define CONFLUENCEAGENT_H

#include <QHash>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>
#include <QTimer>

#include "heading.h"
#include "vymprocess.h"

class BranchItem;
class VymModel;

class ConfluenceAgent : public QObject {
    Q_OBJECT

  public:
    enum JobType {Undefined, CopyPagenameToHeading, NewPage, UpdatePage, UserInfo};

    ConfluenceAgent();
    ConfluenceAgent(BranchItem *bi);
    ~ConfluenceAgent();
    void init();
    void setJobType(JobType jt);
    void setBranch(BranchItem *bi);
    void setModelID(uint id);
    void setPageURL(const QString &u);
    void setNewPageTitle(const QString &t);
    void setUploadFilePath(const QString &fp);
    void test();

    void startJob();

private:
    void continueJob();
    void finishJob();
    void unknownStepWarning();

  public:
    bool getUsers(const QString &name);
    void waitForResult();
    bool success();
    QString getResult();

  public slots:
    virtual void dataReceived(int exitCode, QProcess::ExitStatus exitStatus);
    virtual void timeout();

  private:
    QString confluenceScript;
    VymProcess *vymProcess;
    QTimer *killTimer;
    bool succ;
    QString result;
    JobType jobType;
    int jobStep;
    bool abortJob;  // Flag to abort during initialization of job

    // REST access related, new
  private:  
    void startGetPageSourceRequest(QUrl requestedUrl);
    void startGetPageDetailsRequest();
    void startCreatePageRequest();
    void startUpdatePageRequest();
    void startGetUserInfoRequest();

  private slots:
    void pageSourceReceived(QNetworkReply *reply);
    void pageDetailsReceived(QNetworkReply *reply);
    void contentUploaded(QNetworkReply *reply);
    void userInfoReceived(QNetworkReply *reply);
#ifndef QT_NO_SSL
    void sslErrors(QNetworkReply *, const QList<QSslError> &errors);
#endif

  private:
    // Network handling
    QNetworkAccessManager *networkManager;
    QJsonObject jsobj;
    bool httpRequestAborted;

    // Settings: Credentials to access Confluence
    QString username;
    QString password;

    // Settings: Where to find Confluence
    QString baseURL;
    QString apiURL;

    // Backreferences to take action in calling model
    uint branchID;
    uint modelID;

    // Parameters
    QString pageURL;
    QString newPageTitle;
    QString uploadFilePath;
    QString userQuery;

    // Page details received from Confluence
    QString pageID;
    QString spaceKey;
};
#endif
