#ifndef CONFLUENCEAGENT_H
#define CONFLUENCEAGENT_H

#include <QHash>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>
#include <QTimer>

#include "confluence-user.h"

class BranchItem;
class VymModel;

//////////////////////////////////////////////////////////////////////////

class ConfluenceAgent : public QObject {
    Q_OBJECT

  public:
    enum JobType {
        Undefined,
        CopyPagenameToHeading,
        CreatePage,
        UpdatePage,
        UploadAttachment,
        GetUserInfo
    };

    static bool available();

    ConfluenceAgent();
    ConfluenceAgent(BranchItem *bi);
    ~ConfluenceAgent();
    void init();
    void setJobType(JobType jt);
    void setBranch(BranchItem *bi);
    void setModelID(uint id);
    void setPageURL(const QString &u);
    void setNewPageName(const QString &t);
    void setUploadFilePath(const QString &fp);

    void startJob();

  private:
    void continueJob();
    void finishJob();
    void unknownStepWarning();

  signals:
    void foundUsers(QList <ConfluenceUser>);

  public:
    void getUsers(const QString &name); //! Convenience function to get user data

  private:  
    void startGetPageSourceRequest(QUrl requestedUrl);
    void startGetPageDetailsRequest();
    void startCreatePageRequest();
    void startUpdatePageRequest();
    void startGetUserInfoRequest();
    void startGetAttachmentsInfoRequest();
    void startCreateAttachmentRequest();
    void startUpdateAttachmentRequest();
    QNetworkRequest createRequest(const QUrl &url);
    bool wasRequestSuccessful(
            QNetworkReply *reply, 
            const QString &requestDesc,
            const QByteArray &fullReply);

  private slots:
    void pageSourceReceived(QNetworkReply *reply);
    void pageDetailsReceived(QNetworkReply *reply);
    void pageUploaded(QNetworkReply *reply);
    void userInfoReceived(QNetworkReply *reply);
    void attachmentsInfoReceived(QNetworkReply *reply);
    void attachmentCreated(QNetworkReply *reply);
    void attachmentUpdated(QNetworkReply *reply);
    void timeout();

#ifndef QT_NO_SSL
    void sslErrors(QNetworkReply *, const QList<QSslError> &errors);
#endif

  private:
    // Job related 
    QTimer *killTimer;
    JobType jobType;
    int jobStep;
    bool abortJob;  // Flag to abort during initialization of job

    // Network handling
    QNetworkAccessManager *networkManager;
    QJsonObject pageObj;
    QJsonObject attachmentObj;

    // Settings: Credentials to access Confluence
    bool authUsingPAT;
    QString personalAccessToken;
    QString username;
    QString password;

    // Settings: Where to find Confluence
    QString baseURL;
    QString apiURL;

    // Backreferences to take action in calling model
    uint branchID;
    uint modelID;

    // Export settings
  public:
    bool exportImage;

  private:
    // Parameters
    QString pageURL;
    QString newPageName;
    QString uploadFilePath;
    QString userQuery;

    // Page details received from Confluence
    QString pageID;
    QString spaceKey;

    // Attachments
    QStringList attachmentsTitles;
    QStringList attachmentsIds;
    QString uploadAttachmentName;
    QString uploadAttachmentId;

    // User info received from Confluence
    QList <ConfluenceUser> userList;
};
#endif
