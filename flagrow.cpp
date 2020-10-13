#include <QDebug>

#include "flagrow.h"
#include "mainwindow.h"

extern bool debug;
extern QString tmpVymDir;
extern Main *mainWindow;


/////////////////////////////////////////////////////////////////
// FlagRowFactory
/////////////////////////////////////////////////////////////////
FlagRow::FlagRow()
{
    toolbar   = NULL;
    masterRow = NULL;
    configureAction = NULL;
    //qDebug()<< "Const FlagRow ()";
}

FlagRow::~FlagRow()
{
    //qDebug()<< "Destr FlagRow    toolbar=" << toolbar  << "   masterRow=" << masterRow;
}

Flag* FlagRow::createFlag(const QString &path)
{
    Flag *flag = new Flag;
    flag->load(path);
    flags.append (flag);

    return flag;
}

void FlagRow::createConfigureAction()
{
    if (!toolbar) return;

    QAction *a = new QAction(QIcon("/home/uwe/vym/branches/userflags/icons/configure-plus.svg"), QString("add flag")); // FIXME-1 add to resources and fix path
    a->setCheckable( false );
    a->connect (a, SIGNAL( triggered() ), mainWindow, SLOT( addUserFlag() ) );

    toolbar->addAction (a);
    configureAction = a;
}

void FlagRow::addActionToToolbar(QAction *a)
{
    if (!toolbar || !a) return;

    if (configureAction)
        toolbar->insertAction(configureAction, a);
    else
        toolbar->addAction(a);
}

void FlagRow::shareCashed(Flag *flag)
{
    QString path = tmpVymDir;
    ImageObj *image = flag->getImageObj();
    if( !image->shareCashed(path + "/" + flag->getUuid().toString() + "-" + flag->getName() + image->getExtension())) // FIXME-1 check, if separator converted automagically on windows);
        qDebug() << "FR::shareCashed failed for " << flag->getName();
}

Flag* FlagRow::findFlag (const QUuid &uid)
{
    // Must only be called for masterRow itself!
    if (masterRow)
    {
        qWarning() << "FlagRow::findFlag called for !masterRow";
        return NULL;
    }

    int i = 0;
    while (i <= flags.size() - 1)
    {
	if (flags.at(i)->getUuid() == uid)
	    return flags.at(i);
	i++;	
    }
    return NULL;
}

Flag* FlagRow::findFlag (const QString &name)
{
    // Must only be called for masterRow itself!
    if (masterRow)
    {
        qWarning() << "FlagRow::findFlag called for !masterRow";
        return NULL;
    }

    int i = 0;
    while (i <= flags.size() - 1)
    {
	if (flags.at(i)->getName() == name)
	    return flags.at(i);
	i++;	
    }
    qDebug() << "FR::findFlag failed for name " << name;
    return NULL;
}

void FlagRow::resetUsedCounter()
{
    for (int i = 0; i < flags.size(); ++i)
	flags.at(i)->setUsed (false);
}

QString FlagRow::saveDef(WriteMode mode)
{
    // For the masterrow of userflags: Write definitions of flags

    QString s = "\n";

    for (int i = 0; i < flags.size(); ++i)
        if ( (mode == AllFlags) || (mode == UsedFlags && flags.at(i)->isUsed() ))
            s += flags.at(i)->getDefinition(prefix);

    return s;
}

void FlagRow::saveDataToDir (const QString &tmpdir, WriteMode mode)  
{
    // Save icons to dir, if verbose is set (xml export)
    // and I am a master
    // and this standardflag is really used somewhere.
    // Userflags are written anyway (if master flagrow)       
    
    for (int i = 0; i < flags.size(); ++i)
        if ( (mode == AllFlags) || (mode == UsedFlags && flags.at(i)->isUsed() ))
            flags.at(i)->saveDataToDir (tmpdir);
}

void FlagRow::setName (const QString &n)
{
    rowName = n;
}

void FlagRow::setPrefix (const QString &p)
{
    prefix = p;
}

QString FlagRow::getName () { return rowName; }

void FlagRow::setToolBar (QToolBar *tb)
{
    toolbar = tb;
}

void FlagRow::setMasterRow (FlagRow *row)
{
    masterRow = row; 
}

void FlagRow::updateToolBar (QList <QUuid> activeUids)
{
    if (toolbar )
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


/////////////////////////////////////////////////////////////////
// FlagRow
/////////////////////////////////////////////////////////////////
/*
FlagRow::FlagRow()
{
    toolbar   = NULL;
    masterRow = NULL;
    //qDebug()<< "Const FlagRow ()";
}

FlagRow::~FlagRow()
{
    //qDebug()<< "Destr FlagRow    toolbar=" << toolbar  << "   masterRow=" << masterRow;
}
*/

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

bool FlagRow::toggle (const QString &name, bool useGroups)  
{
    // First get UID from mastRow
    if (!masterRow)
    {
        qWarning() << "FlagRow::toggle name " << name << " no masterRow";
        return false;
    }

    Flag *flag = masterRow->findFlag(name);
    if (!flag)
    {
        qWarning() << "FlagRow::toggle name " << name <<" masterRow has no such flag";
        return false;
    }

    return toggle(flag->getUuid() );
}

bool FlagRow::toggle (const QUuid &uid, bool useGroups) 
{
    // returns true, if flag really is changed

    if (isActive(uid) )
    {
	return deactivate (uid);
    } else
    {
	if (!activate (uid) ) return false;    

        // From here on we have been able to activate the flag

	if (!useGroups) return true;

        if (!masterRow)
        {
            qWarning() << "FlagRow::toggle no masterRow defined for UID " <<uid.toString();
            return true;
        }

	// Deactivate all other active flags group except "name"
	Flag *flag = masterRow->findFlag (uid);
	if (!flag) return true;

	QString mygroup = flag->getGroup();

	for (int i = 0; i < activeUids.size(); ++i)
	{
	    flag = masterRow->findFlag (activeUids.at(i) );
	    if (uid != activeUids.at(i) && !mygroup.isEmpty() && mygroup == flag->getGroup())
		deactivate (activeUids.at(i));
	}
    }
    return true;
}

bool FlagRow::activate (const QString &name)    
{
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

    // Some flags might be hidden, if inactive  // FIXME-1 review when refactoring FR
    if (!flag->isVisible() ) 
    {
        // FIXME-1 testing flag->setVisible(true);
        QAction *action = flag->getAction();
        if (action) action->setVisible(true);
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
    qDebug() << "FR::activate  " << flag->getName();    // FIXME-1 not called???

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
        // Returns true, if flag is changed
	return true;
    }
    if (debug) 
	qWarning ()<<QString("FlagRow::deactivate - %1 is not active").arg(uid.toString());
    return false;
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
    if (!toolbar) activeUids.clear();
}

void FlagRow::setEnabled (bool b)
{
    toolbar->setEnabled (b);
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

