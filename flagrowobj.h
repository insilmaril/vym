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
    virtual void init ();
    virtual void copy (FlagRowObj*);
    virtual void move   (double,double);
    virtual void moveBy (double,double);
    virtual void setZValue (double z);
    virtual void setVisibility(bool);
    virtual FlagObj* addFlag (FlagObj *fo);	    // make deep copy of FlagObj
    virtual QStringList activeFlagNames();
    virtual void positionBBox();
    virtual void calcBBoxSize();
    virtual QString getFlagName (const QPointF &p); // Find flag by position
    bool isActive(const QString&);
    void activate (Flag *flag);
    void deactivate(const QString&);
    void setShowFlags (bool);
    FlagObj* findFlag (const QString&);
private:    
    QList <FlagObj*> flag; 
    bool showFlags;			    // FloatObjects want to hide their flags
};
#endif
