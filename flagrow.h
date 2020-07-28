#ifndef FLAGROW_H
#define FLAGROW_H

#include <QList>
#include <QStringList>
#include <QToolBar>

#include "flag.h"
#include "xmlobj.h"

/*! \brief A set of flags (Flag). 

   A toolbar can be created from the flags in this row.
   The data needed for represention in a vym map 
   is stored in FlagRowObj.
 */

class FlagRow:public XMLObj {
public:
    FlagRow ();
    ~FlagRow ();
    Flag* addFlag (Flag *flag);
    Flag *findFlag (const QString &name);
    Flag *findFlag (const QUuid &uid);
    const QStringList  activeFlagNames();
    const QList <QUuid> activeFlagUids();
    bool isActive(const QString &name);
    bool isActive(const QUuid &uuid);

    /*! \brief Toggle a Flag 
	
	To activate a flag it will be copied from masterRow to current row.
    */	
    bool toggle (const QString&, bool useGroups = true);
    bool toggle (const QUuid&, bool useGroups = true);
    bool activate(const QString&);
    bool activate(const QUuid&);
    bool deactivate(const QString&);
    bool deactivate(const QUuid&);
    bool deactivateGroup(const QString&);
    void deactivateAll();
    void setEnabled (bool);
    void resetUsedCounter();
    QString saveDef();
    bool saveDataToDir (const QString &,const QString &);
    QString saveState();
    void setName (const QString&);	    // prefix for exporting flags to dir
    void setToolBar   (QToolBar *tb);
    void setMasterRow (FlagRow *row);
    void updateToolBar(const QStringList &activeNames);

private:    
    QToolBar *toolBar;
    FlagRow *masterRow;
    QList <Flag*> flags;        //! Used in master row to define flags
    QStringList activeNames;	//! Used in treeitems: Lists all names of currently active flags
    QList <QUuid> activeUids;	//! Used in treeitems: Lists all uids of currently active flags
    QString rowName;		//! Name of this collection of flags
};
#endif

