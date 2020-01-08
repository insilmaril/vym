#ifndef CONFLUENCEAGENT_H
#define CONFLUENCEAGENT_H

#include <QHash>
#include <QObject>
#include <QTimer>

#include "heading.h"
#include "vymprocess.h"

class BranchItem;
class VymModel;

class ConfluenceAgent:public QObject
{
    Q_OBJECT

public:	
    ConfluenceAgent (VymModel *m);
    ~ConfluenceAgent();
    void test();
    bool getPageDetails(const QString &url);
private:
    bool uploadContent(const QString &url, const QString &title, const QString &fpath, const bool &newPage);
public:
    bool updatePage(const QString &url, const QString &title, const QString &fpath);
    bool createPage(const QString &url, const QString &title, const QString &fpath);
    void waitForResult();
    bool success();
    QString getResult();

public slots:
    virtual bool pageDetailsReceived(int exitCode, QProcess::ExitStatus exitStatus);
    virtual void timeout();

private:
    uint modelID;
    QString confluenceScript;
    VymProcess *vymProcess;
    QTimer *killTimer;
    bool succ;
    QString result;
};
#endif

