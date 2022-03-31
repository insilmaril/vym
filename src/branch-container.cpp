#include <QDebug>
#include <math.h>

#include "branch-container.h"

#include "branchitem.h"
#include "heading-container.h"

BranchContainer::BranchContainer(QGraphicsScene *scene, QGraphicsItem *parent, BranchItem *bi) : Container(parent)  // FIXME-2 scene and addItem should not be required, only for mapCenters without parent:  setParentItem automatically sets scene!
{
    //qDebug() << "* Const BranchContainer begin this = " << this << "  branchitem = " << bi;
    scene->addItem(this);
    branchItem = bi;
    init();
}

BranchContainer::~BranchContainer()
{
    //qDebug() << "* Destr BranchContainer" << getName() << this;

    if (branchItem)
    {
        // Unlink all containers in my own subtree, which will be deleted
        // when tree of QGraphicsItems is deleted.
        // In every destructor tell the linked BranchItem to longer consider deleting containers
        // when the BranchItem will be deleted later
        branchItem->unlinkBranchContainer();
    }
}

void BranchContainer::init()
{
    type = Container::Branch;

    headingContainer = new HeadingContainer ();
    headingContainer->setBrush(Qt::NoBrush);
    headingContainer->setPen(Qt::NoPen);

    imagesContainer = new Container ();
    imagesContainer->setBrush(Qt::NoBrush);
    imagesContainer->setPen(Qt::NoPen);
    imagesContainer->setLayoutType(Container::FloatingFree);
    imagesContainer->type = Container::ImageCollection;

    branchesContainer = new Container ();
    branchesContainer->setBrush(Qt::NoBrush);
    branchesContainer->setPen(Qt::NoPen);
    branchesContainer->setLayoutType(Container::Vertical);              // Default, usually depends on depth
    branchesContainer->setVerticalAlignment(Container::AlignedLeft);    // Default, usually depends on position
    branchesContainer->type = Container::BranchCollection;

    innerContainer = new Container ();
    innerContainer->setBrush(Qt::NoBrush);
    innerContainer->setPen(Qt::NoPen);
    innerContainer->type = InnerContent;

    // Adding the containers will reparent them and thus set scene
    innerContainer->addContainer(headingContainer);
    innerContainer->addContainer(imagesContainer);
    innerContainer->addContainer(branchesContainer);
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

void BranchContainer::addToBranchesContainer(Container *c, bool keepScenePos)
{
    QPointF sp = c->scenePos();
    c->setParentItem(branchesContainer);
    if (keepScenePos)
        c->setPos(sceneTransform().inverted().map(sp));
}

void BranchContainer::addToImagesContainer(Container *c, bool keepScenePos)
{
    QPointF sp = c->scenePos();
    c->setParentItem(imagesContainer);
    if (keepScenePos)
        c->setPos(sceneTransform().inverted().map(sp));
}

Container* BranchContainer::getBranchesContainer()
{
    return branchesContainer;
}

Container* BranchContainer::getImagesContainer()
{
    return imagesContainer;
}

HeadingContainer* BranchContainer::getHeadingContainer()
{
    return headingContainer;
}

QPointF BranchContainer::getChildrenPosHint(Container *c)
{
    QRectF r = headingContainer->rect();

    int n = 0;
    qreal radius;
    if (c->type == Branch) {
        radius = 190;
        n = branchesContainer->childItems().count();
    } else if (c->type == Image) {
        radius = 100;
        n = imagesContainer->childItems().count();
    }

    if (!parentItem() || c->type == Image) {
        // Mapcenter, suggest to put image or mainbranch on circle around center
        qreal a =
            -M_PI_4 + M_PI_2 * n + (M_PI_4 / 2) * (n / 4 % 4);
        return QPointF (radius * cos(a), radius * sin(a));
    }

    switch (orientation) {
        case LeftOfParent:
            return QPointF(r.left() - c->rect().width(), r.center().y());
            break;
        case RightOfParent:
            return QPointF(r.right(), r.center().y());
            break;
        default:
            return QPointF(r.left(), r.center().y());
            break;
    }
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

    setLayoutType(Horizontal);
    
    // Settings depending on depth
    if (depth == 0)
    {
        // MapCenter

        // qDebug() << "BC::reposition d == 0 " << getName();
        setHorizontalDirection(LeftToRight);
        innerContainer->setHorizontalDirection(RightToLeft);
        innerContainer->setMovableByFloats(false);
        setMovableByFloats(false);  // FIXME-2 Needed?
        branchesContainer->setLayoutType(FloatingBounded);

        //FIXME-2 add "flags" and origin for testing (but only once!)
        if (innerContainer->childItems().count() <= 3) {
            HeadingContainer *fc = new HeadingContainer();
            fc->setHeading("Flags");
            innerContainer->addContainer(fc);
        }

    } else {
        // Branch or mainbranch
        
        // qDebug() << "BC::reposition d > 1  orientation=" << orientation << getName();
        innerContainer->setMovableByFloats(true);
        branchesContainer->setLayoutType(Vertical);

        switch (orientation) {
            case LeftOfParent:
                setHorizontalDirection(RightToLeft);
                innerContainer->setHorizontalDirection(RightToLeft);
                branchesContainer->setVerticalAlignment(AlignedRight);
                break;
            case RightOfParent:
                setHorizontalDirection(LeftToRight);
                innerContainer->setHorizontalDirection(LeftToRight);
                branchesContainer->setVerticalAlignment(AlignedLeft);
                break;
            default: 
                qWarning() << "BranchContainer::reposition unknown orientation for mainbranch";
                break;
        }
        branchesContainer->orientation = orientation;

        if (branchItem->getHeadingPlain().startsWith("float")) {
            // Special layout: FloatingBounded children 
            orientation = UndefinedOrientation;
            QColor col (Qt::red);
            col.setAlpha(150);
            branchesContainer->setBrush(col);
            branchesContainer->setLayoutType(FloatingBounded);
            innerContainer->setBrush(Qt::cyan);
        } else if (branchItem->getHeadingPlain().startsWith("free")) {
            // Special layout: FloatingBounded children 
            orientation = UndefinedOrientation;
            QColor col (Qt::red);
            col.setAlpha(120);
            branchesContainer->setBrush(col);
            branchesContainer->setLayoutType(FloatingFree);
            innerContainer->setBrush(Qt::gray);
        } 
    }

    Container::reposition();
}

