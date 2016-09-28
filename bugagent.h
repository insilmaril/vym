#ifndef BUGAGENT_H
#define BUGAGENT_H

#include <QHash>
#include <QObject>

#include "vymprocess.h"

class BranchItem;
class VymModel;

class BugAgent:public QObject
{
    Q_OBJECT

enum MissionType {SingleBug,Query};

public:	
    BugAgent (BranchItem *bi,const QString &bug);
    ~BugAgent();

public slots:
    virtual void processFinished(int exitCode, QProcess::ExitStatus exitStatus);

protected:
    virtual void processBugzillaData ();
    virtual void setModelBugzillaData (VymModel *model, BranchItem *bi,const QString &bugID);

private:
    MissionType missionType;
    uint branchID;
    uint modelID;
    QString url;
    QString bugID;
    QString bugScript;
    QStringList result;
    VymProcess *p;

    QHash <QString,QString> bug_desc;
    QHash <QString,QString> bug_prio;
    QHash <QString,QString> bug_sev;
    QHash <QString,QString> bug_deltats;
    QHash <QString,QString> bug_status;
    QHash <QString,QString> bug_whiteboard;

};
#endif

