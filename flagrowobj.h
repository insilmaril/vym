#ifndef FLAGROWOBJ_H
#define FLAGROWOBJ_H

#include <QMainWindow>

//#include "mapobj.h"
#include "flagobj.h"

class Flag;

/*! \brief A collection of flags (FlagObj) in a map. 

   The flags are aligned horizontally  in a row on the map. 
 */

class FlagRowObj:public MapObj {
public:
    FlagRowObj ();
    FlagRowObj (QGraphicsItem*);
    ~FlagRowObj ();
    virtual void copy (FlagRowObj*);
    virtual void move   (double,double);
    virtual void moveBy (double,double);
    virtual void setZValue (double z);
    virtual void setVisibility(bool);
    virtual FlagObj* addFlag (FlagObj *fo);	    // make deep copy of FlagObj
    void updateActiveFlagObjs (const QList <QUuid>, FlagRow *masterRowMain,  FlagRow *masterRowOptional = NULL);
    virtual QStringList activeFlagNames();
    virtual void positionBBox();
    virtual void calcBBoxSize();
    bool isFlagActiveByName(const QString&);
    bool isFlagActive(const QUuid&);
    void activateFlag (Flag *flag);
    FlagObj* findFlagObj (const QString&);
    FlagObj* findFlagObj (const QUuid&);
    virtual QString findFlagNameByPos (const QPointF &p); // Find flag by position
private:    
    QList <FlagObj*> flagobjs; 
    bool showFlags;			    // FloatObjects want to hide their flags
};
#endif
