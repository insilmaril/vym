#include "downloadagent.h"

#include "branchitem.h"
#include "mainwindow.h"
#include "vymmodel.h"

extern Main *mainWindow;
extern QDir vymBaseDir;
extern bool debug;


DownloadAgent::DownloadAgent (const QUrl &url, BranchItem *bi)
{
    if (!bi) 
    {
	qWarning ("Const DownloadAgent: bi==NULL");
	delete (this);
	return;
    }
    branchID=bi->getID();
    VymModel *model=bi->getModel();
    modelID=model->getID();

   //qDebug()<<"Constr. DownloadAgent for "<<branchID;

   connect(&networkManager, SIGNAL(finished(QNetworkReply*)),
            SLOT(downloadFinished(QNetworkReply*)));


    QNetworkRequest request(url);
    QNetworkReply *reply = networkManager.get(request);

    currentDownloads.append(reply);
}

DownloadAgent::~DownloadAgent ()
{
    //qDebug()<<"Destr. DownloadAgent for "<<branchID;
}

void DownloadAgent::downloadFinished (QNetworkReply *reply)
{
    qDebug()<<"DownloadAgent::downloadFinished";
    QUrl url = reply->url();
    if (reply->error()) 
    {
	fprintf(stderr, "VymModel: Download of %s failed: %s\n", url.toEncoded().constData(),
	qPrintable(reply->errorString()));
    } else 
    {
	QByteArray a=reply->readAll();
	VymModel *model=mainWindow->getModel (modelID);
	if (model)
	{
	    BranchItem *dst=(BranchItem*)(model->findID (branchID));	    
	    if (dst)
	    {
		ImageItem *ii=model->createImage(dst);
		if (ii)
		{
		    QImage i;
		    if (!i.loadFromData (a))
			fprintf(stderr, "VymModel: Adding of %s failed.\n", url.toEncoded().constData());
		    else
		    {
			ii->load (i);
			model->reposition();
		    }
		}
	    }

	}
    }

    currentDownloads.removeAll(reply);
    reply->deleteLater();

    deleteLater();
}



