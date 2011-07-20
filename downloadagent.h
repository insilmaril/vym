#ifndef DOWNLOADAGENT_H
#define DOWNLOADAGENT_H

#include <QObject>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class BranchItem;
class VymModel;

class DownloadAgent:public QObject
{
    Q_OBJECT

public:	
    DownloadAgent (const QUrl &url, BranchItem *bi);
    ~DownloadAgent();

public slots:
    virtual void downloadFinished   (QNetworkReply *reply);

private:
    uint branchID;
    uint modelID;

    QNetworkAccessManager networkManager;
    QList <QNetworkReply*> currentDownloads;
};
#endif

