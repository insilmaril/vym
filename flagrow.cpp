#include <QDebug>

#include "flagrow.h"

extern bool debug;

/////////////////////////////////////////////////////////////////
// FlagRow
/////////////////////////////////////////////////////////////////
FlagRow::FlagRow()
{
    toolBar   = NULL;
    masterRow = NULL;
//    qDebug()<< "Const FlagRow ()";
}

FlagRow::~FlagRow()
{
    //qDebug()<< "Destr FlagRow";
}

Flag* FlagRow::addFlag (Flag *flag) 
{
    Flag *f = new Flag;
;
    f->copy (flag);
    flags.append (f);
    return f;
}

Flag* FlagRow::findFlag (const QString &name)  
{
    int i = 0;
    while (i <= flags.size() - 1)
    {
	if (flags.at(i)->getName() == name)
	    return flags.at(i);
	i++;	
    }
    return NULL;
}

Flag* FlagRow::findFlag (const QUuid &uid)
{
    int i = 0;
    while (i <= flags.size() - 1)
    {
	if (flags.at(i)->getUuid() == uid)
	    return flags.at(i);
	i++;	
    }
    return NULL;
}

const QStringList FlagRow::activeFlagNames()
{
    return QStringList();
}

const QList <QUuid> FlagRow::activeFlagUids()
{
    return activeUids;
}


bool FlagRow::isActive (const QString &name)
{
    Flag *f = masterRow->findFlag(name);
    if (!f)
    {
        qWarning() << "FlagRow::isActive couldn't find flag named " << name;
        return false;
    }

    return isActive(f->getUuid());
}

bool FlagRow::isActive (const QUuid &uid)	
{
    QUuid i;
    foreach (i, activeUids)
	if (i == uid) return true;
    return false;   
}

void FlagRow::toggle (const QString &name, bool useGroups)  
{
    Flag *f = findFlag(name);
    toggle (f->getUuid(), useGroups);
}

void FlagRow::toggle (const QUuid &uid, bool useGroups) 
{
    if (isActive(uid) )
    {
	deactivate (uid);
    } else
    {
	if (!activate (uid) ) return;    

	if (!masterRow || !useGroups) return;

	// Deactivate all other active flags group except "name"
	Flag *flag = masterRow->findFlag (uid);
	if (!flag) return;

	QString mygroup = flag->getGroup();

	for (int i = 0; i < activeUids.size(); ++i)
	{
	    flag = masterRow->findFlag (activeUids.at(i) );
	    if (uid != activeUids.at(i) && !mygroup.isEmpty() && mygroup == flag->getGroup())
		deactivate (activeUids.at(i));
	}
    }
}

bool FlagRow::activate (const QString &name)    
{
    foreach (Flag *f, flags)
        qDebug() << "  " << f->getUuid() << f->getName();

    if (isActive (name)) 
    {
	if (debug) qWarning () << QString("FlagRow::activate - %1 is already active").arg(name);
	return false;
    }

    if (!masterRow)
    {
	qWarning() << "FlagRow::activate - no masterRow to activate " << name;
	return false;
    }

    // Check, if flag exists after all...
    Flag *flag = masterRow->findFlag (name);
    if (!flag)
    {
	qWarning() << "FlagRow::activate - flag " << name << " does not exist here!";
	return false;
    }

    activeUids.append (flag->getUuid());
    return true;
}

bool FlagRow::activate (const QUuid &uid)
{
    if (isActive (uid)) 
    {
	if (debug) 
            qWarning () << QString("FlagRow::activate - %1 is already active").arg(uid.toString());
	return true;
    }

    if (!masterRow)
    {
	qWarning() << "FlagRow::activate - no masterRow to activate " << uid.toString();
	return false;
    }

    // Check, if flag exists after all...
    Flag *flag = masterRow->findFlag (uid);
    if (!flag)
    {
	qWarning() << "FlagRow::activate - flag " << uid.toString() << " does not exist here!";
	return false;
    }

    activeUids.append (uid);

    return true;
}


bool FlagRow::deactivate (const QString &name) 
{
    foreach (Flag *f, flags)
        qDebug() << "  " << f->getUuid() << f->getName();

    Flag *flag = masterRow->findFlag (name);
    return deactivate (flag->getUuid());
}

bool FlagRow::deactivate (const QUuid &uid)
{
    int n = activeUids.indexOf (uid);
    if (n >= 0)
    {
	activeUids.removeAt(n);
	return true;
    }
    if (debug) 
	qWarning ()<<QString("FlagRow::deactivate - %1 is not active").arg(uid.toString());
    return true;
}

bool FlagRow::deactivateGroup (const QString &gname) 
{
    if (!masterRow) return false;
    if (gname.isEmpty()) return false;

    foreach (QUuid uid, activeUids)
    {
	Flag *flag = masterRow->findFlag (uid);
	if (flag && gname == flag->getGroup())
	    deactivate (flag->getUuid() );
    }
    return true;
}

void FlagRow::deactivateAll ()
{
    if (!toolBar) activeUids.clear();
}

void FlagRow::setEnabled (bool b)
{
    toolBar->setEnabled (b);
}

void FlagRow::resetUsedCounter()
{
    for (int i=0; i<flags.size(); ++i)
	flags.at(i)->setUsed (false);
}

QString FlagRow::saveDef()
{
    // For the masterrow of userflags: Write definitions of flags

    QString s = "\n";

    for (int i = 0; i < flags.size(); ++i)
        s += flags.at(i)->saveDef();

    return s;
}

bool FlagRow::saveDataToDir (const QString &tmpdir, const QString &prefix)  // FIXME-1 only save flags, if used or default map
{
    bool r = true;
    
    // Save icons to dir, if verbose is set (xml export)
    // and I am a master
    // and this standardflag is really used somewhere.
    // Userflags are written anyway (if master flagrow)
    
    for (int i = 0; i < flags.size(); ++i)
        if (!flags.at(i)->saveDataToDir (tmpdir, prefix))
            r = false;

    return r;	    
}

QString FlagRow::saveState ()
{
    QString s;
    
    if (!activeUids.isEmpty())
        for (int i = 0; i < activeUids.size(); ++i)
        {
            Flag *flag = masterRow->findFlag(activeUids.at(i) );

            // save flag to xml, if flag is set 
            s += flag->saveState();

            // and tell parentRow, that this flag is used   
            flag->setUsed(true);
        }   
    return s;	    
}

void FlagRow::setName (const QString &n)
{
    rowName = n;
}

QString FlagRow::getName () { return rowName; }

void FlagRow::setToolBar (QToolBar *tb)
{
    toolBar = tb;
}

void FlagRow::setMasterRow (FlagRow *row)
{
    masterRow = row; 
}

void FlagRow::updateToolBar (QList <QUuid> activeUids)
{
    if (toolBar )
    {
	for (int i = 0;i < flags.size();++i)
	    flags.at(i)->getAction()->setChecked (false);

	for (int i = 0;i < flags.size();++i)
	{
	    int n = activeUids.indexOf (flags.at(i)->getUuid());
	    if (n >= 0)
		flags.at(i)->getAction()->setChecked (true);	
	}
    }
}


