#ifndef FLAGROWOBJ_H
#define FLAGROWOBJ_H

#include <QMainWindow>

//#include "mapobj.h"
#include "flagobj.h"

class Flag;

/*! \brief A collection of flags (FlagObj) in a map.

   The flags are aligned horizontally  in a row on the map.
 */

class FlagRowObj : public MapObj {
  public:
    FlagRowObj();
    FlagRowObj(QGraphicsItem *);
    ~FlagRowObj();
    virtual void move(double, double);
    virtual void moveBy(double, double);
    virtual void setZValue(double z);
    virtual void setVisibility(bool);
    void updateActiveFlagObjs(const QList<QUuid>, FlagRowMaster *masterRowMain,
                              FlagRowMaster *masterRowOptional = NULL);
    virtual void positionBBox();
    virtual void calcBBoxSize();
    bool isFlagActive(const QUuid &);
    void activateFlag(Flag *flag);
    FlagObj *findFlagObjByUid(const QUuid &);
    virtual QUuid findFlagUidByPos(const QPointF &p); // Find flag by position
  private:
    QList<FlagObj *> flagobjs;
    bool showFlags; // FloatObjects want to hide their flags
};
#endif
