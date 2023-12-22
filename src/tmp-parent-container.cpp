#include <QDebug>
#include <QGraphicsScene>
#include <math.h>   // FIXME-2 needed?

#include "heading-container.h"
#include "tmp-parent-container.h"

#include "branch-container.h"
#include "geometry.h"   // FIXME-2 needed?
#include "misc.h"   // FIXME-2 needed?

#define qdbg() qDebug().nospace().noquote()

TmpParentContainer::TmpParentContainer()
{
    // qDebug() << "* Const TmpParentContainer begin this = " << this;
    init();
}

void TmpParentContainer::init()
{
    // General defaults, also used by BranchContainer
    orientation = UndefinedOrientation;

    tmpLinkedParentContainer = nullptr;
    originalParentBranchContainer = nullptr;

    // setPen(QPen(Qt::green)); // Uncomment for testing

    // TmpParentContainer defaults, should be overridden from MapDesign later
    containerType = Container::TmpParent;

    setLayout(Container::FloatingReservedSpace);

    branchesContainer = new Container ();
    branchesContainer->setContainerType(Container::BranchesContainer);
    branchesContainer->setParentItem(this); // Different for BranchItem!
}

void TmpParentContainer::addToBranchesContainer(BranchContainer *bc)//FIXME-0000000 selected BC jumps around,esp. for MC and floating layouts
                                                                    //Here HV and BC are mixed up. Jupmps when orient changes while moving
{
    QPointF sp = bc->getHeadingContainer()->mapToScene(QPoint(0,0));
    branchesContainer->addContainer(bc);

    // keep position
    bc->setPos(branchesContainer->sceneTransform().inverted().map(sp));
}

void TmpParentContainer::createImagesContainer()
{
    imagesContainer = new Container ();
    imagesContainer->setContainerType(ImagesContainer);
    imagesContainer->setLayout(Container::FloatingFree);
}

void TmpParentContainer::addToImagesContainer(Container *c)
{
    if (!imagesContainer) {
        createImagesContainer();

        imagesContainer->setParentItem(this);   // Different for BranchItem!
    }

    QPointF sp = c->scenePos();
    imagesContainer->addContainer(c, Z_IMAGE);

    // For TmpParentContainer keep position
    c->setPos(imagesContainer->sceneTransform().inverted().map(sp));
}

void TmpParentContainer::reposition()
{
    qdbg() << ind() << "TPC::reposition tpc=" <<      info() << "  orient=" << orientation;
    /*
    if (pbc)
        qdbg() << ind() << "          pbc=" << pbc->info();
    else
        qdbg() << ind() << "          pbc=0";
    qdbg() << ind() << "          pbc->orientation=" << pbc->orientation;
    */

    switch (orientation) {
        case LeftOfParent:
            qDebug() << "TPC::repos tPC left of parent";
            setHorizontalDirection(RightToLeft);
            branchesContainer->setHorizontalAlignment(AlignedRight);
            break;
        case RightOfParent:
            qDebug() << "TPC::repos tPC right of parent";
            setHorizontalDirection(LeftToRight);
            branchesContainer->setHorizontalAlignment(AlignedLeft);
            break;
        case UndefinedOrientation:
            qWarning() << "TPC::reposition tPC - UndefinedOrientation in " << info();
            break;
        default:
            qWarning() << "TPC::reposition tPC - Unknown orientation " << orientation << " in " << info();
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
