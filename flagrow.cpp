#include <QDebug>

#include "flagrow.h"

extern bool debug;

/////////////////////////////////////////////////////////////////
// FlagRow
/////////////////////////////////////////////////////////////////
FlagRow::FlagRow()
{
    toolBar=NULL;
    masterRow=NULL;
//    qDebug()<< "Const FlagRow ()";
}

FlagRow::~FlagRow()
{
    //qDebug()<< "Destr FlagRow";
}

void FlagRow::addFlag (Flag *flag)
{
    Flag *f = new Flag;
;
    f->copy (flag);
    flags.append (f);
    activeNames.append (flag->getName());
}

Flag* FlagRow::getFlag (const QString &name)
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

QStringList FlagRow::activeFlagNames()
{
    return activeNames;
}


bool FlagRow::isActive (const QString &name)	
{
    QString n;
    foreach (n, activeNames)
	if (n == name) return true;
    return false;   
}

bool FlagRow::toggle (const QString &name, bool useGroups)
{
    if (isActive(name) )
	return deactivate (name);
    else
    {
	if (!activate (name) ) return false;    

	if (!masterRow || !useGroups) return false;

	// Deactivate all other active flags group except "name"
	Flag *flag = masterRow->getFlag (name);
	if (!flag) return false;
	QString mygroup = flag->getGroup();

	for (int i = 0; i < activeNames.size(); ++i)
	{
	    flag = masterRow->getFlag (activeNames.at(i) );
	    if (name != activeNames.at(i) && !mygroup.isEmpty() && mygroup == flag->getGroup())
		deactivate (activeNames.at(i));
	}
	return true;
    }
}

bool FlagRow::activate (const QString &name)
{
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
    Flag *flag = masterRow->getFlag (name);
    if (!flag)
    {
	qWarning() << "FlagRow::activate - flag " << name << " does not exist here!";
	return false;
    }

    activeNames.append (name);
    return true;
}


bool FlagRow::deactivate (const QString &name)
{
    int n=activeNames.indexOf (name);
    if (n>=0)
    {
	activeNames.removeAt(n);
	return true;
    }
    if (debug) 
	qWarning ()<<QString("FlagRow::deactivate - %1 is not active").arg(name);
    return false;
}

bool FlagRow::deactivateGroup (const QString &gname) 
{
    if (!masterRow) return false;
    if (gname.isEmpty()) return false;

    foreach (QString s, activeNames )
    {
	Flag *flag=masterRow->getFlag (s);
	if (flag && gname == flag->getGroup())
	    deactivate (s);
    }
    return true;
}

void FlagRow::deactivateAll ()
{
    if (!toolBar) activeNames.clear();
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

    QString s;

    for (int i = 0; i < flags.size(); ++i)
        s += QString("<userflag name=\"%1\" path=\"%2\"/>\n").arg(flags.at(i)->getName()).arg(flags.at(i)->getPath());

    return s;
}

bool FlagRow::saveDataToDir (const QString &tmpdir, const QString &prefix)
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
    
    if (!activeNames.isEmpty())
        for (int i = 0; i < activeNames.size(); ++i)
        {
            Flag *flag = masterRow->getFlag(activeNames.at(i) );

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

void FlagRow::setToolBar (QToolBar *tb)
{
    toolBar = tb;
}

void FlagRow::setMasterRow (FlagRow *row)
{
    masterRow = row; 
}

void FlagRow::updateToolBar (const QStringList &activeNames)
{
    if (toolBar )
    {
	for (int i = 0;i < flags.size();++i)
	    flags.at(i)->getAction()->setChecked (false);
	for (int i = 0;i < flags.size();++i)
	{
	    int n = activeNames.indexOf (flags.at(i)->getName());
	    if (n >= 0)
		flags.at(i)->getAction()->setChecked (true);	
	}
    }
}


