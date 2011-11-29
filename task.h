#ifndef TASK_H
#define TASK_H

#include <QString>

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
    QString getName();
    void setBranch (BranchItem *bi);
    BranchItem* getBranch();
    QString saveToDir();

private:
    TaskModel* model;
    Status status; 
    int prio;
    BranchItem *branch;
};

#endif
