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

BranchContainerBase::~BranchContainerBase()   // FIXME-2 use default
{
    //qDebug() << "* Destr BranchContainerBase" << getName() << this;
}

void BranchContainerBase::init()
{
    // General defaults, also used by BranchContainer
    orientation = UndefinedOrientation;

    imagesContainer = nullptr;
    branchesContainer = nullptr;

    tmpLinkedParentContainer = nullptr;
    originalParentBranchContainer = nullptr;

    setBrush(Qt::NoBrush);
    setPen(QPen(Qt::NoPen));

    // TmpParentContainer defaults, should be overridden from MapDesign later
    containerType = Container::TmpParent;

    setLayout(Container::FloatingReservedSpace);

    branchesContainerAutoLayout = false;
    branchesContainerLayout = Vertical;

    imagesContainerAutoLayout = false;
    imagesContainerLayout = FloatingFree;

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

void BranchContainerBase::setTemporaryLinked(BranchContainer *tpc)
{
    tmpLinkedParentContainer = tpc;
}

void BranchContainerBase::unsetTemporaryLinked()
{
    tmpLinkedParentContainer = nullptr;
}

bool BranchContainerBase::isTemporaryLinked()
{
    if (tmpLinkedParentContainer)
        return true;
    else
        return false;
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

bool BranchContainerBase::hasFloatingBranchesLayout()
{
    if (branchesContainer)
        return branchesContainer->hasFloatingLayout();

    if (branchesContainerLayout == FloatingBounded || branchesContainerLayout == FloatingFree)
        return true;
    else
        return false;
}

bool BranchContainerBase::hasFloatingImagesLayout()
{
    if (imagesContainer)
        return imagesContainer->hasFloatingLayout();

    if (imagesContainerLayout == FloatingBounded || imagesContainerLayout == FloatingFree)
        return true;
    else
        return false;
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

void BranchContainerBase::createImagesContainer()
{
    // imagesContainer is created when images are added to branch
    // The destructor of ImageItem calls 
    // updateChildrenStructure() in parentBranch()
    imagesContainer = new Container ();
    imagesContainer->setContainerType(ImagesContainer);
    imagesContainer->setLayout(imagesContainerLayout);
}

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

void BranchContainerBase::setLayout(const Layout &l)
{
    if (containerType != Branch && containerType != TmpParent)
        qWarning() << "BranchContainerBase::setLayout (...) called for non-branch: " << info();
    Container::setLayout(l);
}

void BranchContainerBase::setImagesContainerLayout(const Layout &ltype)
{
    if (imagesContainerLayout == ltype)
        return;

    imagesContainerLayout = ltype;

    if (imagesContainer)
        imagesContainer->setLayout(imagesContainerLayout);
}

Container::Layout BranchContainerBase::getImagesContainerLayout()
{
    return imagesContainerLayout;
}

void BranchContainerBase::setBranchesContainerLayout(const Layout &layoutNew)
{
    branchesContainerLayout = layoutNew;

    if (branchesContainer)
        branchesContainer->setLayout(branchesContainerLayout);
}

Container::Layout BranchContainerBase::getBranchesContainerLayout()
{
    return branchesContainerLayout;
}

void BranchContainerBase::setBranchesContainerHorizontalAlignment(const HorizontalAlignment &a)
{
    branchesContainerHorizontalAlignment = a;
    if (branchesContainer)
        branchesContainer->setHorizontalAlignment(branchesContainerHorizontalAlignment);
}

void BranchContainerBase::updateBranchesContainerLayout() {}

void BranchContainerBase::reposition() {}

