#include "task.h"

#include <QDebug>

#include "branchitem.h"
#include "taskmodel.h"
#include "vymmodel.h"

Task::Task(TaskModel *tm)
{
    //    qDebug()<<"Constr. Task";
    status = NotStarted;
    awake = Task::WideAwake;
    branch = nullptr;
    prio = 0;
    prio_delta = 0;
    model = tm;
    date_creation = QDateTime::currentDateTime();
}

Task::~Task()
{
    //    qDebug()<<"Destr. Task";
    if (branch)
        branch->setTask(nullptr);
}

void Task::setModel(TaskModel *tm) { model = tm; }

void Task::cycleStatus(bool reverse)
{
    if (awake == Morning)
        setAwake(WideAwake);
    else {
        int i = status;
        reverse ? i-- : i++;

        if (i < 0)
            i = 2;
        if (i > 2)
            i = 0;

        setStatus((Task::Status)i);
    }
    if (branch)
        branch->updateTaskFlag();
}

void Task::setStatus(const QString &s)
{
    if (s == "NotStarted")
        setStatus(NotStarted);
    else if (s == "WIP")
        setStatus(WIP);
    else if (s == "Finished")
        setStatus(Finished);
    else
        qWarning() << "Task::setStatus Unknown value: " << s;
}

void Task::setStatus(Status s)
{
    if (s == status)
        return;
    status = s;
    if (branch)
        branch->updateTaskFlag();
}

Task::Status Task::getStatus() { return status; }

QString Task::getStatusString()
{
    switch (status) {
    case NotStarted:
        return "NotStarted";
    case WIP:
        return "WIP";
    case Finished:
        return "Finished";
    }
    return "Undefined";
}

QString Task::getIconString()
{
    QString s;
    switch (status) {
    case NotStarted:
        s = "task-new";
        break;
    case WIP:
        s = "task-wip";
        break;
    case Finished:
        s = "task-finished";
        break;
    default:
        s = "status:undefined";
    }
    if (status != Finished)
        switch (awake) {
        case Sleeping:
            s += "-sleeping";
            break;
        case Morning:
            s += "-morning";
            break;
        default:
            break;
        }
    return s;
}

void Task::setAwake(const QString &s)
{
    if (s == "Sleeping")
        setAwake(Sleeping);
    else if (s == "Morning")
        setAwake(Morning);
    else if (s == "WideAwake")
        setAwake(WideAwake);
    else
        qWarning() << "Task::setAwake Unknown value: " << s;
}

void Task::setAwake(Task::Awake a)
{
    if (awake != a) {
        awake = a;
        if (branch)
            branch->updateTaskFlag();
    }
}

Task::Awake Task::getAwake() { return awake; }

QString Task::getAwakeString()
{
    switch (getAwake()) {
    case Sleeping:
        return "Sleeping";
    case Morning:
        return "Morning";
    case WideAwake:
        return "WideAwake";
    }
    return "Undefined";
}

bool Task::updateAwake()
{
    qint64 secs = getSecsSleep();

    if (secs < 0) {
        if (awake == Task::Sleeping) {
            setAwake(Task::Morning);
            return true;
        }
    }
    else if (secs > 0) {
        if (awake != Task::Sleeping) {
            setAwake(Task::Sleeping);
            return true;
        }
    }
    return false;
}

void Task::setPriority(int p) { prio = p; }

int Task::getPriority() { return prio; }

int Task::getAgeCreation()
{
    return date_creation.daysTo(QDateTime::currentDateTime());
}

int Task::getAgeModification()
{
    if (date_modification.isValid())
        return date_modification.daysTo(QDateTime::currentDateTime());
    else
        return getAgeCreation();
}

void Task::setDateCreation(const QString &s)
{
    date_creation = QDateTime().fromString(s, Qt::ISODate);
}

QDateTime Task::getDateCreation() { return date_creation; }

void Task::setDateModification()
{
    date_modification = QDateTime::currentDateTime();
}

void Task::setDateModification(const QString &s)
{
    date_modification = QDateTime().fromString(s, Qt::ISODate);
}

QDateTime Task::getDateModification() { return date_modification; }

bool Task::setDaysSleep(qint64 n)
{
    return setDateSleep(QDate::currentDate().addDays(n).toString(Qt::ISODate));
}

bool Task::setHoursSleep(qint64 n)
{
    return setDateSleep(
        QDateTime::currentDateTime().addSecs(n * 3600).toString(Qt::ISODate));
}

bool Task::setSecsSleep(qint64 n)
{
    if (n == 0)
        setAwake(Morning);
    return setDateSleep(
        QDateTime::currentDateTime().addSecs(n).toString(Qt::ISODate));
}

bool Task::setDateSleep(const QString &s)
{
    if (setDateSleep(QDateTime().fromString(s, Qt::ISODate)))
        return true;
    else if (setDateSleep(QDateTime().fromString(s, Qt::TextDate)))
        return true;
    else if (setDateSleep(QDateTime().fromString(s)))
        return true;
    else
        return false;
}

bool Task::setDateSleep(const QDateTime &d)
{
    if (!d.isValid())
        return false;

    date_sleep = d;
    updateAwake();
    return true;
}

qint64 Task::getDaysSleep()
{
    qint64 d = 1;
    if (date_sleep.isValid())
        d = QDateTime::currentDateTime().daysTo(date_sleep);
    else {
        // qWarning() << "Task::getDaysSleep date_sleep is invalid for branch "
        // << branch->headingPlain();
        return -1;
    }
    return d;
}

qint64 Task::getSecsSleep()
{
    qint64 d = 0; // Meaning: No sleep time set so far
    if (date_sleep.isValid())
        d = QDateTime::currentDateTime().secsTo(date_sleep);
    return d;
}

QDateTime Task::getSleep() { return date_sleep; }

void Task::setPriorityDelta(const int &n) { prio_delta = n; }

int Task::getPriorityDelta() { return prio_delta; }

void Task::setBranch(BranchItem *bi)
{
    branch = bi;
    mapName = bi->getModel()->getMapName();
}

BranchItem *Task::getBranch() { return branch; }

QString Task::getName()
{
    if (branch)
        return branch->headingPlain();
    else {
        qWarning() << "Task::getName  no branch!";
        return "UNDEFINED";
    }
}

QString Task::getMapName() { return mapName; }

QString Task::saveToDir()
{
    QString sleepAttr;
    if (date_sleep.isValid())
        sleepAttr = attribute("date_sleep", date_sleep.toString(Qt::ISODate));
    else
        sleepAttr = attribute("date_sleep", "2018-01-01T00:00:00");

    // Experimental: Also output priority based on arrow flags for external
    // sorting
    QString prioAttr;
    if (branch) {
        if (branch->hasActiveFlag("2arrow-up"))
            prioAttr = attribute("prio", "2");
        if (branch->hasActiveFlag("arrow-up"))
            prioAttr = attribute("prio", "1");
    }

    QString prioDeltaAttr;
    if (prio_delta != 0)
        prioDeltaAttr = attribute("prio_delta", QString("%1").arg(prio_delta));
    return singleElement(
        "task",
        attribute("status", getStatusString()) +
            attribute("awake", getAwakeString()) +
            attribute("date_creation", date_creation.toString(Qt::ISODate)) +
            attribute("date_modification",
                     date_modification.toString(Qt::ISODate)) +
            prioDeltaAttr + sleepAttr + prioAttr);
}
