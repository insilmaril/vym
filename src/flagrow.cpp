#include <QDebug>

#include "flagrow.h"

#include "flagrow-master.h"
#include "mainwindow.h"

extern bool debug;
extern Main *mainWindow;

/////////////////////////////////////////////////////////////////
// FlagRow
/////////////////////////////////////////////////////////////////
FlagRow::FlagRow()
{
    //qDebug()<< "Const FlagRow ()";
    masterRow = nullptr;
}

FlagRow::~FlagRow()
{
    //qDebug()<< "Destr FlagRow   masterRow=" << masterRow;
}

const QStringList FlagRow::activeFlagNames() { return QStringList(); }

const QList<QUuid> FlagRow::activeFlagUids() { return activeUids; }

bool FlagRow::isActive(const QString &name)
{
    Flag *f = masterRow->findFlagByName(name);
    if (!f) {
        qWarning() << "FlagRow::isActive couldn't find flag named " << name;
        return false;
    }

    return isActive(f->getUuid());
}

bool FlagRow::isActive(const QUuid &uid)
{
    QUuid i;
    foreach (i, activeUids)
        if (i == uid)
            return true;
    return false;
}

bool FlagRow::hasFlag(const QString &name)
{
    Flag *f = masterRow->findFlagByName(name);
    if (f)
        return true;
    else
        return false;
}

bool FlagRow::toggle(const QString &name, bool useGroups)
{
    // First get UID from mastRow
    if (!masterRow) {
        qWarning() << "FlagRow::toggle name " << name << " no masterRow";
        return false;
    }

    Flag *flag = masterRow->findFlagByName(name);
    if (!flag) {
        qWarning() << "FlagRow::toggle name " << name
                   << " masterRow has no such flag";
        return false;
    }

    return toggle(flag->getUuid());
}

bool FlagRow::toggle(const QUuid &uid, bool useGroups)
{
    // returns true, if flag really is changed

    if (isActive(uid)) {
        return deactivate(uid);
    }
    else {
        if (!activate(uid))
            return false;

        // From here on we have been able to activate the flag

        if (!useGroups)
            return true;

        if (!masterRow) {
            qWarning() << "FlagRow::toggle no masterRow defined for UID "
                       << uid.toString();
            return true;
        }

        // Deactivate all other active flags group except "name"
        Flag *flag = masterRow->findFlagByUid(uid);
        if (!flag)
            return true;

        QString mygroup = flag->getGroup();

        for (int i = 0; i < activeUids.size(); ++i) {
            flag = masterRow->findFlagByUid(activeUids.at(i));
            if (uid != activeUids.at(i) && !mygroup.isEmpty() &&
                mygroup == flag->getGroup())
                deactivate(activeUids.at(i));
        }
    }
    return true;
}

bool FlagRow::activate(const QString &name)
{
    if (!masterRow) {
        qWarning() << "FlagRow::activate - no masterRow to activate " << name;
        return false;
    }

    // Check, if flag exists after all...
    Flag *flag = masterRow->findFlagByName(name);
    if (!flag) {
        qWarning() << "FlagRow::activate - flag " << name
                   << " does not exist here!";
        return false;
    }

    // Some flags might be hidden, if inactive
    if (!flag->isVisible()) {
        QAction *action = flag->getAction();
        if (action)
            action->setVisible(true);
    }

    QUuid uid = flag->getUuid();
    if (!activeUids.contains(uid)) activeUids.append(uid);
    return true;
}

bool FlagRow::activate(const QUuid &uid)
{
    if (isActive(uid)) {
        if (debug)
            qWarning() << QString("FlagRow::activate - %1 is already active")
                              .arg(uid.toString());
        return true;
    }

    if (!masterRow) {
        qWarning() << "FlagRow::activate - no masterRow to activate "
                   << uid.toString();
        return false;
    }

    // Check, if flag exists after all...
    Flag *flag = masterRow->findFlagByUid(uid);
    if (!flag) {
        qWarning() << "FlagRow::activate - flag " << uid.toString()
                   << " does not exist here!";
        return false;
    }

    activeUids.append(uid);
    return true;
}

bool FlagRow::deactivate(const QString &name)
{
    Flag *flag = masterRow->findFlagByName(name);
    // qDebug() << "FlagRow::deactivate " << name << "  uuid=" << flag;
    return deactivate(flag->getUuid());
}

bool FlagRow::deactivate(const QUuid &uid)
{
    int n = activeUids.indexOf(uid);
    if (n >= 0) {
        activeUids.removeAt(n);
        // Returns true, if flag is changed
        return true;
    }
    if (debug)
        qWarning() << QString("FlagRow::deactivate - %1 is not active")
                          .arg(uid.toString());
    return false;
}

bool FlagRow::deactivateGroup(const QString &gname)
{
    if (!masterRow)
        return false;
    if (gname.isEmpty())
        return false;

    foreach (QUuid uid, activeUids) {
        Flag *flag = masterRow->findFlagByUid(uid);
        if (flag && gname == flag->getGroup())
            deactivate(flag->getUuid());
    }
    return true;
}

void FlagRow::deactivateAll()
{
    activeUids.clear();
}

QString FlagRow::saveState()
{
    QString s;

    if (!activeUids.isEmpty())
        for (int i = 0; i < activeUids.size(); ++i) {
            Flag *flag = masterRow->findFlagByUid(activeUids.at(i));

            // save flag to xml, if flag is set
            s += flag->saveState();

            // and tell parentRow, that this flag is used
            //
            // FIXME-3 used flag IDs should be saved for each vymmodel to avoid
            // problems in parallel saving of maps
            flag->setUsed(true);
        }
    return s;
}

void FlagRow::setMasterRow(FlagRowMaster *row) { masterRow = row; }

