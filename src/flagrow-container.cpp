#include <QDebug>

#include "flag.h"
#include "flag-container.h"
#include "flagrow-container.h"
#include "flagrowmaster.h"  // FIXME-2 rename to flagrow-master

extern FlagRowMaster *standardFlagsMaster;
extern FlagRowMaster *userFlagsMaster;
extern FlagRowMaster *systemFlagsMaster;

/////////////////////////////////////////////////////////////////
// FlagRowContainer
/////////////////////////////////////////////////////////////////
FlagRowContainer::FlagRowContainer()
{
    // qDebug() << "Const FlagRowContainer ()";
    type = FlagRowCont;
    layout = Horizontal;
    horizontalDirection = LeftToRight;
}

FlagRowContainer::~FlagRowContainer()
{
    // qDebug() << "Destr FlagRowContainer";
}

void FlagRowContainer::setZValue(double z)  // FIXME-2 z value of children? in base class?
{
    /*
    QGraphicsItem::setZValue(z);
    foreach (QGraphicsItem *child, childItems()) {
        Container* c = (Container*) child;
        c->setZValue(z);
    }
    */
}

void FlagRowContainer::setVisibility(bool v)    // FIXME-1 children visibility?
{
    Container::setVisibility(v);
    /*
    foreach (QGraphicsItem *child, childItems()) {
        Container* c = (Container*) child;
        c->setVisibility(v);
    }
    */
}

void FlagRowContainer::updateActiveFlagContainers(const QList<QUuid> activeFlagUids,
                                      FlagRowMaster *masterRowMain,
                                      FlagRowMaster *masterRowOptional)
{
    // Add missing active flags
    for (int i = 0; i <= activeFlagUids.size() - 1; i++) {
        if (!isFlagActive(activeFlagUids.at(i))) {
            Flag *f = masterRowMain->findFlagByUid(activeFlagUids.at(i));
            if (f) {
                activateFlag(f);
            }
            if (masterRowOptional) {
                f = masterRowOptional->findFlagByUid(activeFlagUids.at(i));
                if (f) {
                    activateFlag(f);
                }
            }
        }
    }

    // Remove flags no longer active in TreeItem
    foreach (QGraphicsItem *child, childItems()) {
        FlagContainer* fc = (FlagContainer*) child;
        if (!activeFlagUids.contains(fc->getUuid())) {
            delete fc;
        }
    }
}

bool FlagRowContainer::isFlagActive(const QUuid &uid)
{
    FlagContainer *fc = findFlagContainerByUid(uid);
    if (fc)
        return true;
    else
        return false;
}

void FlagRowContainer::activateFlag(Flag *flag)
{
    if (flag) {
        FlagContainer *fc = new FlagContainer;
        addContainer(fc);

        // Loading an image  will *copy* it
        // and thus read the flag from cash
        fc->copy(flag->getImageContainer());
        fc->setUuid(flag->getUuid());
        fc->setZValue(QGraphicsItem::zValue());
        fc->setVisibility(visible);
        // qDebug() << "FRC::activateFlag  visible="<< visible << "  Qtvis=" << isVisible();  FIXME-2 testing only
        fc->setVisibility(true);
    }
}

FlagContainer *FlagRowContainer::findFlagContainerByUid(const QUuid &uid)
{
    foreach (QGraphicsItem *child, childItems()) {
        FlagContainer* fc = (FlagContainer*) child;
        if (fc->getUuid() == uid)
            return fc;
    }
    return nullptr;
}

QUuid FlagRowContainer::findFlagUidByPos(const QPointF &p)
{
    if (!boundingRect().contains(p))
        return QUuid();
    foreach (QGraphicsItem *child, childItems()) {
        FlagContainer* fc = (FlagContainer*) child;
        if (fc->boundingRect().contains(p))
        return fc->getUuid();
    }
    return QUuid();
}
