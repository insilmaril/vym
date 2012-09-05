#include <iostream>

#include <QDebug>

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

bool SimpleSettings::readSettings (const QString &path)
{
    QString s;
    if (!loadStringFromDisk(path,s)) 
    {
	qWarning ()<<"SimpleSettings::readSettings() Couldn't read "+path;
	return false;
    }	
    QStringList lines;
    lines=s.split (QRegExp("\n"));
    int i;
    QStringList::Iterator it=lines.begin();
    while (it !=lines.end() )
    {
	i=(*it).indexOf("=",0);
	keylist.append((*it).left(i));
	valuelist.append((*it).right((*it).length()-i-1));
	it++;
    }
    return true;
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
	qWarning ()<<"SimpleSettings::writeSettings() Couldn't write "+path;
}

/*
QString SimpleSettings::readValue (const QString &key)
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
    qWarning ("SimpleSettings::readValue()  Couldn't find key "+key);
    return "";
}
*/

QString SimpleSettings::value (const QString &key, const QString &def)
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

int SimpleSettings::readNumValue (const QString &key, const int &def)
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

void SimpleSettings::setValue (const QString &key, const QString &value)
{
    QStringList::Iterator itk=keylist.begin();
    QStringList::Iterator itv=valuelist.begin();

    if (!key.isEmpty() )
    {
	// Search for existing Value first
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
	
	// If no Value exists, append a new one
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

Settings::Settings(const QString & organization, const QString & application )
#ifdef Q_OS_WIN32
    :QSettings (application, QSettings::IniFormat)
#else
    :QSettings (organization, application)
#endif
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

void Settings::clearLocal(const QString &fpath, const QString &key)
{
    int i=0;
    while (i<pathlist.count() )
    {
	if (fpath == pathlist.at(i) && keylist.at(i).startsWith (key))
	{
            pathlist.removeAt(i);
            keylist.removeAt(i);
            valuelist.removeAt(i);
	}   else
            i++;
    }
}

QVariant Settings::localValue ( const QString &fpath, const QString & key, QVariant def) 
{
    // First search for value in settings saved in map
    int i=0;
    while (i<pathlist.count() )
    {
        if (pathlist.at(i) == fpath && keylist.at(i) == key)
	    return valuelist.at(i);
        i++;
    }

    // Fall back to global vym settings
    return value (key,def);
}   

void Settings::setLocalValue (const QString &fpath, const QString &key, QVariant value)
{
    if (!fpath.isEmpty() && !key.isEmpty() && !value.isNull() )
    {
	// Search for existing Value first
        int i=0;
	while (i<pathlist.count())
	{
            if (pathlist.at(i) == fpath && keylist.at(i) == key)
	    {
                valuelist[i]=value;
		return;
	    }
            i++;
	}
	
	// If no Value exists, append a new one
	pathlist.append (fpath);
	keylist.append (key);
	valuelist.append (value);   
    }
}

QString Settings::getDataXML (const QString &fpath)
{
    QString s;
    int i=0;
    while (i<pathlist.count())
    {
	if (pathlist.at(i)==fpath)
	    if (!valuelist.at(i).isNull())
		s+=singleElement (
		    "setting",
		    attribut ("key",keylist.at(i)) 
		    +attribut ("value",valuelist.at(i).toString())
		);
        i++;
    }
    return s;
}

