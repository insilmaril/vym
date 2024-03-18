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
    enum JobType {Undefined, GetTicketInfo, Query};

    static bool available();
    JobType jobTypeFromText(const QString& text, QString &query);

    JiraAgent();
    ~JiraAgent();

    void init();
    void setJobType(JobType jt);
    JiraAgent::JobType jobType();
    bool setBranch(BranchItem *bi);
    bool setTicket(const QString &id);
    bool setQuery(const QString &s);
    QString serverName();
    QString url();

    void startJob();

  private:
    void continueJob();
    void finishJob();
    void unknownStepWarning();

  signals:
    void jiraTicketReady(QJsonObject);
    void jiraQueryReady(QJsonObject);

  private:
    void startGetTicketRequest();
    void startQueryRequest();

  private slots:
    void ticketReceived(QNetworkReply *reply);
    void queryFinished(QNetworkReply *reply);
    void timeout();
#ifndef QT_NO_SSL
    void sslErrors(QNetworkReply *, const QList<QSslError> &errors);
#endif

  private:
    // Job related 
    QTimer *killTimer;
    JobType jobTypeInt;
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
    QString keyInt;
    QString queryInt;

    // Backreferences to take action in calling model
    int branchID;
    int modelID;
};
#endif
