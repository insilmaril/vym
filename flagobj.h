#ifndef FLAGOBJ_H
#define FLAGOBJ_H


#include <QAction>
#include <QPixmap>
#include <QUuid>

#include "flag.h"
#include "mapobj.h"
#include "imageobj.h"

/*! \brief One flag which is visible in the map. 

    Flags are aligned in a row. 
*/


/////////////////////////////////////////////////////////////////////////////
class FlagObj:public MapObj {
public:
    FlagObj (QGraphicsItem *);
    ~FlagObj ();
    virtual void init ();
    virtual void copy (FlagObj*);
    virtual void move (double x,double y);      // move to absolute Position
    virtual void moveBy (double x,double y);    // move to relative Position
    virtual void setZValue (double z);
    virtual void setVisibility(bool);
    void load (const QString&);
    void load (ImageObj* io);
    void setName (const QString&);
    const QString getName ();   // FIXME-0 should become obsolete, only uuid
    void setUuid(const QUuid &uid);
    const QUuid getUuid();
    QPixmap getPixmap();
    void setAction(QAction*);
    void setAlwaysVisible (bool b);
    bool isAlwaysVisible ();
    void saveToDir (const QString&, const QString&);
    
protected:  
    QString name;
    QUuid uid;
    bool avis;
    virtual void positionBBox();
    virtual void calcBBoxSize();
private:
    ImageObj* imageObj;
};

#endif
