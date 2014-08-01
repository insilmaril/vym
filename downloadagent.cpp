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
#include "settings.h"
#include "vymmodel.h"

extern Main *mainWindow;
extern QString vymVersion;
extern Settings settings;
extern bool debug;

DownloadAgent::DownloadAgent(const QUrl &u, const QString &d)
{
    finishedScriptModelID = 0;
    url = u;
    setDestination(d);
    connect(&agent, SIGNAL(finished(QNetworkReply*)),
            SLOT(requestFinished(QNetworkReply*)));

    QString os;
#if defined(Q_OS_MACX)
    os = "Mac"
#elif defined(Q_OS_WIN32)
    os = "Win32";
#elif defined(Q_OS_LINUX)
    os = "Linux";
#else
    os = "Unknown";
#endif
    userAgent = QString("vym (%1 %2)").arg(os).arg(vymVersion).toUtf8();
}

void DownloadAgent::setDestination(const QString &dest)
{
    destination = dest;
}

QString  DownloadAgent::getDestination()
{
    return destination;
}

bool DownloadAgent::isSuccess()
{
    return success;
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

    QByteArray cookievalue=settings.value("/cookies/id/id",QByteArray() ).toByteArray();
    if (!cookievalue.size() == 0 )
    {
        QNetworkCookie cookie;
        cookie.setPath("/");
        //cookie.setDomain("localhost");
        cookie.setName("id");
        cookie.setValue(cookievalue);
        cookie.setExpirationDate( settings.value("/cookies/id/expires", QVariant(QDateTime::currentDateTime().addSecs(60) )).toDateTime() ); //FIXME-0 expiration time
        agent.cookieJar()->insertCookie(cookie);
    }

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

bool DownloadAgent::saveToDisk(const QString &filename, const QString &data)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        fprintf(stderr, "Could not open %s for writing: %s\n",
                qPrintable(filename),
                qPrintable(file.errorString()));
        return false;
    }

    file.write(data.toLatin1() );
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
        success = false;
        resultMessage = reply->errorString();
        emit ( downloadFinished());
    }
    else {
        success = true;
        if (destination.isEmpty()) destination = makeFileName(url);
        resultMessage = QString ("saved to %1").arg(destination);
        
        if (debug) qDebug()<<"\n* DownloadAgent::reqFinished: ";
        QList <QNetworkCookie> cookies =  reply->manager()->cookieJar()->cookiesForUrl(url);
        foreach (QNetworkCookie c, cookies)
        {
            if (debug)
            {
                qDebug() << "           url: " << url.toString();
                qDebug() << "   cookie name: " << c.name();
                qDebug() << "   cookie path: " << c.path();
                qDebug() << "  cookie value: " << c.value();
                qDebug() << " cookie domain: " << c.domain();
                qDebug() << " cookie exdate: " << c.expirationDate().toLocalTime().toString();
            }

            if (c.name() == "id" ) 
            {
                settings.setValue( "/cookies/id/id", c.value());
                settings.setValue( "/cookies/id/expires", c.expirationDate());
            }
        }

        QString data = reply->readAll();
        if (saveToDisk(destination, data))
            emit ( downloadFinished());
    }

    currentDownloads.removeAll(reply);
    reply->deleteLater();
}

