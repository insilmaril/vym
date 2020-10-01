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
    enum WriteMode {NoFlags, UsedFlags, AllFlags};
    FlagRow ();
    ~FlagRow ();
    Flag* createFlag (const QString &path);
    void shareCashed(Flag *flag);
    Flag *findFlag (const QUuid &uid);
    Flag *findFlag (const QString &name);
    const QStringList  activeFlagNames();
    const QList <QUuid> activeFlagUids();
    bool isActive(const QString &name);
    bool isActive(const QUuid &uuid);

    /*! \brief Toggle a Flag 
	
	To activate a flag its uid will be copied from masterRow to activeUids in current row.
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
    void saveDataToDir (const QString &, WriteMode mode);
    QString saveState();
    void setName (const QString&);	    // prefix for exporting flags to dir
    QString getName();                      // Used for debugging only
    void setPrefix (const QString &prefix);
    void setToolBar   (QToolBar *tb);
    void setMasterRow (FlagRow *row);
    void updateToolBar(QList <QUuid> activeUids);

private:    
    QToolBar *toolBar;
    FlagRow *masterRow;
    QList <Flag*> flags;        //! Used in master row to define flags
    QList <QUuid> activeUids;	//! Used in treeitems: Lists all uids of currently active flags
    QString rowName;		//! Name of this collection of flags
    QString prefix;             //! Prefix for saving data: user/ or standard/
};
#endif

