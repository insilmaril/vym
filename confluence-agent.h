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
    ConfluenceAgent ();
    ~ConfluenceAgent();
    void test();
    bool getPageDetails(const QString &url);
private:
    bool uploadContent(const QString &url, const QString &title, const QString &fpath, const bool &newPage);
public:
    bool updatePage(const QString &url, const QString &title, const QString &fpath);
    bool createPage(const QString &url, const QString &title, const QString &fpath);
    bool getUsers(const QString &name);
    void waitForResult();
    bool success();
    QString getResult();

public slots:
    virtual bool dataReceived(int exitCode, QProcess::ExitStatus exitStatus);
    virtual void timeout();

private:
    QString confluenceScript;
    VymProcess *vymProcess;
    QTimer *killTimer;
    bool succ;
    QString result;
};
#endif

