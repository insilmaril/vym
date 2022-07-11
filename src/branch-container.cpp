#include <QDebug>
#include <math.h>

#include "branch-container.h"

#include "branchitem.h"
#include "geometry.h"
#include "heading-container.h"
#include "link-container.h"

qreal BranchContainer::linkWidth = 20;  // FIXME-2 testing

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

    ornamentsContainer = new Container ();
    ornamentsContainer->type = Ornaments;
    // FIXME-2 not available atm ornamentsContainer->setVerticalAligment(" centered ");

    linkContainer = new LinkContainer();

    innerContainer = new Container ();
    innerContainer->type = InnerContent;


    // Adding the containers will reparent them and thus set scene
    // FIXME-2 ornamentsContainer->addContainer(systemFlagsContainer);
    // FIXME-2 ornamentsContainer->addContainer(userFlagsContainer);
    ornamentsContainer->addContainer(linkContainer);
    ornamentsContainer->addContainer(headingContainer);

    innerContainer->addContainer(ornamentsContainer);

    branchesContainer = nullptr;
    linkSpaceContainer = nullptr;

    addContainer(innerContainer);

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

void BranchContainer::setOriginalOrientation()  // FIXME-1 sets also original parent, should be part of setOriginalPos, which should be in LinkContainer
{
    originalOrientation = orientation;
    originalFloating = isFloating();
    if (parentItem()) {
        originalParentPos = parentItem()->scenePos();
        //qDebug() << "BC:setOrient of " << info();
        //qDebug() << "        parent: " << ((Container*)parentItem())->info() <<originalParentPos;
    }
}

BranchContainer::Orientation BranchContainer::getOriginalOrientation()
{
    return originalOrientation;
}

BranchContainer::Orientation BranchContainer::getOrientation()
{
    return orientation;
}

QPointF BranchContainer::getOriginalParentPos()
{
    return originalParentPos;
}

bool BranchContainer::isOriginalFloating()
{
    return originalFloating;
}

void BranchContainer::setTemporaryLinked()
{
    temporaryLinked = true;
}

void BranchContainer::unsetTemporaryLinked()
{
    temporaryLinked = false;
}

bool BranchContainer::isTemporaryLinked()
{
    return temporaryLinked;
}

int BranchContainer::childrenCount()
{
    return branchCount() + imageCount();
}

int BranchContainer::branchCount()
{
    if (!branchesContainer)
        return 0;
    else
        return branchesContainer->childItems().count();
}

bool BranchContainer::hasFloatingBranchesLayout()
{
    if (branchesContainer)
        return branchesContainer->hasFloatingLayout();

    if (branchesContainerLayoutType == FloatingBounded || branchesContainerLayoutType == FloatingFree)
        return true;
    else
        return false;
}

void BranchContainer::createBranchesContainer()
{
    branchesContainer = new Container ();
    branchesContainer->setLayoutType(branchesContainerLayoutType);
    branchesContainer->setHorizontalAlignment(branchesContainerHorizontalAlignment);
    branchesContainer->type = Container::BranchCollection;

    innerContainer->addContainer(branchesContainer);
}

void BranchContainer::addToBranchesContainer(Container *c, bool keepScenePos)
{
    if (!branchesContainer)
        createBranchesContainer();

    QPointF sp = c->scenePos();
    c->setParentItem(branchesContainer);
    if (keepScenePos)
        c->setPos(branchesContainer->sceneTransform().inverted().map(sp));
}

Container* BranchContainer::getBranchesContainer()
{
    return branchesContainer;
}

void BranchContainer::updateBranchesContainer()
{
    if (branchCount() == 0) {
        // no children branches, remove unused containers
        if (linkSpaceContainer) {
            delete linkSpaceContainer;
            linkSpaceContainer = nullptr;
        }
        if (branchesContainer) {
            delete branchesContainer;
            branchesContainer = nullptr;
        }
    } else {
        // Create containers (if required)
        if (!branchesContainer) 
            createBranchesContainer();

        // Space for links depends on layout:
        if (linkSpaceContainer) {
            if (hasFloatingBranchesLayout()) {
                delete linkSpaceContainer;
                linkSpaceContainer = nullptr;
            }
        } else {
            if (!hasFloatingBranchesLayout()) {
                linkSpaceContainer = new HeadingContainer ();
                linkSpaceContainer->setHeading(" - ");  // FIXME-2 introduce minWidth later in Container instead of a pseudo heading here

                innerContainer->addContainer(linkSpaceContainer);
                linkSpaceContainer->stackBefore(branchesContainer);
            }
        }
    }
}

int BranchContainer::imageCount()
{
    if (!imagesContainer)
        return 0;
    else
        return imagesContainer->childItems().count();
}

void BranchContainer::createImagesContainer()
{
    imagesContainer = new Container ();
    imagesContainer->setLayoutType(Container::FloatingBounded);
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

LinkContainer* BranchContainer::getLinkContainer()
{
    return linkContainer;
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
        n = branchCount();
    } else if (c->type == Image) {
        radius = 100;
        n = imageCount();
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

    QRectF r;

    if (hasFloatingBranchesLayout()) {
        // Floating layout, position on circle around center of myself
        r = headingContainer->rect();
        qreal radius = Geometry::distance(r.center(), r.topLeft()) + 20;

        QPointF center = mapToScene(r.center());
        qreal a = getAngle(p_scene - center);
        hint = center + QPointF (radius * cos(a), - radius * sin(a));
    } else {
        // Regular layout
        if (branchesContainer)
            r = branchesContainer->rect();  // FIXME-1 check rotation: is rect still correct or better mapped bbox/rect?
        qreal y;
        if (d_pos == 0)
            y = r.bottom();
        else
            y = r.bottom() - d_pos * c->rect().height();

        switch (orientation) {
            case LeftOfParent:
                hint = QPointF(-linkWidth + r.left() - c->rect().width(), y);
                break;
            default:
                hint = QPointF( linkWidth + r.right(), y);
                break;
        }
        hint = headingContainer->mapToScene(hint);
    }

    return hint;
}

void BranchContainer::updateUpLink()
{
    if (branchItem->depth() == 0) return;

    if (temporaryLinked) {
    /* FIXME-0000 cont here to update link for tempLinked branches
        BranchItem *pbi = branchItem->parentBranch();
        pbi 
    */
    } else {
        // Get "real" parentBranchContainer, not tmpParentContainer (!)
        BranchContainer *pbc = branchItem->parentBranch()->getBranchContainer();

        QPointF upLinkParent_sp;
        QPointF upLinkSelf;
        QPointF downLink;
        switch (orientation) {
            case RightOfParent:
                upLinkParent_sp = pbc->ornamentsContainer->mapToScene(pbc->ornamentsContainer->rect().bottomRight());
                upLinkSelf = ornamentsContainer->rect().bottomLeft();
                downLink = ornamentsContainer->rect().bottomRight();
                break;
            case LeftOfParent:
                upLinkParent_sp = pbc->ornamentsContainer->mapToScene(pbc->ornamentsContainer->rect().bottomLeft());
                upLinkSelf = ornamentsContainer->rect().bottomRight();
                downLink = ornamentsContainer->rect().bottomLeft();
                break;
            default:
                upLinkParent_sp = pbc->ornamentsContainer->mapToScene(pbc->ornamentsContainer->rect().center());
                upLinkSelf = ornamentsContainer->rect().center();   // FIXME-2 check...
                downLink = ornamentsContainer->rect().center();
                break;
        }
        linkContainer->setUpLinkPosParent(linkContainer->sceneTransform().inverted().map(upLinkParent_sp));
        linkContainer->setUpLinkPosSelf(upLinkSelf);
        linkContainer->setDownLinkPos(downLink);
    }

    linkContainer->updateLinkGeometry();
}

void BranchContainer::setLayoutType(const LayoutType &ltype)
{
    if (type != Branch && type != TmpParent) 
        qWarning() << "BranchContainer::setLayoutType (...) called for non-branch: " << info();
    Container::setLayoutType(ltype);
}
    
void BranchContainer::setBranchesContainerLayoutType(const LayoutType &ltype)
{
    branchesContainerLayoutType = ltype;
    if (branchesContainer)
        branchesContainer->setLayoutType(branchesContainerLayoutType);
}
    
void BranchContainer::setBranchesContainerHorizontalAlignment(const HorizontalAlignment &valign)
{
    branchesContainerHorizontalAlignment = valign;
    if (branchesContainer)
        branchesContainer->setHorizontalAlignment(branchesContainerHorizontalAlignment);
}
    
QRectF BranchContainer::getHeadingRect()
{
    QPointF p = headingContainer->scenePos();
    return QRectF(p.x(), p.y(), headingContainer->rect().width(), headingContainer->rect().height());
}

void BranchContainer::setRotationHeading(const int &a)  // FIXME-0 crashing for mapcenter
{
    headingContainer->setTransformOriginPoint(headingContainer->rect().center());
    //headingContainer->setTransformOriginPoint(QPointF(0,0));
    headingContainer->setRotation( 1.0 * a);
}

int BranchContainer::getRotationHeading()
{
    return std::round(headingContainer->rotation());
}

void BranchContainer::setRotationInnerContent(const int &a)
{
    innerContainer->setTransformOriginPoint(innerContainer->rect().center());
    innerContainer->setRotation(a);
}

int BranchContainer::getRotationInnerContent()
{
    return std::round(innerContainer->rotation());
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
                    if (pbc->orientation == UndefinedOrientation) {
                        // Parent is tmpParentContainer or mapCenter
                        // use relative position to determine orientation
                        if (pos().x() > 0)
                            orientation = RightOfParent;
                        else
                            orientation = LeftOfParent;
                    } else {
                        // Set same orientation as parent
                        setOrientation(pbc->orientation);
                    }
                }
            }   // regular repositioning
        } // else:
        // The "else" here would be that I'm the tmpParentContainer, but
        // then my orientation is already set in MapEditor, so ignore here
    }

    setLayoutType(Horizontal);
    
    // FIXME-2 for testing draw blue rectangles
    /*
    */
    if (type != TmpParent) {
        ornamentsContainer->setPen(QPen(Qt::blue));
        setPen(QPen(Qt::green));
    }

    // Settings depending on depth
    if (depth == 0)
    {
        // MapCenter or TmpParent
        if (type != TmpParent) {
            setHorizontalDirection(LeftToRight);
            innerContainer->setHorizontalDirection(RightToLeft);
        }

        linkContainer->setLinkStyle(LinkContainer::NoLink);

        innerContainer->setMovableByFloats(false);
        setMovableByFloats(false);
        setBranchesContainerLayoutType(FloatingBounded);

    } else {
        // Branch or mainbranch
        linkContainer->setLinkStyle(LinkContainer::Line);
        innerContainer->setMovableByFloats(true);
        setBranchesContainerLayoutType(Vertical);

        switch (orientation) {
            case LeftOfParent:
                setHorizontalDirection(RightToLeft);
                innerContainer->setHorizontalDirection(RightToLeft);
                setBranchesContainerHorizontalAlignment(AlignedRight);
                break;
            case RightOfParent:
                setHorizontalDirection(LeftToRight);
                innerContainer->setHorizontalDirection(LeftToRight);
                setBranchesContainerHorizontalAlignment(AlignedLeft);
                break;
            default: 
                break;
        }

        if (branchItem && branchItem->getHeadingPlain().startsWith("float")) {  // FIXME-2 testing, needs dialog for setting
            // Special layout: FloatingBounded children 
            orientation = UndefinedOrientation;
            QColor col (Qt::red);
            col.setAlpha(150);
            if (branchesContainer) {    // FIXME-2 testing
                branchesContainer->setBrush(col);
            }
            setBranchesContainerLayoutType(FloatingBounded);
            innerContainer->setBrush(Qt::cyan);
        } else if (branchItem && branchItem->getHeadingPlain().startsWith("free")) {// FIXME-2 testing, needs dialog for setting
            // Special layout: FloatingBounded children 
            orientation = UndefinedOrientation;
            QColor col (Qt::red);
            col.setAlpha(120);
            if (branchesContainer) {    // FIXME-2 testing
                branchesContainer->setBrush(col);
            }
            setBranchesContainerLayoutType(FloatingFree);
            innerContainer->setBrush(Qt::gray);
        } 
        /* FIXME-2 Testing
        else {
            QColor col (Qt::blue);
            col.setAlpha(120);
            if (branchesContainer) branchesContainer->setBrush(col);
            //innerContainer->setBrush(Qt::cyan);
        }
        */
    }

    // Update branchesContainer and linkSpaceContainer,
    // this even might remove these containers
    updateBranchesContainer();

    Container::reposition();

    // Finally update links
    if (branchesContainer && branchCount() > 0) {
        foreach (QGraphicsItem *g_item, branchesContainer->childItems()) {
            BranchContainer *bc = (BranchContainer*) g_item;
            bc->updateUpLink();
        }
    }
}
