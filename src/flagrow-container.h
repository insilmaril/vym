#ifndef FLAGROW_CONTAINER_H
#define FLAGROW_CONTAINER_H

#include "container.h"

class Flag;
class FlagContainer;
class FlagRowMaster;

/*! \brief A collection of flags containers in a map.

   The flags are usually aligned horizontally  in a row on the map.
 */

class FlagRowContainer : public Container {
  public:
    FlagRowContainer();
    ~FlagRowContainer();
    virtual void setZValue(double z);
    void updateActiveFlagContainers(const QList<QUuid>, FlagRowMaster *masterRowMain,
                              FlagRowMaster *masterRowOptional = nullptr);
    bool isFlagActive(const QUuid &);
    void activateFlag(Flag *flag);
    FlagContainer *findFlagContainerByUid(const QUuid &);
    QUuid findFlagByPos(const QPointF &p); // Find flag by position
};
#endif
