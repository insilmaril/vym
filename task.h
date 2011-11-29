#ifndef TASK_H
#define TASK_H

#include <QString>

class BranchItem;
class QString;
class TaskModel;

class Task{
public:
    enum Status {NotStarted, WIP, Finished};

    Task(TaskModel* tm);
    ~Task();
    void setModel (TaskModel* tm);
    void cycleStatus();
    void setStatus(Status ts);
    Status getStatus();	
    QString getStatusString();
    void setPriority(QChar p);
    QChar getPriority();
    QString getName();
    void setBranch (BranchItem *bi);
    BranchItem* getBranch();

private:
    TaskModel* model;
    Status status; 
    QChar prio;
    BranchItem *branch;
};

#endif
