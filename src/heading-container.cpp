#include <QDebug>

#include "heading-container.h"

#include "branchitem.h"
#include "mapobj.h"

HeadingContainer::HeadingContainer(QGraphicsItem *parent, BranchItem *bi) : Container(parent)
{
    qDebug() << "* Const HeadingContainer begin this = " << this << "  branchitem = " << bi;
    branchItem = bi;
    init();
}

HeadingContainer::~HeadingContainer()
{
    QString h;
    if (branchItem) h = branchItem->getHeadingPlain();
    qDebug() << "* Destr HeadingContainer" << name << h << this;

    if (branchItem)
    {
        // Unlink containers in my own subtree from related treeItems
        // That results in deleting all containers in subtree first 
        // and later deleting subtree of treeItems
        branchItem->unlinkBranchContainer();
    }
}

void HeadingContainer::init()
{
    type = Container::Heading;
}

void HeadingContainer::copy(Container *other)  // FIXME-0 not implemented
{
}

void HeadingContainer::setBranchItem(BranchItem *bi) { branchItem = bi; }

BranchItem *HeadingContainer::getBranchItem() const { return branchItem; }

