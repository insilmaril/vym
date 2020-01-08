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
    bool updatePage(const QString &url, const QString &title, const QString &fpath);
    void waitForResult();
    bool success();

public slots:
    virtual void pageDetailsReceived(int exitCode, QProcess::ExitStatus exitStatus);
    virtual void timeout();

private:
    uint modelID;
    QString confluenceScript;
    VymProcess *vymProcess;
    QTimer *killTimer;
    bool succ;
};
#endif

