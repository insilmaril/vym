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
    Flag *f=new Flag;
;
    f->copy (flag);
    flags.append (f);
    activeNames.append (flag->getName());
}

Flag* FlagRow::getFlag (const QString &name)
{
    int i=0;
    while (i<=flags.size()-1)
    {
	if (flags.at(i)->getName()==name)
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
    foreach (n,activeNames)
	if (n==name) return true;
    return false;   
}

bool FlagRow::toggle (const QString &name, FlagRow *masterRow)
{
    if (isActive(name) )
	return deactivate (name);
    else
    {
	if (!activate (name) ) return false;    

	// Deactivate group
	if (!masterRow) return false;

	Flag *flag=masterRow->getFlag (name);
	if (!flag) return false;
	QString mygroup=flag->getGroup();

	for (int i=0;i<activeNames.size();++i)
	{
	    flag=masterRow->getFlag (activeNames.at(i) );
	    if (name!=activeNames.at(i) && !mygroup.isEmpty() && mygroup==flag->getGroup())
		deactivate (activeNames.at(i));
	}
	return true;
    }
}

bool FlagRow::activate (const QString &name)
{
    if (isActive (name)) 
    {
	if (debug) qWarning ()<<QString("FlagRow::activate - %1 is already active").arg(name);
	return false;
    }

    if (!masterRow)
    {
	qWarning()<<"FlagRow::activate - no masterRow to activate "<<name;
	return false;
    }

    // Check, if flag exists after all...
    Flag *flag=masterRow->getFlag (name);
    if (!flag)
    {
	qWarning()<<"FlagRow::activate - flag "<<name<<" does not exist here!";
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
    else
    {
	if (debug) qWarning ()<<QString("FlagRow::deactivate - %1 is not active").arg(name);
	return false;
    }
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

QString FlagRow::saveToDir (const QString &tmpdir,const QString &prefix, bool writeflags) 
{
    // Build xml string
    QString s;
    
    if (!toolBar)
    {
	if (!activeNames.isEmpty())
	for (int i=0; i<activeNames.size(); ++i)
	{
	    // save flag to xml, if flag is set 
	    s+=valueElement("standardflag",activeNames.at(i));

	    // and tell parentRow, that this flag is used   
	    masterRow->getFlag(activeNames.at(i))->setUsed(true);
	}   
    } else
	// Save icons to dir, if verbose is set (xml export)
	// and I am a master
	// and this flag is really used somewhere
	if (writeflags)
	    for (int i=0; i<flags.size(); ++i)
		if (flags.at(i)->isUsed()) flags.at(i)->saveToDir (tmpdir,prefix);
    return s;	    
}

void FlagRow::setName (const QString &n)
{
    rowName=n;
}

void FlagRow::setToolBar (QToolBar *tb)
{
    toolBar=tb;
}

void FlagRow::setMasterRow (FlagRow *row)
{
    masterRow=row; 
}

void FlagRow::updateToolBar (const QStringList &activeNames)
{
    if (toolBar )
    {
	for (int i=0;i<flags.size();++i)
	    flags.at(i)->getAction()->setChecked (false);
	for (int i=0;i<flags.size();++i)
	{
	    int n=activeNames.indexOf (flags.at(i)->getName());
	    if (n>=0)
		flags.at(i)->getAction()->setChecked (true);	
	}
    }
}


