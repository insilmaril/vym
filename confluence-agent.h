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

public slots:
    virtual void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
    virtual void timeout();

private:
    void processData ();

private:
    uint branchID;
    uint modelID;
    QString confluenceScript;
    VymProcess *p;
    QTimer *killTimer;
};
#endif

