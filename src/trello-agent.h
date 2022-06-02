#ifndef TRELLOAGENT_H
#define TRELLOAGENT_H

#include <QHash>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QTimer>

class BranchItem;
class VymModel;

/* FIXME-2 not needed for now
class TrelloBoard {
  public:
    TrelloBoard();  
  private:
    QString name;
    QString url;
    QString id;
}
*/

class TrelloAgent : public QObject {
    Q_OBJECT

  public:
    enum JobType {Undefined, GetMyBoards, GetBoardInfo, SyncBoardToBranch};

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
    void startGetMyBoardsRequest();
    void startGetBoardRequest();

  private slots:
    void dataReceived(QNetworkReply *reply);
    void timeout();
#ifndef QT_NO_SSL
    void sslErrors(QNetworkReply *, const QList<QSslError> &errors);
#endif
    void updateListsOfBranch();

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
    QString auth;

    // Settings: Where to find trello
    QString apiURL;

    // Backreferences to take action in calling model
    int branchID;
    int modelID;

    // References to trello
    QString boardID;
};

#endif
