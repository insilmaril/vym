#include <QDebug>

#include "branch-container.h"

#include "branchitem.h"
#include "mapobj.h"

BranchContainer::BranchContainer(QGraphicsItem *parent, BranchItem *bi) : Container(parent)
{
    qDebug() << "* Const BranchContainer begin this = " << this << "  branchitem = " << bi;
    branchItem = bi;
    init();
}

BranchContainer::~BranchContainer()
{
    QString h;
    if (branchItem) h = branchItem->getHeadingPlain();
    qDebug() << "* Destr BranchContainer" << name << h << this;

    if (branchItem)
    {
        // Unlink containers in my own subtree from related treeItems
        // That results in deleting all containers in subtree first 
        // and later deleting subtree of treeItems
        branchItem->unlinkBranchContainer();
    }
}

void BranchContainer::init()
{
    type = Container::Branch;
}

void BranchContainer::copy(Container *other)  // FIXME-0 not implemented
{
}

void BranchContainer::setBranchItem(BranchItem *bi) { branchItem = bi; }

BranchItem *BranchContainer::getBranchItem() const { return branchItem; }

