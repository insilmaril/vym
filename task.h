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
    bool updateAwake();
public:
    void setPriority(int  p);
    int getPriority();
    int getAgeCreation();
    int getAgeModification();
    void setDateCreation (const QString &s);
    QDateTime getDateCreation ();
    void setDateModification();
    void setDateModification(const QString &s);
    QDateTime getDateModification ();
    bool setDaysSleep    (qint64 n);
    bool setHoursSleep   (qint64 n);
    bool setSecsSleep    (qint64 n);
    bool setDateSleep    (const QString &s);
    bool setDateSleep    (const QDateTime &d);
    qint64 getDaysSleep();
    qint64 getSecsSleep();
    QDateTime getSleep();
    QString getName();
    void setPriorityDelta( const int &n);
    int getPriorityDelta();
    void setBranch (BranchItem *bi);
    BranchItem* getBranch();
    QString getMapName();
    QString saveToDir();

private:
    TaskModel* model;
    Status status; 
    Awake awake;
    int prio;
    int prio_delta;
    BranchItem *branch;
    QString mapName;
    QDateTime date_creation;
    QDateTime date_modification;
    QDateTime date_sleep;
};

#endif
