#include <iostream>
#include <qregexp.h>
#include "settings.h"
#include "file.h"

/////////////////////////////////////////////////////////////////
// SimpleSettings
/////////////////////////////////////////////////////////////////
SimpleSettings::SimpleSettings()
{
    clear();	     
}

SimpleSettings::~SimpleSettings()
{
}

void SimpleSettings::clear()
{
    keylist.clear();
    valuelist.clear();
}

void SimpleSettings::readSettings (const QString &path)
{
    QString s;
    if (!loadStringFromDisk(path,s)) 
    {
	qWarning ("SimpleSettings::readSettings() Couldn't read "+path);
	return;
    }	
    QStringList lines;
    lines=QStringList::split (QRegExp("\n"),s,false);
    int i;
    QStringList::Iterator it=lines.begin();
    while (it !=lines.end() )
    {
	i=(*it).find("=",0);
	keylist.append((*it).left(i));
	valuelist.append((*it).right((*it).length()-i-1));
	it++;
    }
}

void SimpleSettings::writeSettings (const QString &path)
{
    QString s;
    QStringList::Iterator itk=keylist.begin();
    QStringList::Iterator itv=valuelist.begin();

    // First search for value in settings saved in map
    while (itk !=keylist.end() )
    {
	s+=*itk+"="+*itv+"\n";
	itk++;
	itv++;
    }
    if (!saveStringToDisk(path,s)) 
	qWarning ("SimpleSettings::writeSettings() Couldn't write "+path);
}

/*
QString SimpleSettings::readEntry (const QString &key)
{
    QStringList::Iterator itk=keylist.begin();
    QStringList::Iterator itv=valuelist.begin();

    // First search for value in settings saved in map
    while (itk !=keylist.end() )
    {
	if (*itk == key)
	    return *itv;
	itk++;
	itv++;
    }
    qWarning ("SimpleSettings::readEntry()  Couldn't find key "+key);
    return "";
}
*/

QString SimpleSettings::readEntry (const QString &key, const QString &def)
{
    QStringList::Iterator itk=keylist.begin();
    QStringList::Iterator itv=valuelist.begin();

    // First search for value in settings saved in map
    while (itk !=keylist.end() )
    {
	if (*itk == key)
	    return *itv;
	itk++;
	itv++;
    }
    return def;
}

int SimpleSettings::readNumEntry (const QString &key, const int &def)
{
    QStringList::Iterator itk=keylist.begin();
    QStringList::Iterator itv=valuelist.begin();

    // First search for value in settings saved in map
    while (itk !=keylist.end() )
    {
	if (*itk == key)
	{
	    bool ok;
	    int i=(*itv).toInt(&ok,10);
	    if (ok)
		return i;
	    else
		return def;
	}   
	itk++;
	itv++;
    }
    return def;
}

void SimpleSettings::setEntry (const QString &key, const QString &value)
{
    QStringList::Iterator itk=keylist.begin();
    QStringList::Iterator itv=valuelist.begin();

    if (!key.isEmpty() )
    {
	// Search for existing entry first
	while (itk !=keylist.end() )
	{
	    if (*itk == key)
	    {
		if (!value.isEmpty())
		    *itv=value;
		else
		    *itv="";
		*itv=value;
		return;
	    }
	    itk++;
	    itv++;
	}
	
	// If no entry exists, append a new one
	keylist.append (key);
	valuelist.append (value);
    }
}



/////////////////////////////////////////////////////////////////
// Settings
/////////////////////////////////////////////////////////////////
Settings::Settings()
{
    clear();	     
}

Settings::Settings(const QString & organization, const QString & application ):QSettings (organization,application)
{
    clear();	     
}

Settings::~Settings()
{
}

void Settings::clear()
{
    pathlist.clear();
    keylist.clear();
    valuelist.clear();
}

void Settings::clearLocal(const QString &s)
{
    QStringList::Iterator itp=pathlist.begin();
    QStringList::Iterator itk=keylist.begin();
    QStringList::Iterator itv=valuelist.begin();

    while (itp !=pathlist.end() )
    {
	if ((*itk).startsWith (s))
	{
	    itp=pathlist.remove (itp);
	    itk=keylist.remove (itk);
	    itv=valuelist.remove (itv);
	}   else
	{
	    itp++;
	    itk++;
	    itv++;
	}
    }
}

QString Settings::readLocalEntry ( const QString &fpath, const QString & key, const QString & def = QString::null ) 
{
    QStringList::Iterator itp=pathlist.begin();
    QStringList::Iterator itk=keylist.begin();
    QStringList::Iterator itv=valuelist.begin();

    // First search for value in settings saved in map
    while (itp !=pathlist.end() )
    {
	if (*itp == fpath && *itk == key)
	    return *itv;
	itp++;
	itk++;
	itv++;
    }

    // Fall back to global vym settings
    bool ok;
    return readEntry (key,def, &ok);
}   

void Settings::setLocalEntry (const QString &fpath, const QString &key, const QString &value)
{
    QStringList::Iterator itp=pathlist.begin();
    QStringList::Iterator itk=keylist.begin();
    QStringList::Iterator itv=valuelist.begin();

    if (!fpath.isEmpty() && !key.isEmpty() && !value.isEmpty() )
    {
	// Search for existing entry first
	while (itp !=pathlist.end() )
	{
	    if (*itp == fpath && *itk == key)
	    {
		*itv=value;
		return;
	    }
	    itp++;
	    itk++;
	    itv++;
	}
	
	// If no entry exists, append a new one
	pathlist.append (fpath);
	keylist.append (key);
	valuelist.append (value);
    }
}

QString Settings::getDataXML (const QString &fpath)
{
    QString s;
    QStringList::Iterator itp=pathlist.begin();
    QStringList::Iterator itk=keylist.begin();
    QStringList::Iterator itv=valuelist.begin();

    while (itp !=pathlist.end() )
    {
	if (*itp == fpath )
	    if (!(*itv).isEmpty())
		s+=singleElement (
		    "setting",
		    attribut ("key",*itk) 
		    +attribut ("value",*itv)
		);
	itp++;
	itk++;
	itv++;
    }
    return s;
}

