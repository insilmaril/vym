#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSslError>
#include <QStringList>
#include <QTimer>
#include <QUrl>

#include <stdio.h>

QT_BEGIN_NAMESPACE
class QSslError;
QT_END_NAMESPACE

QT_USE_NAMESPACE

#include "downloadagent.h"
#include "mainwindow.h"
#include "vymmodel.h"

extern Main *mainWindow;

DownloadAgent::DownloadAgent(const QUrl &u, const QString &d)
{
    finishedScriptModelID = 0;
    url = u;
    destination = d;
    connect(&agent, SIGNAL(finished(QNetworkReply*)),
            SLOT(requestFinished(QNetworkReply*)));
}

QString  DownloadAgent::getDestination()
{
    return destination;
}

bool DownloadAgent::getResult()
{
    return result;
}

QString DownloadAgent::getResultMessage()
{
    return resultMessage;
}

void DownloadAgent::setFinishedAction (VymModel *m, const QString &script)
{
    finishedScriptModelID = m->getModelID();
    finishedScript = script;
}

uint DownloadAgent::getFinishedScriptModelID()
{
    return finishedScriptModelID;
}

QString DownloadAgent::getFinishedScript ()
{
    return finishedScript;
}

void DownloadAgent::setUserAgent(const QString &s)
{
    userAgent = s.toLocal8Bit();
}

void DownloadAgent::doDownload(const QUrl &url)
{
    QNetworkRequest request(url);
    if (!userAgent.isEmpty()) request.setRawHeader("User-Agent", userAgent);

    QNetworkReply *reply = agent.get(request);
    connect(reply, SIGNAL(sslErrors(QList<QSslError>)), SLOT(sslErrors(QList<QSslError>)));

    currentDownloads.append(reply);
}

QString DownloadAgent::makeFileName(const QUrl &url)
{
    QString path = url.path();
    QString basename = QFileInfo(path).fileName();
    QString tmp = "xyz";    // FIXME-0 create tmpfile

    return tmp + "-" + basename;
}

bool DownloadAgent::saveToDisk(const QString &filename, QIODevice *data)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        fprintf(stderr, "Could not open %s for writing: %s\n",
                qPrintable(filename),
                qPrintable(file.errorString()));
        return false;
    }

    file.write(data->readAll());
    file.close();

    return true;
}

void DownloadAgent::execute()
{
    doDownload(url);
}

void DownloadAgent::sslErrors(const QList<QSslError> &sslErrors)
{
#ifndef QT_NO_OPENSSL
    foreach (const QSslError &error, sslErrors)
        fprintf(stderr, "SSL error: %s\n", qPrintable(error.errorString()));
#endif
}

void DownloadAgent::requestFinished(QNetworkReply *reply)
{
    QUrl url = reply->url();
    if (reply->error()) 
    {
        result = false;
        resultMessage = reply->errorString();
        emit ( downloadFinished());
    }
    else {
        result = true;
        if (destination.isEmpty()) destination = makeFileName(url);
        resultMessage = QString ("saved to %1").arg(destination);
        if (saveToDisk(destination, reply))
            emit ( downloadFinished());
    }

    currentDownloads.removeAll(reply);
    reply->deleteLater();
}

