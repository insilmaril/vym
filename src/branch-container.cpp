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
    //qDebug() << "* Destr BranchContainer" << getName() << h << this;

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

    headingContainer = new HeadingContainer ();
    headingContainer->setBrush(Qt::NoBrush);
    headingContainer->setPen(Qt::NoPen);
    scene()->addItem (headingContainer);

    childrenContainer = new Container ();
    childrenContainer->setBrush(Qt::NoBrush);
    childrenContainer->setPen(Qt::NoPen);
    childrenContainer->setLayoutType(Container::Vertical);      // Default, usually depends on depth
    childrenContainer->setVerticalAlignment(Container::Left);   // Default, usually depends on position
    childrenContainer->type = Container::Children;
    scene()->addItem (childrenContainer);

    innerContainer = new Container ();
    innerContainer->setBrush(Qt::NoBrush);
    innerContainer->setPen(Qt::NoPen);
    innerContainer->type = InnerContent;
    scene()->addItem (innerContainer);

    innerContainer->addContainer(headingContainer);
    innerContainer->addContainer(childrenContainer);
    addContainer(innerContainer);

    setBrush(Qt::NoBrush);
    setLayoutType(Container::Horizontal);
    setHorizontalDirection(Container::LeftToRight);
}

void BranchContainer::setBranchItem(BranchItem *bi) { branchItem = bi; }

BranchItem *BranchContainer::getBranchItem() const { return branchItem; }

QString BranchContainer::getName() {
    if (branchItem)
        return Container::getName() + " '" + branchItem->getHeadingPlain() + "'";
    else
        return Container::getName() + " - ?";
}

void BranchContainer::addToChildrenContainer(Container *c, bool keepScenePos)
{
    QPointF sp = c->scenePos();
    c->setParentItem(childrenContainer);
    if (keepScenePos)
        c->setPos(sceneTransform().inverted().map(sp));
}

Container* BranchContainer::getChildrenContainer()
{
    return childrenContainer;
}

HeadingContainer* BranchContainer::getHeadingContainer()
{
    return headingContainer;
}

void BranchContainer::setLayoutType(const LayoutType &ltype)
{
    Container::setLayoutType(ltype);

    if (type != Branch) 
        qWarning() << "BranchContainer::setLayoutType (...) called for !Branch";
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
    if (branchItem)
        headingContainer->setHeading(branchItem->getHeadingText());
}

void BranchContainer::reposition()
{
    // tmpParentContainer has no branchItem, return for now  // FIXME-2 review with xlinks later!
    if (!branchItem) return;

    // Abreviation for depth
    uint depth = branchItem->depth();

    // Set orientation based on depth and if we are floating around
    if (depth == 0)
        orientation = UndefinedOrientation;
    else {
        if (parentContainer()->layout == FloatingBounded || parentContainer()->orientation == UndefinedOrientation) {
            if (pos().x() > 0)
                orientation = RightOfParent;
            else
                orientation = LeftOfParent;
         } else
            // Set same orientation as parent
            setOrientation(parentContainer()->orientation);
    }

    
    // Settings depending on depth
    if (depth == 0)
    {
        // MapCenter

        // qDebug() << "BC::reposition d == 0 " << getName();

        setLayoutType(Horizontal);
        setHorizontalDirection(LeftToRight);
        innerContainer->setHorizontalDirection(RightToLeft);
        childrenContainer->setLayoutType(FloatingBounded);
    } else {
        // Branch or mainbranch
        // qDebug() << "BC::reposition d > 1  orientation=" << orientation << getName();

        setLayoutType(Horizontal);
        childrenContainer->setLayoutType(Vertical);

        switch (orientation) {
            case LeftOfParent:
                setHorizontalDirection(RightToLeft);
                innerContainer->setHorizontalDirection(RightToLeft);
                childrenContainer->setVerticalAlignment(Right);
                break;
            case RightOfParent:
                setHorizontalDirection(LeftToRight);
                innerContainer->setHorizontalDirection(LeftToRight);
                childrenContainer->setVerticalAlignment(Left);
                break;
            default: 
                qWarning() << "BranchContainer::reposition unknown orientation for mainbranch";
                break;
        }
        childrenContainer->orientation = orientation;

        if (branchItem->getHeadingPlain().startsWith("float")) {
            // Special layout: FloatingBounded children 
            orientation = UndefinedOrientation;
            QColor col (Qt::red);
            col.setAlpha(150);
            childrenContainer->setBrush(col);
            childrenContainer->setLayoutType(FloatingBounded);
            innerContainer->setBrush(Qt::cyan);
            innerContainer->setLayoutType(Vertical);
        } 
    }

    Container::reposition();
}

