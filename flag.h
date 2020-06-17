#ifndef FLAG_H
#define FLAG_H


#include <QAction>
#include <QPixmap>

#include "xmlobj.h"

/*! \brief One flag belonging to a FlagRow.

    Each TreeItem in a VymModel has a set of standard flags and system
    flags.
*/


/////////////////////////////////////////////////////////////////////////////
class Flag:public XMLObj {
public:
    enum FlagType {SystemFlag, StandardFlag, UserFlag, UndefinedFlag};

    Flag ();
    Flag (const QString &fname);
    Flag (Flag*);
    ~Flag ();
    virtual void init ();
    virtual void copy (Flag*);
    void load (const QString&);
    void load (const QPixmap&);
    void setName (const QString&);
    const QString getName ();
    void setVisible (bool b);
    bool isVisible ();
    void setGroup (const QString&);
    const QString getGroup();
    void unsetGroup ();
    void setToolTip(const QString&);
    const QString getToolTip();
    QPixmap getPixmap();
    void setAction (QAction *a);
    QAction* getAction ();
    void setUsed (bool);    
    bool isUsed();
    FlagType getType();
    void setType (FlagType t);
    void saveToDir (const QString&, const QString&);
    
protected:  
    QString name;
    bool visible;
    QString group;
    QString tooltip;
    QAction *action;
    bool state;
    bool used;
    FlagType type;

private:
    QPixmap pixmap;
    
};

#endif
