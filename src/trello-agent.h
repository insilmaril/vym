#ifndef TRELLOAGENT_H
#define TRELLOAGENT_H

#include <QHash>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QTimer>

class BranchItem;
class VymModel;

class TrelloAgent : public QObject {
    Q_OBJECT

  public:
    enum JobType {Undefined, GetBoardInfo};

    static bool available();

    TrelloAgent();
    ~TrelloAgent();

    void init();
    void setJobType(JobType jt);
    bool setBranch(BranchItem *bi);

    void startJob();

  private:
    void continueJob();
    void finishJob();
    void unknownStepWarning();

  signals:
    void trelloBoardDataReady(QJsonDocument);

  private:
    void startGetBoardRequest();

  private slots:
    void dataReceived(QNetworkReply *reply);
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
    QJsonDocument jsdoc;

    // Settings: Credentials to access Trello
    QString apiKey;
    QString token;

    // Settings: Where to find trello
    QString apiURL;

    // Backreferences to take action in calling model
    int branchID;
    int modelID;
};
#endif
