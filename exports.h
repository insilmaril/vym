#ifndef EXPORTS_H
#define EXPORTS_H

#include <qdir.h>
#include <qstring.h>
#include <iostream>

#include "settings.h"
#include "vymmodel.h"


/*! \brief Base class for all exports
*/

///////////////////////////////////////////////////////////////////////

class ExportBase
{
public:
    ExportBase();
    ExportBase(VymModel *m);
    virtual ~ExportBase();
    virtual void init();
    virtual void setDirPath (const QString&);
    virtual QString getDirPath();
    virtual void setFilePath (const QString&);
    virtual QString getFilePath ();
    virtual QString getMapName ();
    virtual void setModel (VymModel *m);
    virtual void setWindowTitle (const QString &);
    virtual void addFilter (const QString &);
    virtual void setListTasks( bool b);
    virtual bool execDialog();
    virtual bool canceled();
    void completeExport(QString args="");  //! set lastExport and send status message

protected:  
    VymModel *model;
    QString exportName;
    virtual QString getSectionString (TreeItem*);

    QString indent (const int &n, bool useBullet);
    QDir tmpDir;
    QString dirPath;        // Path to dir  e.g. /tmp/vym-export/
    QString defaultDirPath; // Default path
    QString filePath;       // Path to file e.g. /tmp/vym-export/export.html
    QString extension;      // Extension, e.g. .html
    QString indentPerDepth;
    int indentPerDepth2;
    QStringList bulletPoints;
    QString caption;
    QString filter;
    bool listTasks;         // Append task list
    bool cancelFlag;
};

///////////////////////////////////////////////////////////////////////
class ExportAO:public ExportBase
{
public:
    ExportAO();
    virtual void doExport();
    virtual QString underline (const QString &text, const QString &line);
};

///////////////////////////////////////////////////////////////////////
class ExportASCII:public ExportBase
{
public:
    ExportASCII();
    virtual void doExport();
    virtual QString underline (const QString &text, const QString &line);
};

///////////////////////////////////////////////////////////////////////
class ExportCSV:public ExportBase
{
public:
    ExportCSV();
    void doExport();
};

///////////////////////////////////////////////////////////////////////
class ExportXMLBase:public ExportBase
{
};

///////////////////////////////////////////////////////////////////////
class ExportKDE4Bookmarks:public ExportXMLBase
{
public:
    virtual void doExport();
};  

///////////////////////////////////////////////////////////////////////
class ExportFirefoxBookmarks:public ExportXMLBase
{
public:
    virtual void doExport();
};  

#include "exporthtmldialog.h"
///////////////////////////////////////////////////////////////////////
class ExportHTML:public ExportBase
{
public:
    ExportHTML();
    ExportHTML(VymModel *m);
    virtual void init();
    virtual QString createTOC();
    virtual void doExport(bool useDialog=true);
private:
    QString getBranchText(BranchItem *);
    QString buildList (BranchItem *);
    QString imageMap;
    QString cssSrc;
    QString cssDst;

    bool frameURLs;

    QPointF offset;

    ExportHTMLDialog dia;
};  

///////////////////////////////////////////////////////////////////////
class ExportLaTeX:public ExportBase
{
public:
    ExportLaTeX();
    QString escapeLaTeX (const QString &s);
    virtual void doExport();
private:
    QHash <QString,QString> esc;
};  

///////////////////////////////////////////////////////////////////////
class ExportOrgMode:public ExportBase
{
public:
    ExportOrgMode();
    virtual void doExport();
};  

///////////////////////////////////////////////////////////////////////
class ExportOO:public ExportBase
{
public:
    ExportOO();
    ~ExportOO();
    void exportPresentation();
    bool setConfigFile (const QString &);
private:
    QString buildList (TreeItem *);
    bool useSections;
    QString configFile;
    QString configDir;
    QString templateDir;
    QString content;
    QString contentTemplate;
    QString contentTemplateFile;
    QString contentFile;
    QString pageTemplate;
    QString pageTemplateFile;
    QString sectionTemplate;
    QString sectionTemplateFile;
};

///////////////////////////////////////////////////////////////////////
class ExportTaskjuggler:public ExportXMLBase
{
public:
    virtual void doExport();
};  

#endif
