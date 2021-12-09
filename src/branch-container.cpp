#include <QDebug>

#include "branch-container.h"

#include "branchitem.h"
#include "heading-container.h"
#include "mapobj.h"

BranchContainer::BranchContainer(QGraphicsScene *scene, QGraphicsItem *parent, BranchItem *bi) : Container(parent)
{
    qDebug() << "* Const BranchContainer begin this = " << this << "  branchitem = " << bi;
    scene->addItem(this);
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

    setName("branch");
    setBrush(QColor(Qt::blue));
    setContentType(Container::Containers);
    setLayoutType(Container::Horizontal);
    setHorizontalDirection(Container::RightToLeft);

    headingContainer = new HeadingContainer ();
    headingContainer->setName("heading");
    headingContainer->setBrush(QColor(Qt::gray));
    scene()->addItem (headingContainer);

    childrenContainer = new Container ();
    childrenContainer->setName("children");
    childrenContainer->setBrush(QColor(Qt::green));
    childrenContainer->setContentType(Container::Containers);
    childrenContainer->setLayoutType(Container::Vertical);
    scene()->addItem (childrenContainer);

    addContainer(headingContainer);
    addContainer(childrenContainer);
}

void BranchContainer::setBranchItem(BranchItem *bi) { branchItem = bi; }

BranchItem *BranchContainer::getBranchItem() const { return branchItem; }

void BranchContainer::addToChildrenContainer(Container *c)
{
    c->setParentItem(childrenContainer);
}

Container* BranchContainer::getChildrenContainer()
{
    return childrenContainer;
}

bool BranchContainer::isInClickBox(const QPointF &p)
{
    return headingContainer->rect().contains(headingContainer->mapFromScene(p));
}

void BranchContainer::updateVisuals()
{
    headingContainer->setText(branchItem->getHeadingText());
}


