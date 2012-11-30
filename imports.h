#ifndef IMPORTS_H
#define IMPORTS_H

#include <qdir.h>
#include <qstring.h>
#include <iostream>

#include "settings.h"


///////////////////////////////////////////////////////////////////////

class ImportBase
{
public:
    ImportBase();
    virtual ~ImportBase();
    virtual void setDir(const QString &);
    virtual void setFile(const QString &);
    virtual bool transform();
    virtual QString getTransformedFile();
protected:
    QDir tmpDir;
    QString inputDir;
    QString inputFile;
    QString transformedFile;
    
};

///////////////////////////////////////////////////////////////////////
class ImportKDE4Bookmarks:public ImportBase
{
public:
    bool transform();
};  


///////////////////////////////////////////////////////////////////////
class ImportFirefoxBookmarks:public ImportBase
{
public:
    bool transform();
};  


///////////////////////////////////////////////////////////////////////
class ImportMM:public ImportBase
{
public:
    bool transform();
};  



#endif
