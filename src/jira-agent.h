#ifndef JIRAAGENT_H
#define JIRAAGENT_H

#include <QHash>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QTimer>

class BranchItem;
class VymModel;

class JiraAgent : public QObject {
    Q_OBJECT

  public:
    enum JobType {Undefined, GetTicketInfo};

    static bool available();

    JiraAgent();
    ~JiraAgent();

    void init();
    void setJobType(JobType jt);
    bool setBranch(BranchItem *bi);
    bool setTicket(const QString &id);
    QString serverName();
    QString url();

    void startJob();

  private:
    void continueJob();
    void finishJob();
    void unknownStepWarning();

  signals:
    void jiraTicketReady(QJsonObject);

  private:
    void startGetTicketRequest();

  private slots:
    void ticketReceived(QNetworkReply *reply);
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
    QJsonObject jsobj;

    // Settings: Credentials to access JIRA
    bool authUsingPATInt;
    QString personalAccessTokenInt;
    QString userNameInt;
    QString passwordInt;

    // Settings: Where to find JIRA and which ticket
    QString baseUrlInt;
    QString serverNameInt;
    QString apiUrl;
    QString ticketUrl;
    QString ticketID;

    // Backreferences to take action in calling model
    int branchID;
    int modelID;
};
#endif
