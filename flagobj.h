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
    virtual void move (double x,double y);      // move to absolute Position
    virtual void moveBy (double x,double y);    // move to relative Position
    virtual void setZValue (double z);
    virtual void setVisibility(bool);
    void loadImage (ImageObj* io);
    void setUuid(const QUuid &uid);
    const QUuid getUuid();
    QPixmap getPixmap();
    void setAction(QAction*);
    void setAlwaysVisible (bool b);
    bool isAlwaysVisible ();
    
protected:  
    QUuid uid;
    bool avis;
    virtual void positionBBox();
    virtual void calcBBoxSize();
private:
    ImageObj* imageObj;
};

#endif
