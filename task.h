#ifndef TASK_H
#define TASK_H

#include <QString>

class BranchItem;
class QString;

class Task{
public:
    enum Status {NotStarted, WIP, Finished};

    Task();
    void setStatus(Status ts);
    Status getStatus();	
    QString getStatusString();
    void setPriority(QChar p);
    QChar getPriority();
    QString getName();
    void setBranch (BranchItem *bi);

private:
    Status status; 
    QChar prio;
    BranchItem *branch;
};

#endif
