#ifndef FLAGOBJ_H
#define FLAGOBJ_H


#include <QAction>
#include <QPixmap>

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
    void load (const QPixmap&);
    void setName (const QString&);
    const QString getName ();
    QPixmap getPixmap();
    void setAction(QAction*);
    void setAlwaysVisible (bool b);
    bool isAlwaysVisible ();
    bool isActive();
    void toggle();
    void activate();
    void deactivate();
    void saveToDir (const QString&, const QString&);
    
protected:  
    QString name;
    bool state;
    bool avis;
    virtual void positionBBox();
    virtual void calcBBoxSize();
private:
    ImageObj* icon;
};

#endif
