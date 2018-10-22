#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSslError>
#include <QStringList>
#include <QTemporaryFile>
#include <QTimer>
#include <QUrl>

#include <stdio.h>

QT_BEGIN_NAMESPACE
class QSslError;
QT_END_NAMESPACE

QT_USE_NAMESPACE

class VymModel;

class DownloadAgent: public QObject
{
    Q_OBJECT
    QNetworkAccessManager agent;
    QList<QNetworkReply *> currentDownloads;

public:
    DownloadAgent(const QUrl &u);
    ~DownloadAgent();
    QString getDestination ();
    void setFinishedAction (VymModel *m, const QString &script);
    QString getFinishedScript();
    uint getFinishedScriptModelID();
    void setUserAgent(const QString &s);
    bool  isSuccess();
    QString getResultMessage();
    void doDownload(const QUrl &url);
    bool saveToDisk(const QString &filename, const QByteArray &data);

public slots:
    void execute();
    void requestFinished(QNetworkReply *reply);
    void sslErrors(const QList<QSslError> &errors);

signals:
    void downloadFinished();

private:
    QString tmpFilePath;
    QByteArray userAgent;
    QUrl url;

    bool success;
    QString resultMessage;

    QString finishedScript;
    uint finishedScriptModelID;
};

