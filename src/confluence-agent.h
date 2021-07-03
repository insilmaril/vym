#ifndef CONFLUENCEAGENT_H
#define CONFLUENCEAGENT_H

#include <QHash>
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
    enum JobType {Undefined, CopyPagenameToHeading};

    ConfluenceAgent();
    ConfluenceAgent(BranchItem *bi);
    ~ConfluenceAgent();
    void init();
    void setJobType(JobType jt);
    void startJob();
    void finishJob();
    void setPageURL(QString u);
    void test();
    bool getPageDetails(const QString &url);

  private:
    bool uploadContent(const QString &url, const QString &title,
                       const QString &fpath, const bool &newPage);

  public:
    bool updatePage(const QString &url, const QString &title,
                    const QString &fpath);
    bool createPage(const QString &url, const QString &title,
                    const QString &fpath);
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

    // REST access related, new
  public:
    void startGetPageSourceRequest(QUrl requestedUrl);
    void startGetPageDetailsRequest(QString query);

  private:  
    bool getPageSource();

  private slots:
    void pageSourceReceived();
    void pageDetailsReceived();
#ifndef QT_NO_SSL
    void sslErrors(QNetworkReply *, const QList<QSslError> &errors);
#endif

  private:
    QNetworkAccessManager qnam;
    QNetworkReply *reply;
    bool httpRequestAborted;

    QString username;
    QString password;

    uint branchID;
    uint modelID;

    QString baseURL;
    QString apiURL;

    QString pageURL;
    QString pageID;
    QString spaceKey;
};
#endif
