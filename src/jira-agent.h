#ifndef JIRAAGENT_H
#define JIRAAGENT_H

#include <QHash>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QTimer>

#include "heading.h"
#include "vymprocess.h"

class BranchItem;
class VymModel;

class JiraAgent : public QObject {
    Q_OBJECT

  public:
    enum JobType {Undefined, GetTicketInfo};

    JiraAgent();
    ~JiraAgent();

    void init();
    void setJobType(JobType jt);
    bool setBranch(BranchItem *bi);
    bool setTicket(const QString &id);
    QString getURL();

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

  protected:
    virtual void setModelJiraData(VymModel *model, BranchItem *bi,
                                  const QString &ticketID);
    virtual void undoUpdateMessage(BranchItem *bi = NULL);

  private:
    // Job related 
    QTimer *killTimer;
    JobType jobType;
    int jobStep;
    bool abortJob;  // Flag to abort during initialization of job

    // Network handling
    QNetworkAccessManager *networkManager;
    QJsonObject jsobj;
    bool httpRequestAborted;

    // Settings: Credentials to access JIRA
    QString username;
    QString password;

    // Settings: Where to find JIRA and which ticket
    QString baseURL;
    QString apiURL;
    QString ticketID;
    QString ticketURL;

    // Backreferences to take action in calling model
    uint branchID;
    uint modelID;

    ////  Old stuff  FIXME-0

    QString url;
    Heading oldHeading;

    QHash<QString, QString> ticket_desc;
    QHash<QString, QString> ticket_type;
    QHash<QString, QString> ticket_prio;
    QHash<QString, QString> ticket_status;
    QHash<QString, QString> ticket_resolution;
    QHash<QString, QString> ticket_updated;
    QHash<QString, QString> ticket_created;
    QHash<QString, QString> ticket_assignee;
    QHash<QString, QString> ticket_reporter;
    QHash<QString, QString> ticket_url;
};
#endif
