#include <QDebug>
#include <QGraphicsScene>
#include <math.h>   // FIXME-2 needed?

#include "branch-container-base.h"

#include "branch-container.h"
#include "geometry.h"   // FIXME-2 needed?
#include "misc.h"   // FIXME-2 needed?

#define qdbg() qDebug().nospace().noquote()

BranchContainerBase::BranchContainerBase()
{
    // qDebug() << "* Const BranchContainerBase begin this = " << this;
    init();
}

void BranchContainerBase::init()
{
    // General defaults, also used by BranchContainer
    orientation = UndefinedOrientation;

    imagesContainer = nullptr;
    branchesContainer = nullptr;

    movingStateInt = NotMoving;
    tmpLinkedParentContainer = nullptr;
    originalParentBranchContainer = nullptr;

    setBrush(Qt::NoBrush);
    setPen(QPen(Qt::NoPen));

    // TmpParentContainer defaults, should be overridden from MapDesign later
    containerType = Container::TmpParent;

    setLayout(Container::FloatingReservedSpace);

    horizontalDirection = Container::LeftToRight;
}

void BranchContainerBase::setOrientation(const Orientation &o)
{
    orientation = o;
}

BranchContainerBase::Orientation BranchContainerBase::getOrientation()
{
    return orientation;
}

void BranchContainerBase::setMovingState(const MovingState &ms, BranchContainer *tpc)
{
    movingStateInt = ms;
    tmpLinkedParentContainer = tpc;
}

BranchContainerBase::MovingState  BranchContainerBase::movingState()
{
    return movingStateInt;
}

int BranchContainerBase::childrenCount()
{
    return branchCount() + imageCount();
}

int BranchContainerBase::branchCount()
{
    if (!branchesContainer)
        return 0;
    else
        return branchesContainer->childItems().count();
}

void BranchContainerBase::addToBranchesContainer(Container *c) {}

Container* BranchContainerBase::getBranchesContainer()
{
    return branchesContainer;
}

int BranchContainerBase::imageCount()
{
    if (!imagesContainer)
        return 0;
    else
        return imagesContainer->childItems().count();
}

void BranchContainerBase::createImagesContainer() {}

void BranchContainerBase::addToImagesContainer(Container *c) {}

Container* BranchContainerBase::getImagesContainer()
{
    return imagesContainer;
}

QList <BranchContainer*> BranchContainerBase::childBranches()
{
    QList <BranchContainer*> list;

    if (!branchesContainer) return list;

    foreach (QGraphicsItem *g_item, branchesContainer->childItems())
        list << (BranchContainer*)g_item;

    return list;
}

QList <ImageContainer*> BranchContainerBase::childImages()
{
    QList <ImageContainer*> list;

    if (!imagesContainer) return list;

    foreach (QGraphicsItem *g_item, imagesContainer->childItems())
        list << (ImageContainer*)g_item;

    return list;
}


void BranchContainerBase::reposition() {}

