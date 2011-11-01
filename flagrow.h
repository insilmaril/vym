#ifndef FLAGROW_H
#define FLAGROW_H

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
    void addFlag (Flag *flag);
    Flag *getFlag (const QString &name);
    QStringList  activeFlagNames();
    bool isActive(const QString &name);

    /*! \brief Toggle a Flag 
	
	To activate a flag it will be copied from masterRow to current row.
    */	
    void toggle (const QString&, FlagRow *masterRow=NULL);
    void activate(const QString&);
    void deactivate(const QString&);
    void deactivateAll();
    void setEnabled (bool);
    void resetUsedCounter();
    QString saveToDir (const QString &,const QString &,bool);
    void setName (const QString&);	    // prefix for exporting flags to dir
    void setToolBar   (QToolBar *tb);
    void setMasterRow (FlagRow *row);
    void updateToolBar(const QStringList &activeNames);

private:    
    QToolBar *toolBar;
    FlagRow *masterRow;
    QList <Flag*> flags; 
    QStringList activeNames;	//! Lists all names of currently active flags
    QString rowName;		//! Name of this collection of flags
//  bool showFlags;		// FloatObjects want to hide their flags
};
#endif

