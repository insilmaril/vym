#ifndef SETTINGS_H
#define SETTINGS_H

#include <qsettings.h>
#include <qstring.h>
#include <qstringlist.h>

#include "xmlobj.h"

// Some helper functions and simplified settings class
// to read and parse settings e.g.  in undo/redo directories

class SimpleSettings
{
public:
    SimpleSettings ();
    ~SimpleSettings ();
    void clear();
    void readSettings(const QString &);
    void writeSettings(const QString &);
    QString readEntry (const QString &key, const QString &def=QString());
    int readNumEntry (const QString &, const int &def=0);
    void setEntry (const QString &,const QString &);
private:    
    QStringList keylist;
    QStringList valuelist;
};


// Overloaded QSettings class, used to save some settings in 
// a map instead of users home directory
class Settings:public QSettings,public XMLObj
{
public:
    Settings ();
    Settings (const QString & , const QString &);
    ~Settings ();
    void clear();
    void clearLocal (const QString &);
    QString readLocalEntry ( const QString &, const QString &, const QString &);
    void setLocalEntry (const QString &, const QString &, const QString &);
    QString getDataXML (const QString &);

protected:
    QStringList pathlist;
    QStringList keylist;
    QStringList valuelist;
};

#endif
