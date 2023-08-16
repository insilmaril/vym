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

  private: QNetworkRequest createRequest(const QUrl &url);
  private: void startGetPageSourceRequest(QUrl requestedUrl);
  private slots: void pageSourceReceived(QNetworkReply *reply);

  private: void startGetPageDetailsRequest();
  private slots: void pageDetailsReceived(QNetworkReply *reply);

  private: void startCreatePageRequest();
  private: void startUpdatePageRequest();
  private slots: void pageUploaded(QNetworkReply *reply);


  private: void startGetUserInfoRequest();
  private slots: void userInfoReceived(QNetworkReply *reply);

  private: void startGetAttachmentsInfoRequest();
  private slots: void attachmentsInfoReceived(QNetworkReply *reply);

  private: void startCreateAttachmentRequest();
  private slots: void attachmentCreated(QNetworkReply *reply);

  private: void startUpdateAttachmentRequest();
  private slots: void attachmentUpdated(QNetworkReply *reply);

  private: bool wasRequestSuccessful(
            QNetworkReply *reply, 
            const QString &requestDesc,
            const QByteArray &fullReply);


  private slots: void timeout();

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
