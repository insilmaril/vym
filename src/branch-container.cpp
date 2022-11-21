#include <QDebug>
#include <math.h>

#include "branch-container.h"

#include "branchitem.h"
#include "flag-container.h"
#include "flagrow-container.h"
#include "frame-container.h"
#include "geometry.h"
#include "heading-container.h"
#include "link-container.h"
#include "mapdesign.h"
#include "misc.h"
#include "xlinkobj.h"

#define qdbg() qDebug().nospace().noquote()

extern FlagRowMaster *standardFlagsMaster;
extern FlagRowMaster *userFlagsMaster;
extern FlagRowMaster *systemFlagsMaster;

qreal BranchContainer::linkWidth = 20;  // FIXME-3 testing

BranchContainer::BranchContainer(QGraphicsScene *scene, BranchItem *bi)  // FIXME-2 scene and addItem should not be required, only for mapCenters without parent:  setParentItem automatically sets scene!
{
    // qDebug() << "* Const BranchContainer begin this = " << this << "  branchitem = " << bi;
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
    containerType = Container::Branch;

    // BranchContainers inherit FrameContainer, reset overlay
    overlay = false;

    orientation = UndefinedOrientation;

    imagesContainer = nullptr;
    selectionContainer = nullptr;

    headingContainer = new HeadingContainer ();

    ornamentsContainer = new Container ();
    ornamentsContainer->containerType = OrnamentsContainer;

    linkContainer = new LinkContainer();

    innerContainer = new Container ();
    innerContainer->containerType = InnerContent;

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

    // Center of whole mainBranches should be the heading
    setCentralContainer(headingContainer);

    // Set some defaults, should be overridden from MapDesign later
    imagesContainerAutoLayout = true;
    branchesContainerAutoLayout = true;
    imagesContainerLayout = FloatingFree;
    branchesContainerLayout = Vertical;

    tmpParentContainer = nullptr;

    scrollOpacity = 1;
}

BranchContainer* BranchContainer::parentBranchContainer()
{
    Container *p = parentContainer();
    if (!p) return nullptr;;

    // Some checks before the real parent can be returned
    if (p->containerType != BranchesContainer) return nullptr;

    p = p->parentContainer();
    if (!p) return nullptr;;

    if (p->containerType != InnerContent) return nullptr;

    p = p->parentContainer();
    if (!p) return nullptr;;

    if (p->containerType == OuterContainer) {
        p = p->parentContainer();
        if (!p) return nullptr;;
    }

    if (! (p->containerType == Branch || p->containerType == TmpParent)) return nullptr;

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

void BranchContainer::setOrientation(const Orientation &o)
{
    orientation = o;
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

bool BranchContainer::isOriginalFloating()
{
    return originalFloating;
}

void BranchContainer::setTemporaryLinked(BranchContainer *tpc)
{
    tmpParentContainer = tpc;
}

void BranchContainer::unsetTemporaryLinked()
{
    tmpParentContainer = nullptr;
}

bool BranchContainer::isTemporaryLinked()
{
    if (tmpParentContainer)
        return true;
    else
        return false;
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

void BranchContainer::createBranchesContainer() // FIXME-0 move settings to updateStyles, also createImagesContainer
{
    branchesContainer = new Container ();
    branchesContainer->containerType = Container::BranchesContainer;

    if (branchItem) {
        // tmpParentContainer has no associated branchItem!
        MapDesign *md = branchItem->getMapDesign();

        if (branchesContainerAutoLayout)
            branchesContainer->setLayout(
                    md->branchesContainerLayout(NewBranch, branchItem->depth()));
        else
            branchesContainer->setLayout(branchesContainerLayout);

        // Will be changed later, depends on orientation
        branchesContainer->setVerticalAlignment(branchesContainerVerticalAlignment);
    }

    innerContainer->addContainer(branchesContainer);
}

void BranchContainer::addToBranchesContainer(Container *c, bool keepScenePos)
{
    if (!branchesContainer) {
        createBranchesContainer();
//        updateStyles(NewBranch);  // FIXME-2 probably not needed
    }

    QPointF sp = c->scenePos();
    branchesContainer->addContainer(c);
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
        if (!branchesContainer) {
            // should never happen
            qDebug() << "BC::updateBranchesContainer no branchesCOntainer available!";
            return;
        }

        // Space for links depends on layout and scrolled state:
        if (linkSpaceContainer) {
            if (hasFloatingBranchesLayout() || branchItem->isScrolled()) {
                delete linkSpaceContainer;
                linkSpaceContainer = nullptr;
            }
        } else {
            if (!hasFloatingBranchesLayout() && !branchItem->isScrolled()) {
                linkSpaceContainer = new HeadingContainer ();
                //linkSpaceContainer->setContainerType(); // FIXME-2
                linkSpaceContainer->setHeading("   ");  // FIXME-2 introduce minWidth later in Container instead of a pseudo heading here  see oc.pos

                innerContainer->addContainer(linkSpaceContainer);
                linkSpaceContainer->stackBefore(branchesContainer);
            }
        }
    }
}

void BranchContainer::createOuterContainer()
{
    if (!outerContainer) {
        outerContainer = new Container;
        outerContainer->setParentItem(this);
        outerContainer->containerType = OuterContainer;
        outerContainer->setLayout(BoundingFloats);
        outerContainer->addContainer(innerContainer);
        if (imagesContainer)
            outerContainer->addContainer(imagesContainer);
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
    // Usually both inagesContainer and branchesContainer are children of
    // innerContainer.  The layout of innerContainer is either Horizontal or
    // BoundingFloats outerContainer is only needed in corner case d)
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
        innerContainer->setLayout(Horizontal);
    } else {
        // e) remaining cases
        deleteOuterContainer();
        innerContainer->setLayout(FloatingBounded);
    }
}

void BranchContainer::showStructure()
{
    Container *c = getBranchesContainer();
    if (outerContainer)
        qDebug() << "outerContainer:" << outerContainer->info();
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
    imagesContainer->containerType = ImagesContainer;
    if (branchItem) {
        // tmpParentContainer has no associated branchItem!
        MapDesign *md = branchItem->getMapDesign();
        if (imagesContainerAutoLayout)
            imagesContainer->setLayout(
                    md->imagesContainerLayout(NewBranch, branchItem->depth()));
        else
            imagesContainer->setLayout(imagesContainerLayout);
    }
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
    imagesContainer->addContainer(c);

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
    if (c->containerType == Branch) {
        radius = 190;
        n = branchCount();
    } else if (c->containerType == Image) {
        radius = 100;
        n = imageCount();
    }

    if (!parentItem() || c->containerType == Image) {
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

    BranchContainer *pbc;
    if (tmpParentContainer)
        pbc = tmpParentContainer;
    else
        // Get "real" parentBranchContainer, not tmpParentContainer (!)
        pbc = branchItem->parentBranch()->getBranchContainer();

    QPointF upLinkParent_sp;
    QPointF upLinkSelf;
    QPointF downLink;
    switch (orientation) {
        case RightOfParent:
            upLinkParent_sp = pbc->ornamentsContainer->mapToScene(
                    pbc->ornamentsContainer->rect().bottomRight());
            upLinkSelf = ornamentsContainer->rect().bottomLeft();
            downLink = ornamentsContainer->rect().bottomRight();
            break;
        case LeftOfParent:
            upLinkParent_sp = pbc->ornamentsContainer->mapToScene(
                    pbc->ornamentsContainer->rect().bottomLeft());
            upLinkSelf = ornamentsContainer->rect().bottomRight();
            downLink = ornamentsContainer->rect().bottomLeft();
            break;
        default:
            upLinkParent_sp = pbc->ornamentsContainer->mapToScene(
                    pbc->ornamentsContainer->rect().center());
            upLinkSelf = ornamentsContainer->rect().center();   // FIXME-2 check...
            downLink = ornamentsContainer->rect().center();
            break;
    }
    linkContainer->setUpLinkPosParent(ornamentsContainer->sceneTransform().inverted().map(upLinkParent_sp));
    linkContainer->setUpLinkPosSelf(upLinkSelf);
    linkContainer->setDownLinkPos(downLink);

    // Color of links
    if (linkContainer->getLinkColorHint() == LinkContainer::HeadingColor)
        linkContainer->setLinkColor(headingContainer->getColor());
    else
        linkContainer->setLinkColor(branchItem->getMapDesign()->defaultLinkColor());

    linkContainer->updateLinkGeometry();
}

void BranchContainer::setLayout(const Layout &l)
{
    if (containerType != Branch && containerType != TmpParent)
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
    if (branchesContainerLayout == ltype || ltype == UndefinedLayout)
        return;

    branchesContainerLayout = ltype;

    if (branchesContainer) { // FIXME-2 only use this if switching to floating*
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

void BranchContainer::setBranchesContainerVerticalAlignment(const VerticalAlignment &a)
{
    branchesContainerVerticalAlignment = a;
    if (branchesContainer)
        branchesContainer->setVerticalAlignment(branchesContainerVerticalAlignment);
}

void BranchContainer::setBranchesContainerBrush(const QBrush &b)
{
    branchesContainerBrush = b;
    if (branchesContainer)
        branchesContainer->setBrush(branchesContainerBrush);
}

QRectF BranchContainer::getHeadingRect()
{
    QPointF p = headingContainer->mapToScene(headingContainer->rect().topLeft());
    return QRectF(p.x(), p.y(), headingContainer->rect().width(), headingContainer->rect().height());
}

qreal BranchContainer::getHeadingRotation()
{
    QPointF v =
          headingContainer->mapToScene(headingContainer->rect().topRight())
        - headingContainer->mapToScene(headingContainer->rect().topLeft());
    return getAngle(v);
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

void BranchContainer::setRotationContent(const int &a)
{
    if (outerContainer) {
        outerContainer->setTransformOriginPoint(0, 0);  // FIXME-2 originpoint needed?
        outerContainer->setRotation(a);
        innerContainer->setRotation(0);
    } else {
        innerContainer->setTransformOriginPoint(0, 0);  // FIXME-X originpoint needed?
        innerContainer->setRotation(a);    // FIXME-2 If BC is FloatingBounded, and bc has images, frame does not include images
    }
    setFrameRotation(a);
}

int BranchContainer::getRotationContent()
{
    return std::round(innerContainer->rotation());
}

QUuid BranchContainer::findFlagByPos(const QPointF &p)
{
    QUuid uid = systemFlagRowContainer->findFlagByPos(p);
    if (!uid.isNull())
        return uid;

    uid = standardFlagRowContainer->findFlagByPos(p);
    return uid;
}

bool BranchContainer::isInClickBox(const QPointF &p)
{
    return ornamentsContainer->rect().contains(ornamentsContainer->mapFromScene(p));
}

QRectF BranchContainer::getBBoxURLFlag()
{
    Flag *f = systemFlagsMaster->findFlagByName("system-url");
    if (f) {
        QUuid u = f->getUuid();
        FlagContainer *fc = systemFlagRowContainer->findFlagContainerByUid(u);
        if (fc)
            return fc->mapRectToScene(fc->rect());
    }
    return QRectF();
}

void BranchContainer::select()
{
    SelectableContainer::select(ornamentsContainer);
}

void BranchContainer::updateStyles(StyleUpdateMode styleUpdateMode)
{
    // updateStyles() is never called for TmpParent!

    //qDebug() << "BC::updateStyles of " << info();

    uint depth = branchItem->depth();
    MapDesign *md = branchItem->getMapDesign();

    // Set container layouts
    if (branchesContainerAutoLayout)
        setBranchesContainerLayout(
                md->branchesContainerLayout(styleUpdateMode, depth));

    if (imagesContainerAutoLayout)
        setImagesContainerLayout(
                md->imagesContainerLayout(styleUpdateMode, depth));

    // Links
    if (linkContainer->getLinkColorHint() == LinkContainer::HeadingColor)
        linkContainer->setLinkColor(headingContainer->getColor());
    else
        linkContainer->setLinkColor(md->defaultLinkColor());

    // Create/delete bottomline
    if (FrameContainer::frameType != FrameContainer::NoFrame &&
            linkContainer->hasBottomLine())
            linkContainer->deleteBottomLine();
    else {
        if (!linkContainer->hasBottomLine())
            linkContainer->createBottomLine();
    }

    // Example rules for testing
    /*
    if (depth == 0) {
        qDebug() << " d=0";
        headingContainer->setHeadingColor(Qt::blue);
    } else if (depth == 1) {
        headingContainer->setHeadingColor(Qt::green);
        qDebug() << " d=1";
    } else {
        qDebug() << " d>1";
        headingContainer->setHeadingColor(Qt::black);
    }
    */

    /* Conditions
     - styleUpdateMode: Relink or new branch?
     - depth "equal" or "greater than"
     - frame set? (different link points then)
     - map design
    */

    // FIXME-3 for testing we do some coloring and additional drawing
    /*
    if (containerType != TmpParent) {
        // BranchContainer
        setPen(QPen(Qt::green));

        // OrnamentsContainer
        ornamentsContainer->setPen(QPen(Qt::blue));
        //ornamentsContainer->setPen(Qt::NoPen);

        // InnerContainer
        innerContainer->setPen(QPen(Qt::cyan));

        if (branchesContainer) branchesContainer->setPen(QColor(Qt::gray));

        // Background colors for floating content
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
        }
    }   // Visualizations for testing
    */
}

void BranchContainer::updateVisuals()
{
    // Update heading
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

    if (!pbc && containerType != TmpParent) {
        // I am a (not moving) mapCenter
        orientation = UndefinedOrientation;
    } else {
        if (containerType != TmpParent) {
            // "regular repositioning", not currently moved in MapEditor
            if (!pbc)
                orientation = UndefinedOrientation;
            else {
                /*
                qdbg() << ind() << "BC::repos pbc=" << pbc->info();
                qdbg() << ind() << "BC::repos pbc->orientation=" << pbc->orientation;
                */

                if (pbc->orientation == UndefinedOrientation) {
                    // Parent is tmpParentContainer or mapCenter
                    // use relative position to determine orientation

                    if (parentContainer()->hasFloatingLayout()) {
                        if (pos().x() >= 0)
                            orientation = RightOfParent;
                        else
                            orientation = LeftOfParent;
                        /*
                        qdbg() << ind() << "BC: Setting neworient " << orientation << " in: " << info();
                        qdbg() << ind() << "    pc: " << parentContainer()->info();
                        */
                    } else {
                        // Special case: Mainbranch in horizontal or vertical layout
                        orientation = RightOfParent;
                    }
                } else {
                    // Set same orientation as parent
                    setOrientation(pbc->orientation);
                    //qdbg() << ind() << "BC: Setting parentorient " << orientation << " in: " << info();
                }
            }
        } // else:
        // The "else" here would be that I'm the tmpParentContainer, but
        // then my orientation is already set in MapEditor, so ignore here
    }

    // Settings depending on depth  // FIXME-0 most of this should go to BC::updateStyles
    if (depth == 0)
    {
        // MapCenter or TmpParent?
        if (containerType != TmpParent) {
            setHorizontalDirection(LeftToRight);
            innerContainer->setHorizontalDirection(LeftToRight);
            setLayout(FloatingBounded);
        } else {
            // TmpParent
            setLayout(Horizontal);  // FIXME-2 needed?
        }

        linkContainer->setLinkStyle(LinkContainer::NoLink);

        innerContainer->setLayout(BoundingFloats);  // in TmpParentContainer animate children containers
    } else {
        // Branch or mainbranch
        setLayout(Horizontal);

        linkContainer->setLinkStyle(LinkContainer::Line);

        switch (orientation) {
            case LeftOfParent:
                setHorizontalDirection(RightToLeft);
                innerContainer->setHorizontalDirection(RightToLeft);
                setBranchesContainerVerticalAlignment(AlignedRight);
                break;
            case RightOfParent:
                setHorizontalDirection(LeftToRight);
                innerContainer->setHorizontalDirection(LeftToRight);
                setBranchesContainerVerticalAlignment(AlignedLeft);
                break;
            case UndefinedOrientation:
                qWarning() << "BC::reposition - UndefinedOrientation in " << info();
                break;
            default:
                qWarning() << "BC::reposition - Unknown orientation " << orientation << " in " << info();
                break;
        }
    }

    // Depending on layouts, we might need to insert outerContainer and relink children
    updateChildrenStructure();

    // Update branchesContainer and linkSpaceContainer,
    // this even might remove these containers
    updateBranchesContainer();

    Container::reposition();

    // Update links
    if (branchesContainer && branchCount() > 0) {
        foreach (QGraphicsItem *g_item, branchesContainer->childItems()) {
            BranchContainer *bc = (BranchContainer*) g_item;
            bc->updateUpLink();
        }
    }

    // (tmpParentContainer has no branchItem!)
    if (branchItem) {
        XLinkObj *xlo;
        for (int i = 0; i < branchItem->xlinkCount(); ++i) {
            xlo = branchItem->getXLinkObjNum(i);
            if (xlo)
                xlo->updateXLink();
        }
    }

    // Update frames
    if (frameType != FrameContainer::NoFrame) {
        if (frameIncludeChildren) {
            if (outerContainer)
                setFrameRect( outerContainer->rect());
            else {
                setFrameRect(innerContainer->rect());
                setFramePos(innerContainer->pos());
            }
        } else {
            // Do not include children
            setFrameRect(ornamentsContainer->rect());
            setFramePos(mapFromItem(innerContainer, ornamentsContainer->pos()));
            /*
            qDebug() << "BC::repos  frame for " << ornamentsContainer->info();
            qDebug() << "                  ic " << innerContainer->info();
            qDebug() << "                  bc " << info();
            */
        }
    }
}
