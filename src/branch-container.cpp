#include <QDebug>
#include <math.h>

#include "branch-container.h"

#include "branchitem.h"
#include "flagrow-container.h"
#include "geometry.h"
#include "heading-container.h"
#include "link-container.h"

extern FlagRowMaster *standardFlagsMaster;
extern FlagRowMaster *userFlagsMaster;
extern FlagRowMaster *systemFlagsMaster;

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
    ornamentsContainer->type = OrnamentsContainer;
    // FIXME-2 not available atm ornamentsContainer->setVerticalAligment(" centered ");

    linkContainer = new LinkContainer();

    innerContainer = new Container ();
    innerContainer->type = InnerContent;

    // Adding the containers will reparent them and thus set scene
    // FIXME-2 ornamentsContainer->addContainer(systemFlagsContainer);
    // FIXME-2 ornamentsContainer->addContainer(userFlagsContainer);
    ornamentsContainer->addContainer(linkContainer);
    ornamentsContainer->addContainer(headingContainer);

    standardFlagRowContainer = new FlagRowContainer;
    systemFlagRowContainer = new FlagRowContainer;

    ornamentsContainer->addContainer(standardFlagRowContainer);
    ornamentsContainer->addContainer(systemFlagRowContainer);
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

    temporaryLinked = false;
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

void BranchContainer::setRealScenePos(const QPointF &sp)    // FIXME-1 still needed?
{
    // Move my self in a way, that finally center of ornamentsContainer
    // will be at scenePos sp
    QPointF t_oc = mapFromItem(ornamentsContainer, ornamentsContainer->rect().center());
    QPointF p_new = sp - t_oc;

    if (branchItem->depth() == 0)
        setPos(p_new);
    else {
        //QPointF p_new = sceneTransform().inverted().map(scenePos() + t_oc);
        //setPos(scenePos() + t_oc);
        QPointF q = ornamentsContainer->mapToScene(ornamentsContainer->rect().center());
        QPointF r = sp - q;
        qDebug() << "BC::sRSP " << info() <<
            " sp=" << sp <<
            " o_cntr=" << q <<
            " t_oc=" << t_oc <<
            " r=" << r <<
            "o=" << orientation;
        setPos(pos() + r);
    }
}

QPointF BranchContainer::getRealScenePos()  // FIXME-1 still needed?
{
    QPointF r = ornamentsContainer->mapToScene(ornamentsContainer->rect().center());
    qDebug() << "BC::gRSP " << info() << " o_cntr=" << r << "o=" << orientation ;
    return r;
}

void BranchContainer::moveToRefPos(const QPointF &rp) // FIXME-000 cont here
{
    // Move my self in a way, that finally the refrence position
    // (center of ornamentsContainer) will be at rp
    QPointF t_oc = mapFromItem(ornamentsContainer, ornamentsContainer->rect().center());
    QPointF p_new = rp - t_oc;

    if (branchItem->depth() == 0)
        setPos(p_new);
    else {
        //QPointF p_new = sceneTransform().inverted().map(scenePos() + t_oc);
        //setPos(scenePos() + t_oc);
        QPointF q = ornamentsContainer->mapToScene(ornamentsContainer->rect().center());
        QPointF r = rp - q;
        qDebug() << "BC::sRP " << info() <<
            " rp=" << rp <<
            " o_cntr=" << q <<
            " t_oc=" << t_oc <<
            " r=" << r <<
            "o=" << orientation;
        setPos(pos() + r);
    }
}

QPointF BranchContainer::getRefPos() // FIXME-000 cont here
{
    QPointF rp = ornamentsContainer->mapToScene(ornamentsContainer->rect().center());
    qDebug() << "BC::gRP " << info() << " rp=" << rp << "o=";
    return rp;
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
    if (!outerContainer) {
        outerContainer = new Container (this);
        outerContainer->setLayout(BoundingFloats);
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
    qDebug() << info();
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

void BranchContainer::createImagesContainer()
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

    c->setParentItem(imagesContainer);

    QPointF sp = c->scenePos();
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

void BranchContainer::switchLayout(const Layout &l) // FIXME-0 testing, will go to above setLayout later Also needs to be renamed to switchBranchesContainerLayout
{
    if (l == layout) return;

    if (branchCount() == 0 || (l != FloatingFree && l != FloatingBounded )) {
        Container::setLayout(l);
        return;
    }

    // If we have children, we want to preserve positions
    // before changing layout to floating
    qDebug() << "BC::switchLayout, preserving positions";
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

    if (branchesContainer) { // FIXME-0 only use this if switching to floating*
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

void BranchContainer::updateVisuals() // FIXME-2 missing: standardFlags, systemFlags
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

    setLayout(Horizontal);

    // Settings depending on depth
    if (depth == 0)
    {
        // MapCenter or TmpParent
        if (type != TmpParent) {
            setHorizontalDirection(LeftToRight);
            innerContainer->setHorizontalDirection(RightToLeft);
        }

        linkContainer->setLinkStyle(LinkContainer::NoLink);

        innerContainer->setLayout(BoundingFloats);
        setBranchesContainerLayout(FloatingBounded);    // FIXME-2 think, if autolayout should be used for mapcenter
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
                setBranchesContainerHorizontalAlignment(AlignedLeft);
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
        ornamentsContainer->setPen(QPen(Qt::blue));

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
    }

}
