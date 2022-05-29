#include <QDebug>
#include <math.h>

#include "branch-container.h"

#include "branchitem.h"
#include "geometry.h"
#include "heading-container.h"

qreal BranchContainer::linkWidth = 20;

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

    orientation = UndefinedOrientation;

    imagesContainer = nullptr;

    headingContainer = new HeadingContainer ();
    headingContainer->setBrush(Qt::NoBrush);
    headingContainer->setPen(Qt::NoPen);

    innerContainer = new Container ();
    innerContainer->setBrush(Qt::NoBrush);
    innerContainer->setPen(Qt::NoPen);
    innerContainer->type = InnerContent;

    // Adding the containers will reparent them and thus set scene
    innerContainer->addContainer(headingContainer);

    //branchesContainer = nullptr;
    createBranchesContainer();  // FIXME-0  soon only create on demand

    addContainer(innerContainer);

    setBrush(Qt::NoBrush);
    setLayoutType(Container::Horizontal);
    setHorizontalDirection(Container::LeftToRight);

    temporaryLinked = false;
}

BranchContainer* BranchContainer::parentBranchContainer()
{
    Container *p = parentContainer();
    if (!p) return nullptr;;

    // Some checks before the real parent can be returned
    if (p->type != BranchCollection) return nullptr;

    p = p->parentContainer();

    if (p->type != InnerContent) return nullptr;

    p = p->parentContainer();

    if (! (p->type == Branch || p->type == TmpParent)) return nullptr;

    return (BranchContainer*)p;
}

void BranchContainer::setBranchItem(BranchItem *bi) { branchItem = bi; }

BranchItem *BranchContainer::getBranchItem() const { return branchItem; }

QString BranchContainer::getName() {
    if (branchItem)
        return Container::getName() + " '" + branchItem->getHeadingPlain() + "'";
    else
        return Container::getName() + " - ?";
}

void BranchContainer::setOrientation(const Orientation &m)
{
    orientation = m;
}

void BranchContainer::setOriginalOrientation()
{
    originalOrientation = orientation;
}

BranchContainer::Orientation BranchContainer::getOrientation()
{
    return orientation;
}

void BranchContainer::setTemporaryLinked(bool b)
{
    temporaryLinked = b;
}

bool BranchContainer::isTemporaryLinked()
{
    return temporaryLinked;
}

void BranchContainer::createBranchesContainer()
{
    branchesContainer = new Container ();
    branchesContainer->setBrush(Qt::NoBrush);
    branchesContainer->setPen(Qt::NoPen);
    branchesContainer->setLayoutType(Container::Vertical);              // Default, usually depends on depth
    branchesContainer->setVerticalAlignment(Container::AlignedLeft);    // Default, usually depends on position
    branchesContainer->type = Container::BranchCollection;
    innerContainer->addContainer(branchesContainer);
}

void BranchContainer::addToBranchesContainer(Container *c, bool keepScenePos)
{
    QPointF sp = c->scenePos();
    c->setParentItem(branchesContainer);
    if (keepScenePos)
        c->setPos(branchesContainer->sceneTransform().inverted().map(sp));
}

Container* BranchContainer::getBranchesContainer()
{
    return branchesContainer;
}

void BranchContainer::createImagesContainer()
{
    imagesContainer = new Container ();
    imagesContainer->setBrush(Qt::NoBrush);
    imagesContainer->setPen(Qt::NoPen);
    imagesContainer->setLayoutType(Container::FloatingFree);
    imagesContainer->type = Container::ImageCollection;
    innerContainer->addContainer(imagesContainer);

    if (branchesContainer)
        imagesContainer->stackBefore(branchesContainer);
}

void BranchContainer::addToImagesContainer(Container *c, bool keepScenePos)
{
    if (!imagesContainer) {
        createImagesContainer();
    }

    QPointF sp = c->scenePos();
    c->setParentItem(imagesContainer);
    if (keepScenePos)
        c->setPos(imagesContainer->sceneTransform().inverted().map(sp));

}

Container* BranchContainer::getImagesContainer()
{
    return imagesContainer;
}

HeadingContainer* BranchContainer::getHeadingContainer()
{
    return headingContainer;
}

QList <BranchContainer*> BranchContainer::childBranches()
{
    QList <BranchContainer*> list;

    if (!branchesContainer) return list;

    foreach (QGraphicsItem *g_item, branchesContainer->childItems())
        list << (BranchContainer*)g_item;

    return list;
}

QList <ImageContainer*> BranchContainer::childImages()
{
    QList <ImageContainer*> list;

    if (!imagesContainer) return list;

    foreach (QGraphicsItem *g_item, imagesContainer->childItems())
        list << (ImageContainer*)g_item;

    return list;
}

QPointF BranchContainer::getPositionHintNewChild(Container *c)
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

QPointF BranchContainer::getPositionHintRelink(Container *c, int d_pos, const QPointF &p_scene) // FIXME-1 not working correctly with multiple selected branches
{
    QPointF hint;

    QRectF r = headingContainer->rect();

    Container *targetContainer;

    switch (c->type) {
        case TmpParent:
            // So far this method is only called when tmpParentContainer is temporarily relinked
            if (childImages().count() > 0)
                // If there are images in tPC, just use their layout for now
                targetContainer = imagesContainer;
            else
                targetContainer = branchesContainer;
            break;
        case Branch: 
            targetContainer = branchesContainer;
            break;
        case Image:
            targetContainer = imagesContainer;
            break;
        default:
            qWarning() << "BranchContainer::getPositionHintRelink Unknown container type!";
            return QPointF();
    }

    if (targetContainer->hasFloatingLayout()) {
        // Floating layout, position on circle around center of myself
        qreal radius = Geometry::distance(r.center(), r.topLeft()) + 20;

        QPointF center = mapToScene(r.center());
        qreal a = getAngle(p_scene - center);
        hint = center + QPointF (radius * cos(a), - radius * sin(a));
    } else {
        // Regular layout
        qreal y;
        if (d_pos == 0)
            y =branchesContainer->rect().bottom();
        else
            y =branchesContainer->rect().bottom() - d_pos * c->rect().height();

        switch (orientation) {
            case LeftOfParent:
                hint = QPointF(-linkWidth + branchesContainer->rect().left() - c->rect().width(), y);
                break;
            default:
                hint = QPointF( linkWidth + r.right(), y);
                break;
        }
        hint = headingContainer->mapToScene(hint);
    }

    return hint;
}

void BranchContainer::setLayoutType(const LayoutType &ltype)
{
    if (type != Branch && type != TmpParent) 
        qWarning() << "BranchContainer::setLayoutType (...) called for non-branch: " << info();
    Container::setLayoutType(ltype);
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
    //qDebug() << "BC::reposition " << info();

    // Abreviation for depth
    uint depth;
    if (branchItem)
        depth = branchItem->depth();
    else
        // tmpParentContainer has no branchItem
        depth = 0;  

    // Set orientation based on depth and if we are floating around or 
    // in the process of being (temporary) relinked
    BranchContainer *pbc = parentBranchContainer();

    if (!pbc && type != TmpParent) {
        // I am a (not moving) mapCenter
        orientation = UndefinedOrientation;
    } else {
        if (type != TmpParent) {
            // I am not the tmpParentContainer
            if (pbc->type == TmpParent) {
                // I am currently moved around in MapEditor
                if (pbc->isTemporaryLinked()) {
                    // tmpParentContainer is currently tmpLinked
                    // use orientation from temporaryTarget, which is als in tmpParentContainer
                    orientation = pbc->orientation;
                } else {
                    // tmpParentContainer moved around, but no target yet
                    // use originalOrientation
                    orientation = originalOrientation;
                }
            } else {
                // "regular repositioning", not currently moved in MapEditor
                if (depth == 0) 
                    orientation = UndefinedOrientation;
                else {
                    if (parentBranchContainer()->orientation == UndefinedOrientation) {
                        // Parent is tmpParentContainer or mapCenter
                        // use relative position to determine orientation
                        if (pos().x() > 0)
                            orientation = RightOfParent;
                        else
                            orientation = LeftOfParent;
                    } else {
                        // Set same orientation as parent
                        setOrientation(parentBranchContainer()->orientation);
                    }
                }
            }   // regular repositioning
        } // else:
        // The "else" here would be that I'm the tmpParentContainer, but
        // then my orientation is already set in MapEditor, so ignore here
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
        if (type != TmpParent && innerContainer->childItems().count() <= 3) {
            HeadingContainer *fc = new HeadingContainer();
            fc->setHeading("Flags");
            innerContainer->addContainer(fc);
        }

    } else {
        // Branch or mainbranch
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
                // TmpParent - orientation is set in MapEditor while moving objects
                break;
        }
        // FIXME-2 no longer needed with orientation part of BC: branchesContainer->orientation = orientation;

        if (branchItem && branchItem->getHeadingPlain().startsWith("float")) {
            // Special layout: FloatingBounded children 
            orientation = UndefinedOrientation;
            QColor col (Qt::red);
            col.setAlpha(150);
            branchesContainer->setBrush(col);
            branchesContainer->setLayoutType(FloatingBounded);
            innerContainer->setBrush(Qt::cyan);
        } else if (branchItem && branchItem->getHeadingPlain().startsWith("free")) {
            // Special layout: FloatingBounded children 
            orientation = UndefinedOrientation;
            QColor col (Qt::red);
            col.setAlpha(120);
            branchesContainer->setBrush(col);
            branchesContainer->setLayoutType(FloatingFree);
            innerContainer->setBrush(Qt::gray);
        } 
        else { // FIXME-1 testing
            QColor col (Qt::blue);
            col.setAlpha(120);
            branchesContainer->setBrush(col);
            //innerContainer->setBrush(Qt::cyan);
        }
    }

    qDebug() << "  orientation =" << orientation << "of " << getName();
    Container::reposition();
}
