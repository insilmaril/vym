#include <QDebug>
#include <math.h>

#include "branch-container.h"

#include "branchitem.h"
#include "flagrow-container.h"
#include "geometry.h"
#include "heading-container.h"
#include "link-container.h"
#include "misc.h"

extern FlagRowMaster *standardFlagsMaster;
extern FlagRowMaster *userFlagsMaster;
extern FlagRowMaster *systemFlagsMaster;

qreal BranchContainer::linkWidth = 20;  // FIXME-2 testing

#define qdbg() qDebug().nospace().noquote()

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
    ornamentsContainer->type = OrnamentsContainer;
    // FIXME-2 not available atm ornamentsContainer->setVerticalAligment(" centered ");

    linkContainer = new LinkContainer();

    innerContainer = new Container ();
    innerContainer->type = InnerContent;

    ornamentsContainer->addContainer(linkContainer);
    ornamentsContainer->addContainer(headingContainer);

    standardFlagRowContainer = new FlagRowContainer;    // FIXME-1 Only create FRCs on demand
    systemFlagRowContainer = new FlagRowContainer;

    // Adding the containers will reparent them and thus set scene
    ornamentsContainer->addContainer(standardFlagRowContainer);
    ornamentsContainer->addContainer(systemFlagRowContainer);
    ornamentsContainer->setCentralContainer(headingContainer);
    innerContainer->addContainer(ornamentsContainer);

    branchesContainer = nullptr;
    linkSpaceContainer = nullptr;

    outerContainer = nullptr;

    addContainer(innerContainer);

    setLayout(Container::Horizontal);
    setHorizontalDirection(Container::LeftToRight);

    // Use layout defaults
    imagesContainerAutoLayout = true;
    branchesContainerAutoLayout = true;
    imagesContainerLayout = getDefaultImagesContainerLayout();
    branchesContainerLayout = Vertical;

    temporaryLinked = false;

    refPos = QPointF();

    scrollOpacity = 1;
}

BranchContainer* BranchContainer::parentBranchContainer()
{
    Container *p = parentContainer();
    if (!p) return nullptr;;

    // Some checks before the real parent can be returned
    if (p->type != BranchesContainer) return nullptr;

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

#include <QTransform>
void BranchContainer::setScrollOpacity(qreal o)   // FIXME-2 testing for potential later animation
{
    scrollOpacity = o;
    //headingContainer->setScrollOpacity(a);
    headingContainer->setOpacity(o);
    if (standardFlagRowContainer)
        standardFlagRowContainer->setOpacity(o);
    QTransform t;
    t.scale (1, o);
    setTransform(t);

    qDebug() << "BC::sSO  o=" << o << " flags=" << flags();
    setOpacity(o);
}

qreal BranchContainer::getScrollOpacity()
{
    return scrollOpacity;
}

void BranchContainer::setRefPos(const QPointF &p)
{
    qDebug() << "BC setRefPos p=" << qpointFToString(p, 0);
    refPos = p;
}

QPointF BranchContainer::getRefPos()
{
    // Return the reference position.
    //
    // The refpos is the center of the heading container relative to my parents
    // center of heading container (or origin in case of mapcenter). 
    // The coordinate system used here is the one of the parent branch container (!)

    BranchContainer *pbc = parentBranchContainer();

    if (!pbc)
        // I am the mapcenter, return scene position
        return headingContainer->mapToScene(headingContainer->rect().center());

    QPointF p_own = headingContainer->mapToItem(pbc, headingContainer->rect().center());

    HeadingContainer *phc = pbc->getHeadingContainer();
    QPointF p_parent = phc->mapToItem(pbc, phc->rect().center());

    QPointF r = p_own - p_parent;
    return r;
}

void BranchContainer::moveToRefPos() // FIXME-1 used at all?
{
    qDebug() << "BC::moveToRefPos " << info() ;
    // Move my self in a way, that finally the refrence position
    // (center of headingContainer) will be at p - relative to parents center of hC

    BranchContainer *pbc = parentBranchContainer();

    QPointF p_own;
    QPointF p_new;

    if (!pbc) {
        // I am the mapcenter, move in a way, that center of hC is at scene position p
        p_own = headingContainer->mapToScene(headingContainer->rect().center());
        p_new = refPos - getRefPos() + pos();

        qDebug() << "BC::mtRP refPos="
            << qpointFToString(refPos, 0)
            << "getRefPos=" << qpointFToString(getRefPos(), 0)
            << "p_new=" << qpointFToString(p_new,0)
            << "pos=" << qpointFToString(pos(),0);

    } else {
        p_own = headingContainer->mapToItem(this, headingContainer->rect().center());
        HeadingContainer *phc = pbc->getHeadingContainer();
        QPointF p_parent = phc->mapToItem(pbc, phc->rect().center());

        p_new = refPos - getRefPos() + pos();

        qDebug() << "BC::mtRP refPos="
            << qpointFToString(refPos, 0)
            << "getRefPos=" << qpointFToString(getRefPos(), 0)
            << " p_own=" << qpointFToString(p_own,0)
            << " p_new=" << qpointFToString(p_new,0)
            << " p_parent=" << qpointFToString(p_parent,0)
            << " pos=" << qpointFToString(pos(),0);
    }
    qDebug() << "     new refPos="
        << qpointFToString(p_new, 0);
    setPos(p_new);
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

    if (branchesContainerLayout == FloatingBounded || branchesContainerLayout == FloatingFree)
        return true;
    else
        return false;
}

void BranchContainer::createBranchesContainer()
{
    branchesContainer = new Container ();
    if (branchesContainerAutoLayout)
        branchesContainer->setLayout(getDefaultBranchesContainerLayout());
    else
        branchesContainer->setLayout(branchesContainerLayout);
    branchesContainer->setHorizontalAlignment(branchesContainerHorizontalAlignment);
    branchesContainer->type = Container::BranchesContainer;

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

        // Space for links depends on layout and scrolled state:
        if (linkSpaceContainer) {
            if (hasFloatingBranchesLayout() || branchItem->isScrolled()) {
                delete linkSpaceContainer;
                linkSpaceContainer = nullptr;
            }
        } else {
            if (!hasFloatingBranchesLayout() && !branchItem->isScrolled()) {
                linkSpaceContainer = new HeadingContainer ();
                linkSpaceContainer->setHeading(" - ");  // FIXME-2 introduce minWidth later in Container instead of a pseudo heading here

                innerContainer->addContainer(linkSpaceContainer);
                linkSpaceContainer->stackBefore(branchesContainer);
            }
        }
    }
}

void BranchContainer::createOuterContainer()
{
    qDebug() << "BC::createOuterContainer in " << info();
    if (!outerContainer) {
        outerContainer = new Container (this);
        //outerContainer->setLayout(Horizontal);
        outerContainer->setLayout(BoundingFloats);  // FIXME-000 disabled temporarily?
        outerContainer->type = InnerContent;
        outerContainer->addContainer(innerContainer);
        if (imagesContainer)
            outerContainer->addContainer(imagesContainer);
        innerContainer->setLayout(Horizontal);
        addContainer(outerContainer);
    }
}

void BranchContainer::deleteOuterContainer()
{
    if (outerContainer) {
        addContainer(innerContainer);
        if (imagesContainer) {
            innerContainer->addContainer(imagesContainer);
        }
        delete outerContainer;
        outerContainer = nullptr;
    }
}

void BranchContainer::updateChildrenStructure()
{
    // The structure of subcontainers within a BranchContainer
    // depends on layouts of imagesContainer and branchesContainer:
    //
    // Usually both iC and bC are children of innerContainer.
    // The layout of innerContainer is either Horizontal or BoundingFloats
    // outerContainer is only needed in corner case d)
    //
    // a) No FloatingBounded children
    //    - No outerContainer
    //    - innerContainer is Horizontal
    //    - branchesContainer is not FloatingBounded
    //    - imagesContainer is FloatingFree
    //
    // b) Only branches are FloatingBounded
    //    - No outerContainer
    //    - innerContainer BoundingFloats
    //    - branchesContainer is FloatingBounded
    //    - imagesContainer is FloatingFree
    //
    // c) images and branches are FloatingBounded
    //    - No outerContainer
    //    - innerContainer BoundingFloats
    //    - branchesContainer is FloatingBounded
    //    - imagesContainer is FloatingBounded
    //
    // d) Only images are FloatingBounded
    //    - outerContainer contains
    //      - innerContainer
    //      - imagesContainer
    //    - innerContainer is Horizontal
    //    - branchesContainer is Vertical
    //    - imagesContainer is FloatingBounded

    if (branchesContainerLayout != FloatingBounded && imagesContainerLayout != FloatingBounded) {
        // a) No FloatingBounded children
        deleteOuterContainer();
        innerContainer->setLayout(Horizontal);
    } else if (branchesContainerLayout == FloatingBounded && imagesContainerLayout != FloatingBounded) {
        // b) Only branches are FloatingBounded
        deleteOuterContainer();
        innerContainer->setLayout(BoundingFloats);
    } else if (branchesContainerLayout == FloatingBounded && imagesContainerLayout == FloatingBounded) {
        // c) images and branches are FloatingBounded
        deleteOuterContainer();
        innerContainer->setLayout(BoundingFloats);
    } else if (branchesContainerLayout != FloatingBounded && imagesContainerLayout == FloatingBounded) {
        // d) Only images are FloatingBounded
        createOuterContainer();
    } else {
        // e) remaining cases
        deleteOuterContainer();
        innerContainer->setLayout(FloatingBounded);
    }
}

void BranchContainer::showStructure()
{
    qDebug() << info() << " refPos=" << qpointFToString(getRefPos(), 0);
    Container *c = getBranchesContainer();
    if (outerContainer)
        qDebug() << "outerContainer:" << outerContainer;
    else
        qDebug() << "No outerContainer.";

    qDebug() << "InnerContainer: " << innerContainer->info();

    if (c) {
        qDebug() << "branchesContainer: " << c->info();
        for (int i=0; i < c->childItems().count(); i++)
            qDebug() << "  i=" << i << ((Container*)c->childItems().at(i))->info();
    }
    c = getImagesContainer();
    if (c) {
        qDebug() << "imagesContainer: " << c->info();
        for (int i=0; i < c->childItems().count(); i++)
            qDebug() << "  i=" << i << ((Container*)c->childItems().at(i))->info();
    }


    c = standardFlagRowContainer;
    if (c) {
        qDebug() << "Standard flags: " << c;
        for (int i=0; i < c->childItems().count(); i++)
            qDebug() << "  i=" << i << ((Container*)c->childItems().at(i))->info();
    }
    c = systemFlagRowContainer;
    if (c) {
        qDebug() << "System flags: " << c;
        for (int i=0; i < c->childItems().count(); i++)
            qDebug() << "  i=" << i << ((Container*)c->childItems().at(i))->info();
    }
}

int BranchContainer::imageCount()
{
    if (!imagesContainer)
        return 0;
    else
        return imagesContainer->childItems().count();
}

void BranchContainer::createImagesContainer() // FIXME-2 imagesContainer not deleted, when no longer used
{
    imagesContainer = new Container ();
    if (imagesContainerAutoLayout)
        imagesContainer->setLayout(getDefaultImagesContainerLayout());
    else
        imagesContainer->setLayout(imagesContainerLayout);
    imagesContainer->type = Container::ImagesContainer;
    if (outerContainer)
        outerContainer->addContainer(imagesContainer);
    else
        innerContainer->addContainer(imagesContainer);
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
            r = branchesContainer->rect();  // FIXME-2 check rotation: is rect still correct or better mapped bbox/rect?
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
    /* FIXME-1 cont here to update link for tempLinked branches
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

void BranchContainer::setLayout(const Layout &l)
{
    if (type != Branch && type != TmpParent)
        qWarning() << "BranchContainer::setLayout (...) called for non-branch: " << info();
    Container::setLayout(l);
}

void BranchContainer::setImagesContainerLayout(const Layout &ltype)
{
    if (imagesContainerLayout == ltype)
        return;

    imagesContainerLayout = ltype;

    if (imagesContainer)
        imagesContainer->setLayout(imagesContainerLayout);
}

Container::Layout BranchContainer::getImagesContainerLayout()
{
    return imagesContainerLayout;
}

void BranchContainer::setBranchesContainerLayout(const Layout &ltype)
{
    if (branchesContainerLayout == ltype)
        return;

    branchesContainerLayout = ltype;

    if (branchesContainer) { // FIXME-1 only use this if switching to floating*
        // Keep current positions
        QPointF oc_pos = ornamentsContainer->pos();
        QPointF bcc_pos = branchesContainer->pos() - oc_pos;
        //qDebug() << "BC::setBCLayout " << getName() << " oc_pos=" << oc_pos << "bcc_pos=" << bcc_pos;

        foreach (QGraphicsItem *child, branchesContainer->childItems()) {
            BranchContainer *bc = (BranchContainer*)child;
            bc->setPos( bc->pos() + bcc_pos);
        }

        if (ltype == FloatingFree)
            setPos (pos() +  oc_pos);
    }

    if (branchesContainer)
        branchesContainer->setLayout(branchesContainerLayout);
}

Container::Layout BranchContainer::getBranchesContainerLayout()
{
    return branchesContainerLayout;
}

void BranchContainer::setBranchesContainerHorizontalAlignment(const HorizontalAlignment &valign)
{
    branchesContainerHorizontalAlignment = valign;
    if (branchesContainer)
        branchesContainer->setHorizontalAlignment(branchesContainerHorizontalAlignment);
}

void BranchContainer::setBranchesContainerBrush(const QBrush &b)
{
    branchesContainerBrush = b;
    if (branchesContainer)
        branchesContainer->setBrush(branchesContainerBrush);
}

QRectF BranchContainer::getHeadingRect()
{
    QPointF p = headingContainer->scenePos();
    return QRectF(p.x(), p.y(), headingContainer->rect().width(), headingContainer->rect().height());
}

void BranchContainer::setRotationHeading(const int &a)
{
    headingContainer->setRotation( 1.0 * a);
    //headingContainer->setScale(f + a * 1.1);      // FIXME-2 what about scaling?? Which transformCenter?
}

int BranchContainer::getRotationHeading()
{
    return std::round(headingContainer->rotation());
}

void BranchContainer::setRotationInnerContent(const int &a)
{
    innerContainer->setRotation(a);    // FIXME-1 outer container is not rotated (only images BoundingFloats)
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

    // Update standard flags active in TreeItem

    QList<QUuid> TIactiveFlagUids = branchItem->activeFlagUids();
    standardFlagRowContainer->updateActiveFlagContainers(
        TIactiveFlagUids, standardFlagsMaster, userFlagsMaster);

    // Add missing system flags active in TreeItem
    TIactiveFlagUids = branchItem->activeSystemFlagUids();
    systemFlagRowContainer->updateActiveFlagContainers(TIactiveFlagUids, systemFlagsMaster);

}

Container::Layout BranchContainer::getDefaultBranchesContainerLayout()
{
    if (type == TmpParent)
        return FloatingFree;

    if (branchItem->depth() == 0)
        return FloatingBounded;
    else
        return Vertical;
}

Container::Layout BranchContainer::getDefaultImagesContainerLayout()
{
    return FloatingBounded;
}

void BranchContainer::reposition()
{
    // qDebug() << "BC::reposition " << info();

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

                        if (!parentContainer()->hasFloatingLayout()) {
                            // Special case: Horizontal or vertical layout, but child of MC
                            // Should only occur in testing
                            qdbg() << "Setting hardcoded RoP for " << info();
                            qdbg() << "                       pc=" << parentContainer()->info();
                            orientation = RightOfParent;
                        } else {
                            qdbg() << "Setting neworient for " << info();
                            qdbg() << "                       pc=" << parentContainer()->info();
                            if (pos().x() > 0)
                                orientation = RightOfParent;
                            else
                                orientation = LeftOfParent;
                        }
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

    setLayout(Horizontal);  // FIXME-2 always needed here?

    linkContainer->setLinkStyle(LinkContainer::NoLink);

    // Settings depending on depth
    if (depth == 0)
    {
        // MapCenter or TmpParent?
        if (type != TmpParent) {
            setHorizontalDirection(LeftToRight);
            innerContainer->setHorizontalDirection(LeftToRight);
        }

        linkContainer->setLinkStyle(LinkContainer::NoLink);

        innerContainer->setLayout(BoundingFloats);
        setBranchesContainerLayout(FloatingBounded);    // FIXME-2 think, if autolayout should be used for mapcenter

        // FIXME-0 testing only layouts
        //innerContainer->setLayout(Horizontal);
        //setBranchesContainerLayout(Horizontal);
    } else {
        // Branch or mainbranch
        linkContainer->setLinkStyle(LinkContainer::Line);

        if (branchesContainerAutoLayout)
            setBranchesContainerLayout(Vertical);

        switch (orientation) {
            case LeftOfParent:
                setHorizontalDirection(RightToLeft);
                innerContainer->setHorizontalDirection(RightToLeft);
                setBranchesContainerHorizontalAlignment(AlignedRight);
                break;
            case RightOfParent:
                setHorizontalDirection(LeftToRight);
                innerContainer->setHorizontalDirection(LeftToRight);
                setBranchesContainerHorizontalAlignment(AlignedCentered);
                break;
            default:
                break;
        }
    }

    // Depending on layouts, we might need to insert outerContainer and relink children
    updateChildrenStructure();

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

    // FIXME-3 for testing we do some coloring and additional drawing
    /*
    */
    if (type != TmpParent) {
        // BranchContainer
        setPen(QPen(Qt::green));

        // OrnamentsContainer
        //ornamentsContainer->setPen(QPen(Qt::blue));
        ornamentsContainer->setPen(Qt::NoPen);

        // InnerContainer
        innerContainer->setPen(QPen(Qt::cyan));

        QColor col;
        if (branchesContainerLayout == FloatingBounded && depth > 0) {
            // Special layout: FloatingBounded
            col = QColor(Qt::gray);
            col.setAlpha(150);
            setBranchesContainerBrush(col);
        } else if (branchesContainerLayout == FloatingFree) {
            // Special layout: FloatingFree
            col = QColor(Qt::blue);
            col.setAlpha(120);
            setBrush(col);
        } else {
            // Don't paint other containers
            setBranchesContainerBrush(Qt::NoBrush);
            setBrush(Qt::NoBrush);
            if (branchesContainer) branchesContainer->setPen(QColor(Qt::gray));
        }
    }   // Debug visualizations
}
