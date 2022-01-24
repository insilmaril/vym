#include <QDebug>

#include "branch-container.h"

#include "branchitem.h"
#include "heading-container.h"
#include "mapobj.h"

BranchContainer::BranchContainer(QGraphicsScene *scene, QGraphicsItem *parent, BranchItem *bi) : Container(parent)
{
    //qDebug() << "* Const BranchContainer begin this = " << this << "  branchitem = " << bi;
    scene->addItem(this);
    branchItem = bi;
    init();
}

BranchContainer::~BranchContainer()
{
    QString h;
    if (branchItem) h = branchItem->getHeadingPlain();
    //qDebug() << "* Destr BranchContainer" << name << h << this;

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

    setBrush(QColor(Qt::blue));
    setLayoutType(Container::Horizontal);
    //setHorizontalDirection(Container::RightToLeft);
    setHorizontalDirection(Container::LeftToRight);

    headingContainer = new HeadingContainer ();
    headingContainer->setBrush(QColor(Qt::gray));
    scene()->addItem (headingContainer);

    childrenContainer = new Container ();
    childrenContainer->setBrush(QColor(Qt::green));
    childrenContainer->setLayoutType(Container::Vertical);
    childrenContainer->setVerticalAlignment(Container::Left);
    scene()->addItem (childrenContainer);

    innerContainer = new Container ();
    innerContainer->setBrush(QColor(Qt::magenta));
    scene()->addItem (innerContainer);

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

QRectF BranchContainer::getHeadingRect()
{
    QPointF p = headingContainer->scenePos();
    return QRectF(p.x(), p.y(), headingContainer->rect().width(), headingContainer->rect().height());
}

void BranchContainer::setTmpParentContainer(BranchItem* dstBI, QPointF mousePos, int offset)
{
    //qDebug() << "BC::setTmpParentContainer " << dstBI << mousePos << offset;
}

void BranchContainer::unsetTmpParentContainer(QPointF absPos)
{
    qDebug() << " a) BC::unsetTmpParentContainer absPos = " << absPos << " BC=" << this;
    qDebug() << " b) relPos before: " << pos() << "scenePos: " << scenePos();
    setPos (absPos);   // FIXME-2 only for floating layout
    qDebug() << " c) relPos  after: " << pos() << "scenePos: " << scenePos();
}

bool BranchContainer::isInClickBox(const QPointF &p)
{
    return headingContainer->rect().contains(headingContainer->mapFromScene(p));
}

void BranchContainer::updateVisuals()
{
    headingContainer->setText(branchItem->getHeadingText());
}

void BranchContainer::reposition()
{
    // FIXME-2 temporary:   Let mainbranches float. Needs to go to central Layout class later
    if (branchItem && branchItem->depth() == 0)
    {
        qDebug() << "BC::reposition  d=0" << branchItem->getHeadingPlain() << this;
        boundsType = BoundedFloating;
    }
    else
    {
        qDebug() << "BC::reposition  d!=0" << this;
        boundsType = BoundedStacked;
        verticalAlignment = Left;
    }

    Container::reposition();
}

