#include <QDebug>
#include <QGraphicsScene>
#include <math.h>   // FIXME-2 needed?

#include "minimal-branch-container.h"

#include "branch-container.h"
#include "geometry.h"   // FIXME-2 needed?
#include "misc.h"   // FIXME-2 needed?

#define qdbg() qDebug().nospace().noquote()

MinimalBranchContainer::MinimalBranchContainer()
{
    // qDebug() << "* Const MinimalBranchContainer begin this = " << this;
    init();
}

MinimalBranchContainer::~MinimalBranchContainer()   // FIXME-2 use default
{
    //qDebug() << "* Destr MinimalBranchContainer" << getName() << this;
}

void MinimalBranchContainer::init()
{
    containerType = Container::Branch;  // FIXME-2 introduce new type

    orientation = UndefinedOrientation;

    imagesContainer = nullptr;

    branchesContainer = nullptr;

    setLayout(Container::Horizontal);   // FIXME-2
    setHorizontalDirection(Container::LeftToRight);  // FIXME-2

    // Create an uplink for every branch 

    // Set some defaults, should be overridden from MapDesign later
    branchesContainerAutoLayout = true;
    branchesContainerLayout = Vertical;

    imagesContainerAutoLayout = true;
    imagesContainerLayout = FloatingFree;

    // Not temporary linked
    tmpLinkedParentContainer = nullptr;
    originalParentBranchContainer = nullptr;
}

void MinimalBranchContainer::setOrientation(const Orientation &o)
{
    orientation = o;
}

MinimalBranchContainer::Orientation MinimalBranchContainer::getOrientation()
{
    return orientation;
}

void MinimalBranchContainer::setTemporaryLinked(BranchContainer *tpc)
{
    tmpLinkedParentContainer = tpc;
        /*
    if (containerType != TmpParent) {
        updateUpLink(); // FIXME-0 needed?
    }
        */
}

void MinimalBranchContainer::unsetTemporaryLinked()
{
    tmpLinkedParentContainer = nullptr;
}

bool MinimalBranchContainer::isTemporaryLinked()
{
    if (tmpLinkedParentContainer)
        return true;
    else
        return false;
}

int MinimalBranchContainer::childrenCount()
{
    return branchCount() + imageCount();
}

int MinimalBranchContainer::branchCount()
{
    if (!branchesContainer)
        return 0;
    else
        return branchesContainer->childItems().count();
}

bool MinimalBranchContainer::hasFloatingBranchesLayout()
{
    if (branchesContainer)
        return branchesContainer->hasFloatingLayout();

    if (branchesContainerLayout == FloatingBounded || branchesContainerLayout == FloatingFree)
        return true;
    else
        return false;
}

bool MinimalBranchContainer::hasFloatingImagesLayout()
{
    if (imagesContainer)
        return imagesContainer->hasFloatingLayout();

    if (imagesContainerLayout == FloatingBounded || imagesContainerLayout == FloatingFree)
        return true;
    else
        return false;
}


void MinimalBranchContainer::addToBranchesContainer(Container *c, bool keepScenePos)
{
    if (!branchesContainer) {
        // Create branchesContainer before adding to it
        branchesContainer = new Container ();
        branchesContainer->setContainerType(Container::BranchesContainer);
        branchesContainer->setParentItem(this); // Different for BranchItem!
        // FIXME-2 branchesContainer->zPos = Z_BRANCHES;
    }

    QPointF sp = c->scenePos();
    branchesContainer->addContainer(c);

    updateBranchesContainerLayout();

    if (keepScenePos)
        c->setPos(branchesContainer->sceneTransform().inverted().map(sp));
}

Container* MinimalBranchContainer::getBranchesContainer()
{
    return branchesContainer;
}

int MinimalBranchContainer::imageCount()
{
    if (!imagesContainer)
        return 0;
    else
        return imagesContainer->childItems().count();
}

void MinimalBranchContainer::createImagesContainer()
{
    // imagesContainer is created when images are added to branch
    // The destructor of ImageItem calls 
    // updateChildrenStructure() in parentBranch()
    imagesContainer = new Container ();
    imagesContainer->setContainerType(ImagesContainer);
    imagesContainer->setLayout(imagesContainerLayout);
}

void MinimalBranchContainer::addToImagesContainer(Container *c, bool keepScenePos)
{
    if (!imagesContainer) {
        createImagesContainer();
        /* FIXME-2 imagesContainer styles should be updated in Container c, but ImageContainer has no updateStyles() yet
        if (branchItem)
            updateStyles(RelinkBranch);
        */
        imagesContainer->setParentItem(this);   // Different for BranchItem!
    }

    QPointF sp = c->scenePos();
    imagesContainer->addContainer(c, Z_IMAGE);

    if (keepScenePos)
        c->setPos(imagesContainer->sceneTransform().inverted().map(sp));
}

Container* MinimalBranchContainer::getImagesContainer()
{
    return imagesContainer;
}

QList <BranchContainer*> MinimalBranchContainer::childBranches()
{
    QList <BranchContainer*> list;

    if (!branchesContainer) return list;

    foreach (QGraphicsItem *g_item, branchesContainer->childItems())
        list << (BranchContainer*)g_item;

    return list;
}

QList <ImageContainer*> MinimalBranchContainer::childImages()
{
    QList <ImageContainer*> list;

    if (!imagesContainer) return list;

    foreach (QGraphicsItem *g_item, imagesContainer->childItems())
        list << (ImageContainer*)g_item;

    return list;
}

void MinimalBranchContainer::setLayout(const Layout &l)
{
    if (containerType != Branch && containerType != TmpParent)
        qWarning() << "MinimalBranchContainer::setLayout (...) called for non-branch: " << info();
    Container::setLayout(l);
}

void MinimalBranchContainer::setImagesContainerLayout(const Layout &ltype)
{
    if (imagesContainerLayout == ltype)
        return;

    imagesContainerLayout = ltype;

    if (imagesContainer)
        imagesContainer->setLayout(imagesContainerLayout);
}

Container::Layout MinimalBranchContainer::getImagesContainerLayout()
{
    return imagesContainerLayout;
}

void MinimalBranchContainer::setBranchesContainerLayout(const Layout &layoutNew)
{
    branchesContainerLayout = layoutNew;

    if (branchesContainer)
        branchesContainer->setLayout(branchesContainerLayout);
}

Container::Layout MinimalBranchContainer::getBranchesContainerLayout()
{
    return branchesContainerLayout;
}

void MinimalBranchContainer::setBranchesContainerHorizontalAlignment(const HorizontalAlignment &a)
{
    branchesContainerHorizontalAlignment = a;
    if (branchesContainer)
        branchesContainer->setHorizontalAlignment(branchesContainerHorizontalAlignment);
}

void MinimalBranchContainer::setBranchesContainerBrush(const QBrush &b)
{
    branchesContainerBrush = b;
    if (branchesContainer)
        branchesContainer->setBrush(branchesContainerBrush);
}

void MinimalBranchContainer::updateBranchesContainerLayout()
{
    // Set container layouts
    setBranchesContainerLayout(branchesContainerLayout);
}

void MinimalBranchContainer::reposition()
{
    qdbg() << ind() << "MBC::reposition mbc=" <<      info() << "  orient=" << orientation;
    /*
    if (pbc)
        qdbg() << ind() << "          pbc=" << pbc->info();
    else
        qdbg() << ind() << "          pbc=0";
    qdbg() << ind() << "          pbc->orientation=" << pbc->orientation;
    */

    switch (orientation) {
        case LeftOfParent:
            //qDebug() << "BC::repos tPC left of parent";
            setHorizontalDirection(RightToLeft);
            setBranchesContainerHorizontalAlignment(AlignedRight);
            break;
        case RightOfParent:
            //qDebug() << "BC::repos tPC right of parent";
            setHorizontalDirection(LeftToRight);
            setBranchesContainerHorizontalAlignment(AlignedLeft);
            break;
        case UndefinedOrientation:
            qWarning() << "MBC::reposition tPC - UndefinedOrientation in " << info();
            break;
        default:
            qWarning() << "MBC::reposition tPC - Unknown orientation " << orientation << " in " << info();
            break;
    }

    Container::reposition();

    // Update links of children
    if (branchesContainer && branchCount() > 0) {
        foreach (Container *c, branchesContainer->childContainers()) {
            BranchContainer *bc = (BranchContainer*) c;
            bc->updateUpLink();
        }
    }
}
