#ifndef JIRAAGENT_H
#define JIRAAGENT_H

#include <QHash>
#include <QObject>
#include "heading.h"

#include "vymprocess.h"

class BranchItem;
class VymModel;

class JiraAgent:public QObject
{
    Q_OBJECT

enum MissionType {SingleTicket, Query};

public:	
    JiraAgent (BranchItem *bi,const QString &ticket);
    ~JiraAgent();

public slots:
    virtual void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
    virtual void timeout();

protected:
    virtual void processJiraData ();
    virtual void setModelJiraData (VymModel *model, BranchItem *bi, const QString &ticketID);
    virtual void undoUpdateMessage( BranchItem *bi = NULL);

private:
    MissionType missionType;
    uint branchID;
    uint modelID;
    QString url;
    QString ticketID;
    QString ticketScript;
    QStringList result;
    VymProcess *p;
    Heading oldHeading;

    QHash <QString, QString> ticket_desc;
    QHash <QString, QString> ticket_type;
    QHash <QString, QString> ticket_prio;
    QHash <QString, QString> ticket_status;
    QHash <QString, QString> ticket_resolution;
    QHash <QString, QString> ticket_updated;
    QHash <QString, QString> ticket_created;
    QHash <QString, QString> ticket_assignee;
    QHash <QString, QString> ticket_reporter;
    QHash <QString, QString> ticket_url;

};
#endif

