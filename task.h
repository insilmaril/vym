#ifndef TASK_H
#define TASK_H

#include <QString>

#include <QDateTime>
#include "xmlobj.h"

class BranchItem;
class QString;
class TaskModel;
class VymModel;

class Task:public XMLObj {
public:
    enum Status {NotStarted,WIP,Finished};
    enum Awake {Sleeping,Morning,WideAwake};

    Task(TaskModel* tm);
    ~Task();
    void setModel (TaskModel* tm);
    void cycleStatus(bool reverse=false);
    void setStatus(const QString &s);
    void setStatus(Status ts);
    Status getStatus();	
    QString getStatusString();
    QString getIconString();    //! Used to create icons in task list and flags in mapview
    void setAwake(const QString &s);
    void setAwake(Awake a);
    Awake getAwake();
    QString getAwakeString();
    void recalcAwake();
public:
    void setPriority(int  p);
    int getPriority();
    int getAgeCreation();
    int getAgeModified();
    void setDateCreation (const QString &s);
    void setDateModified ();
    void setDateModified (const QString &s);
    void setDaysSleep    (qint64 n);
    void setHoursSleep   (qint64 n);
    void setSecsSleep    (qint64 n);
    void setDateSleep    (const QString &s);
    qint64 getDaysSleep();
    qint64 getSecsSleep();
    QDateTime getSleep();
    QString getName();
    void setBranch (BranchItem *bi);
    BranchItem* getBranch();
    QString getMapName();
    QString saveToDir();

private:
    TaskModel* model;
    Status status; 
    Awake awake;
    int prio;
    BranchItem *branch;
    QString mapName;
    QDateTime date_creation;
    QDateTime date_modified;
    QDateTime date_sleep;
};

#endif
