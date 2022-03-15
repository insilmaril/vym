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

void BranchContainer::setLayoutType(const LayoutType &ltype)
{
    Container::setLayoutType(ltype);

    if (type != Branch) 
        qWarning() << "BranchContainer::setLayoutType (Floating) called for !Branch";
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
        headingContainer->setText(branchItem->getHeadingText());
}

// FIXME-0 BrancContainer - wrong horizontalDirection below "float" if left of center
void BranchContainer::reposition()  // FIXME-0 Concept for floating mainbranches and orientation
{
    if (branchItem)
    {
        bool leftOfCenter;
        if (branchItem->depth() == 0)
        {
            // MapCenter

            qDebug() << "BC::reposition d == 0 " << getName() << this << 
                "children: " << childrenContainer;

            setLayoutType(Horizontal);
            setHorizontalDirection(LeftToRight);
            //innerContainer->setHorizontalDirection(LeftToRight);
            innerContainer->setHorizontalDirection(RightToLeft);
            //childrenContainer->setVerticalAlignment(Left);
            //childrenContainer->setLayoutType(Vertical);
            childrenContainer->setLayoutType(Floating);
        } else if (branchItem->depth() == 1) {
            // MainBranch

            if (pos().x() < 0) 
                leftOfCenter = true;
            else
                leftOfCenter = false;

            qDebug() << "BC::reposition d == 1  loc=" << 
                leftOfCenter << info() << this;

            setLayoutType(Horizontal);
            childrenContainer->setLayoutType(Vertical);

            if (leftOfCenter) {
                // Left of center
                setHorizontalDirection(RightToLeft);
                innerContainer->setHorizontalDirection(RightToLeft);
                childrenContainer->setVerticalAlignment(Right);
            } else {
                // Right of center
                setHorizontalDirection(LeftToRight);
                innerContainer->setHorizontalDirection(LeftToRight);
                childrenContainer->setVerticalAlignment(Left);
            }
        } else {
            // Branch deeper in tree

            leftOfCenter = branchItem->parentBranch()->getBranchContainer()->getHorizontalDirection(); // FIXME-2 typechange :-(
            if (branchItem->getHeadingPlain().startsWith("float")) {
                // Special layout: floating children 
                qDebug() << "BC::reposition d > 1  FLOATING begin loc" << 
                    leftOfCenter << info();
                childrenContainer->setLayoutType(Floating);
                innerContainer->setBrush(Qt::cyan);
            } else if (branchItem->getHeadingPlain().startsWith("vert")) {
                qDebug() << "BC::reposition d > 1  VERTICAL begin loc" << 
                    leftOfCenter << info();
                childrenContainer->setLayoutType(Vertical);
                childrenContainer->setBrush(Qt::gray);
                innerContainer->setLayoutType(Vertical);
                innerContainer->setVerticalAlignment(Left);
                innerContainer->setBrush(Qt::green);
            } else {
                // Normal layout
                qDebug() << "BC::reposition d > 1  loc=" << 
                    leftOfCenter << info();

                setLayoutType(Container::Horizontal);
                childrenContainer->setLayoutType(Vertical);

                if (leftOfCenter) {
                    // Left of center
                    innerContainer->setHorizontalDirection(RightToLeft);
                    childrenContainer->setVerticalAlignment(Right);
                } else {
                    // Right of center
                    innerContainer->setHorizontalDirection(LeftToRight);
                    childrenContainer->setVerticalAlignment(Left);
                }
            }   // Normal layout
        }
    } //else
        //qDebug() << "BC::reposition  no branchItem!!!!  tmpParentContainer?  this = " << this;

    Container::reposition();

    // Aftermath, position some containers if we have a floating layout
    /*
    if (branchItem)
    {
        if (branchItem->depth() > 1 && branchItem->getHeadingPlain() == "float") {
            qDebug() << "BC::reposition d > 1  FLOATING end   " << 
                getName() << this << 
                "children: " << childrenContainer << "  children->ct=" << childrenContainer->ct;

            innerContainer->moveBy(childrenContainer->ct.x(), childrenContainer->ct.y());
            childrenContainer->moveBy(childrenContainer->ct.x(), childrenContainer->ct.y());
        }
    }
    */
}

