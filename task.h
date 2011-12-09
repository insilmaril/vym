#ifndef TASK_H
#define TASK_H

#include <QString>

#include <QDateTime>
#include "xmlobj.h"

class BranchItem;
class QString;
class TaskModel;

class Task:public XMLObj {
public:
    enum Status {Finished,WIP,NotStarted};

    Task(TaskModel* tm);
    ~Task();
    void setModel (TaskModel* tm);
    void cycleStatus();
    void setStatus(const QString &s);
    void setStatus(Status ts);
    Status getStatus();	
    QString getStatusString();
    void setPriority(int  p);
    int getPriority();
    int getAgeCreation();
    int getAgeModified();
    void setDateCreation (const QString &s);
    void setDateModified ();
    void setDateModified (const QString &s);
    void setDateSleep    (const QString &s);
    int getDaysSleep();
    QString getName();
    void setBranch (BranchItem *bi);
    BranchItem* getBranch();
    QString saveToDir();

private:
    TaskModel* model;
    Status status; 
    int prio;
    BranchItem *branch;
    QDateTime date_creation;
    QDateTime date_modified;
    QDateTime date_sleep;
};

#endif
