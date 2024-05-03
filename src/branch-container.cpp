#include <QDebug>
#include <math.h>

#include "branch-container.h"

#include "branchitem.h"
#include "flag-container.h"
#include "flagrow-container.h"
#include "frame-container.h"
#include "heading-container.h"
#include "link-container.h"
#include "linkobj.h"
#include "mapdesign.h"
#include "misc.h"
#include "xlinkobj.h"

#define qdbg() qDebug().nospace().noquote()

extern FlagRowMaster *standardFlagsMaster;
extern FlagRowMaster *userFlagsMaster;
extern FlagRowMaster *systemFlagsMaster;

qreal BranchContainer::linkWidth = 20;  // FIXME-3 testing

BranchContainer::BranchContainer(QGraphicsScene *scene, BranchItem *bi)  // FIXME-4 scene and addItem should not be required, only for mapCenters without parent:  setParentItem automatically sets scene!
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

    if (upLink)
        delete upLink;
}

void BranchContainer::init()
{
    // BranchContainer defaults
    // partially overwriting MinimalBranchContainer
    // can be overwritten by MapDesign later
    containerType = Container::Branch;

    setLayout(Container::Horizontal);

    imagesContainerAutoLayout = true;

    branchesContainerAutoLayout = true;
    branchesContainerLayoutInt = Container::Vertical;

    scrollOpacity = 1;

    columnWidthAutoDesignInt = true;

    rotationsAutoDesignInt = true;
    rotationHeadingInt = 0;
    rotationSubtreeInt = 0;

    scaleAutoDesignInt = true;
    scaleHeadingInt = 1;
    scaleSubtreeInt = 1;

    autoDesignInnerFrame = true;
    autoDesignOuterFrame = true;

    // BranchContainers inherit FrameContainer, reset overlay
    overlay = false;

    // Setup children containers
    headingContainer = new HeadingContainer;

    innerFrame = nullptr;
    outerFrame = nullptr;

    ornamentsContainer = new Container;
    ornamentsContainer->containerType = OrnamentsContainer;

    linkContainer = new LinkContainer;

    innerContainer = new Container;
    innerContainer->containerType = InnerContainer;

    standardFlagRowContainer = nullptr;
    systemFlagRowContainer = nullptr;

    listContainer = nullptr;
    bulletPointContainer = nullptr;

    ornamentsContainer->addContainer(headingContainer, Z_HEADING);

    ornamentsContainer->setCentralContainer(headingContainer);

    innerContainer->addContainer(linkContainer, Z_LINK);
    innerContainer->addContainer(ornamentsContainer, Z_ORNAMENTS);

    linkSpaceContainer = nullptr;

    outerContainer = nullptr;

    addContainer(innerContainer);

    // Create an uplink for every branch 
    // This link will become the child of my parents
    // linkContainer later. This will allow moving when parent moves,
    // without recalculating geometry.
    upLink = new LinkObj;

    // Center of whole mainBranches should be the heading
    setCentralContainer(headingContainer);

    // Elastic layout experiments FIXME-2
    v.setParentItem(this);
    v.setPen(QPen(Qt::red));
    v.setVisible(false);

    /* Uncomment for testing
    QGraphicsEllipseItem *center = new QGraphicsEllipseItem (0, 0, 5, 5, this);
    center->setPen(QPen(Qt::blue));
    setPen(QPen(Qt::blue));
    */
}

BranchContainer* BranchContainer::parentBranchContainer()
{
    if (movingStateInt == TemporaryLinked && tmpLinkedParentContainer)
        return tmpLinkedParentContainer;

    if (movingStateInt == Moving)
        // Parent is tmpParentContainer, which technically is not a
        // BranchContainer, so don't return it here
        return nullptr;

    if (!branchItem)
        return nullptr;

    // For MapCenters parentBranch is rootItem and will return nullptr
    return branchItem->parentBranch()->getBranchContainer();
}

void BranchContainer::setBranchItem(BranchItem *bi) { branchItem = bi; }

BranchItem *BranchContainer::getBranchItem() const { return branchItem; }

QString BranchContainer::getName() {
    if (branchItem)
        return Container::getName() + " '" + branchItem->headingPlain() + "'";
    else
        return Container::getName() + " - ?";
}

void BranchContainer::setOriginalOrientation()
{
    originalOrientation = orientation;
    originalParentBranchContainer = parentBranchContainer();
    if (parentItem()) {
        if (parentBranchContainer())
            originalParentPos = parentBranchContainer()->downLinkPos();
        else
            originalParentPos = pos();
    }
}

BranchContainer::Orientation BranchContainer::getOriginalOrientation()
{
    return originalOrientation;
}

QPointF BranchContainer::getOriginalParentPos()
{
    return originalParentPos;
}

void BranchContainer::setOriginalScenePos()
{
    //qDebug() << "BC::sOSCP  of " << info();
    originalPos = scenePos();
}

void BranchContainer::updateVisibilityOfChildren()
{
    if (branchesContainer && branchItem)
    {
        if (branchItem->isScrolled()) {
            branchesContainer->setVisible(false);
            linkContainer->setVisible(false);
        } else {
            branchesContainer->setVisible(true);
            linkContainer->setVisible(true);
        }

        // Images of *this* branch may still stay visible, 
        // deeper down in the tree they will be hidden due to 
        // parent branch already being hidden
    }
}

#include <QTransform>
void BranchContainer::setScrollOpacity(qreal o)   // FIXME-3 testing for potential later animation
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

void BranchContainer::addToBranchesContainer(BranchContainer *bc)
{
    // Overloaded for real BranchContainers compared to MinimalBranchContainer

    if (!branchesContainer) {
        // Create branchesContainer before adding to it
        // (It will be deleted later in updateChildrenStructure(), if there
        // are no children)
        branchesContainer = new Container ();
        branchesContainer->containerType = Container::BranchesContainer;
        branchesContainer->zPos = Z_BRANCHES;
        branchesContainer->setLayout(branchesContainerLayoutInt);

        if (listContainer)
            listContainer->addContainer(branchesContainer);
        else
            innerContainer->addContainer(branchesContainer);

        branchesContainer->addContainer(bc);
        updateChildrenStructure();
    } else
        branchesContainer->addContainer(bc);
}

void BranchContainer::updateImagesContainer()
{
    if (imagesContainer && imagesContainer->childItems().count() == 0) {
        delete imagesContainer;
        imagesContainer = nullptr;
    }
}

void BranchContainer::createOuterContainer()
{
    if (!outerContainer) {
        outerContainer = new Container;
        outerContainer->containerType = OuterContainer;
        outerContainer->setLayout(BoundingFloats);
        addContainer(outerContainer);

        // Children structure is updated in updateChildrenStructure(), which is
        // anyway calling this method
    }
}

void BranchContainer::deleteOuterContainer()
{
    if (outerContainer) {
        // Before outerContainer get's deleted, it's children need to be reparented
        if (outerFrame)
            outerFrame->addContainer(innerContainer);
        else
            addContainer(innerContainer);
        if (imagesContainer)
            innerContainer->addContainer(imagesContainer);

        delete outerContainer;
        outerContainer = nullptr;
    }
}

void BranchContainer::updateTransformations()
{
    MapDesign *md = nullptr;
    int depth = 0;
    if (branchItem)  {
        md = branchItem->mapDesign();
        depth = branchItem->depth();
    } else
        return;

    if (!md)
        return;

    // Rotation of heading
    if (innerFrame)
        innerFrame->setRotation(rotationHeadingInt);
    else if (ornamentsContainer)
        ornamentsContainer->setRotation(rotationHeadingInt);

    // Scale of heading
    headingContainer->setScale(scaleHeadingInt);

    // Rotation and scaling of subtree
    if (outerFrame) {
        outerFrame->setRotation(rotationSubtreeInt);
        outerFrame->setScale(scaleSubtreeInt);
        if (outerContainer) {
            outerContainer->setRotation(0);
            outerContainer->setScale(1);
        } else {
            innerContainer->setRotation(0);
            innerContainer->setScale(1);
        }
    } else {
        if (outerContainer) {
            outerContainer->setRotation(rotationSubtreeInt);
            outerContainer->setScale(scaleSubtreeInt);
        } else {
            innerContainer->setRotation(rotationSubtreeInt);
            innerContainer->setScale(scaleSubtreeInt);
        }
    }
}

void BranchContainer::updateChildrenStructure() // FIXME-2 check if still a problem:
// When a map with list layout is loaded and 
// layout is switched to e.g. Vertical, the links 
// are not drawn. Has to be saved/loaded first
// Also: bullet points have bottomlines
{
    if (branchesContainerLayoutInt == List) {
        if (!listContainer) {
            // Create and setup a listContainer *below* the ornamentsContainer
            innerContainer->setLayout(Vertical);
            innerContainer->setHorizontalAlignment(AlignedLeft);

            // listContainer has one linkSpaceContainer left of branchesContainer
            listContainer = new Container;
            listContainer->containerType = Container::ListContainer;
            listContainer->setLayout(Horizontal);
            if (linkSpaceContainer)
                listContainer->addContainer(linkSpaceContainer);
            if (branchesContainer)
                listContainer->addContainer(branchesContainer);

            // Insert at end, especially behind innerFrame or ornamentsContainer
            innerContainer->addContainer(listContainer, Z_LIST);
            //qDebug() << "... Created listContainer in branch " << branchItem->headingPlain();

        }
    } else {
        // Switch back from listContainer to regular setup with innerContainer
        if (listContainer) {
            innerContainer->setLayout(Horizontal);
            if (linkSpaceContainer)
                innerContainer->addContainer(linkSpaceContainer);
            if (branchesContainer)
                innerContainer->addContainer(branchesContainer);
            delete listContainer;
            listContainer = nullptr;
        }
    }

    // The structure of subcontainers within a BranchContainer
    // depends on layouts of imagesContainer and branchesContainer:
    //
    // Usually both inagesContainer and branchesContainer are children of
    // innerContainer.  The layout of innerContainer is either Horizontal or
    // BoundingFloats. outerContainer is only needed in corner case d)
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

    // qDebug() << "BC::updateChildrenStructure() of " << info();

    if (branchesContainerLayoutInt != FloatingBounded && imagesContainerLayoutInt != FloatingBounded) {
        // a) No FloatingBounded images or branches
        deleteOuterContainer();
        if (listContainer)
            innerContainer->setLayout(Vertical);
        else
            innerContainer->setLayout(Horizontal);
    } else if (branchesContainerLayoutInt == FloatingBounded && imagesContainerLayoutInt != FloatingBounded) {
        // b) Only branches are FloatingBounded
        deleteOuterContainer();
        innerContainer->setLayout(BoundingFloats);
    } else if (branchesContainerLayoutInt == FloatingBounded && imagesContainerLayoutInt == FloatingBounded) {
        // c) images and branches are FloatingBounded
        deleteOuterContainer();
        innerContainer->setLayout(BoundingFloats);
    } else if (branchesContainerLayoutInt != FloatingBounded && imagesContainerLayoutInt == FloatingBounded) {
        // d) Only images are FloatingBounded
        createOuterContainer();
        if (listContainer)
            innerContainer->setLayout(Vertical);
        else
            innerContainer->setLayout(Horizontal);
    } else {
        // e) remaining cases
        deleteOuterContainer();
        innerContainer->setLayout(FloatingBounded);
    }

    updateTransformations();

    // Update structure of outerContainer
    if (outerContainer) {
        // outerContainer should be child of outerFrame, if this is used
        if (outerFrame)
            outerFrame->addContainer(outerContainer);
        else
            outerContainer->setParentItem(this);
        outerContainer->addContainer(innerContainer);
        if (imagesContainer)
            outerContainer->addContainer(imagesContainer);
    }

    // Structure for bullet point list layouts
    BranchContainer *pbc = parentBranchContainer();
    if (pbc && pbc->branchesContainerLayoutInt == List) {
        // Parent has list layout
        if (!bulletPointContainer) {
            //qDebug() << "... Creating bulletPointContainer";
            bulletPointContainer = new HeadingContainer;    // FIXME-3 create new type or re-use LinkObj and set type 
            // See also https://www.w3schools.com/charsets/ref_utf_punctuation.asp
            VymText vt(" • ");
            //vt.setColor(branchItem->headingColor());
            bulletPointContainer->setHeading(vt);
            bulletPointContainer->setColor(branchItem->headingColor());
            if (ornamentsContainer)
                ornamentsContainer->addContainer(bulletPointContainer, Z_BULLETPOINT);
        }
    } else {
        // Parent has no list layout
        if (bulletPointContainer) {
            delete bulletPointContainer;
            bulletPointContainer = nullptr;
        }
    }

    updateImagesContainer();

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
        // Space for links depends on layout and scrolled state:
        if (linkSpaceContainer) {
            if (hasFloatingBranchesLayout() || !branchItem || branchItem->isScrolled()) {
                delete linkSpaceContainer;
                linkSpaceContainer = nullptr;
            }
        } else {
            if (!hasFloatingBranchesLayout() && (!branchItem || !branchItem->isScrolled())) {
                linkSpaceContainer = new HeadingContainer ();
                linkSpaceContainer->setContainerType(LinkSpace);
                linkSpaceContainer->zPos = Z_LINKSPACE;
                linkSpaceContainer->setHeading(VymText("   "));  // FIXME-3 introduce minWidth later in Container instead of a pseudo heading here  see oc.pos

                if (listContainer)
                    listContainer->addContainer(linkSpaceContainer);
                else
                    innerContainer->addContainer(linkSpaceContainer);

                linkSpaceContainer->stackBefore(branchesContainer);
            }
        }
    }
}

void BranchContainer::createImagesContainer()
{
    // imagesContainer is created when images are added to branch
    // The destructor of ImageItem calls 
    // updateChildrenStructure() in parentBranch()
    imagesContainer = new Container ();
    imagesContainer->containerType = ImagesContainer;
    imagesContainer->setLayout(imagesContainerLayoutInt);

    if (outerContainer)
        outerContainer->addContainer(imagesContainer);
    else
        innerContainer->addContainer(imagesContainer);
}

void BranchContainer::addToImagesContainer(Container *c)
{
    if (!imagesContainer) {
        createImagesContainer();
    }

    QPointF sp = c->scenePos();
    imagesContainer->addContainer(c, Z_IMAGE);

    c->setPos(imagesContainer->sceneTransform().inverted().map(sp));
}

HeadingContainer* BranchContainer::getHeadingContainer()
{
    return headingContainer;
}

LinkContainer* BranchContainer::getLinkContainer()
{
    return linkContainer;
}

LinkObj* BranchContainer::getLink()
{
    return upLink;
}

void BranchContainer::linkTo(BranchContainer *pbc)
{

    if (!pbc) return;

    originalParentBranchContainer = nullptr;

    pbc->linkContainer->addLink(upLink);
}

QPointF BranchContainer::getPositionHintNewChild(Container *c)
{
    // Should we put new child c on circle around myself?
    bool useCircle = false;
    int n = 0;
    qreal radius;
    if (c->containerType == Branch && hasFloatingBranchesLayout()) {
        useCircle = true;
        radius = 190;
        n = branchCount();
    } else if (c->containerType == Image && hasFloatingImagesLayout()) {
        useCircle = true;
        radius = 100;
        n = imageCount();
    }

    if (useCircle) {
        // Mapcenter, suggest to put image or mainbranch on circle around center
        qreal a = 5 * M_PI_4 + M_PI_2 * n + (M_PI_4 / 2) * (n / 4 % 4);
        return QPointF (radius * cos(a), radius * sin(a));
    }

    QRectF r = headingContainer->rect();

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

QPointF BranchContainer::downLinkPos()
{
    return downLinkPos(orientation);
}

QPointF BranchContainer::downLinkPos(const Orientation &orientationChild)
{
    if (frameType(true) != FrameContainer::NoFrame) {
        if (!parentBranchContainer())
            // Framed MapCenter: Use center of frame    // FIXME-2 should depend on layout, not depth
            return ornamentsContainer->mapToScene(
                    ornamentsContainer->center());
        else {
            // Framed branch: Use left or right edge
            switch (orientationChild) {
                case RightOfParent:
                    return ornamentsContainer->mapToScene(
                            ornamentsContainer->rightCenter());
                case LeftOfParent:
                    return ornamentsContainer->mapToScene(
                            ornamentsContainer->leftCenter());
                default:    // Shouldn't happen...
                    return ornamentsContainer->mapToScene(
                            ornamentsContainer->bottomLeft());
            }
        }
    }

    // No frame, return bottom left/right corner
    switch (orientationChild) {
        case RightOfParent:
            return ornamentsContainer->mapToScene(
                    ornamentsContainer->bottomRight());
        case LeftOfParent:
            return ornamentsContainer->mapToScene(
                    ornamentsContainer->bottomLeft());
        default:
            return ornamentsContainer->mapToScene(
                    ornamentsContainer->bottomRight());
    }
}

QPointF BranchContainer::upLinkPos(const Orientation &orientationChild)
{
    if (frameType(true) != FrameContainer::NoFrame ||
        frameType(false) != FrameContainer::NoFrame) {
        if (!parentBranchContainer() && movingStateInt != Moving ) {
            // Framed MapCenter: Use center of frame    // FIXME-3 should depend on layout, not depth
            return ornamentsContainer->mapToScene(
                    ornamentsContainer->center());
        } else {
            // Framed branch: Use left or right edge
            switch (orientationChild) {
                case RightOfParent:
                    return ornamentsContainer->mapToScene(
                            ornamentsContainer->leftCenter());
                case LeftOfParent:
                    return ornamentsContainer->mapToScene(
                            ornamentsContainer->rightCenter());
                default: // mapcenter is moved, use bottomLeft corner
                    //qWarning() << "BC::upLinkPos  framed undefined orientation in " << info();
                    return ornamentsContainer->mapToScene(
                            ornamentsContainer->bottomLeft());
            }
        }
    }

    // For branches without frames, return bottom left/right corner
    switch (orientationChild) {
        case RightOfParent:
            return ornamentsContainer->mapToScene(
                    ornamentsContainer->bottomLeft());
        case LeftOfParent:
            return ornamentsContainer->mapToScene(
                    ornamentsContainer->bottomRight());
        default:
            qWarning() << "BC::upLinkPos  not framed undefined orientation in " << info();
            return ornamentsContainer->mapToScene(
                    ornamentsContainer->bottomLeft());
    }
}

void BranchContainer::updateUpLink()
{
    // Sets geometry and color of upLink and bottomline. 
    // Called from MapEditor e.g. during animation or 
    // from VymModel, when colors change

    // MapCenters still might have upLinks: The bottomLine is part of upLink!

    if (!isVisible())
        return;

    QPointF upLinkSelf_sp = upLinkPos(orientation);
    QPointF downLink_sp = downLinkPos();

    BranchContainer *pbc = nullptr;

    if (tmpLinkedParentContainer) {
        // I am temporarily linked to tmpLinkedParentContainer
        pbc = tmpLinkedParentContainer;
    } else if (originalParentBranchContainer)
        // I am moving with tmpParent, use original parent for link
        pbc = originalParentBranchContainer;
    else
        // Regular update, e.g. when linkStyle changes
        pbc = parentBranchContainer();

    BranchItem *tmpParentBI = nullptr;


    if (pbc) {
        tmpParentBI = pbc->getBranchItem();
        QPointF upLinkParent_sp;

        upLinkParent_sp = pbc->downLinkPos();
        upLinkParent_sp = pbc->downLinkPos(orientation);

        QGraphicsItem* upLinkParent = upLink->parentItem();
        if (!upLinkParent) return;

        upLink->setUpLinkPosParent(
                upLinkParent->sceneTransform().inverted().map(upLinkParent_sp));
        upLink->setUpLinkPosSelf(
                upLinkParent->sceneTransform().inverted().map(upLinkSelf_sp));
        upLink->setDownLinkPos(
                upLinkParent->sceneTransform().inverted().map(downLink_sp));
    } else {
        // I am a MapCenter without parent. Add LinkObj to my own LinkContainer,
        // so that at least positions are updated and bottomLine can be drawn
        linkContainer->addLink(upLink);

        upLink->setLinkStyle(LinkObj::NoLink);

        upLink->setUpLinkPosSelf(
                linkContainer->sceneTransform().inverted().map(upLinkSelf_sp));
        upLink->setDownLinkPos(
                linkContainer->sceneTransform().inverted().map(downLink_sp));
    }


    // Color of link (depends on current parent)
    if (upLink->getLinkColorHint() == LinkObj::HeadingColor)
        upLink->setLinkColor(branchItem->headingColor());
    else {
        if (branchItem)
            upLink->setLinkColor(branchItem->mapDesign()->defaultLinkColor());
    }

    // Style of link
    if (tmpParentBI) {
        if (pbc && pbc->branchesContainerLayoutInt == List)
            upLink->setLinkStyle(LinkObj::NoLink);
        else {
            upLink->setLinkStyle(tmpParentBI->mapDesign()->linkStyle(tmpParentBI->depth()));
        }
    }

    // Create/delete bottomline, depends on frame and (List-)Layout // FIXME-2 list layout not considered
                                                                    // also missing: if branch has children
    if (frameType(true) != FrameContainer::NoFrame) {
        if (upLink->hasBottomLine())
            upLink->deleteBottomLine();
    } else {
        if (!upLink->hasBottomLine())
            upLink->createBottomLine();
    }
    // Finally geometry
    upLink->updateLinkGeometry();
}

void BranchContainer::setLayout(const Layout &l)
{
    if (containerType != Branch && containerType != TmpParent)
        qWarning() << "BranchContainer::setLayout (...) called for non-branch: " << info();
    Container::setLayout(l);
}

void BranchContainer::setImagesContainerLayout(const Layout &ltype)
{
    if (imagesContainerLayoutInt == ltype)
        return;

    imagesContainerLayoutInt = ltype;

    if (imagesContainer)
        imagesContainer->setLayout(imagesContainerLayoutInt);

    updateChildrenStructure();
}

Container::Layout BranchContainer::imagesContainerLayout()
{
    return imagesContainerLayoutInt;
}

bool BranchContainer::hasFloatingImagesLayout()
{
    if (imagesContainer)
        return imagesContainer->hasFloatingLayout();

    if (imagesContainerLayoutInt == FloatingBounded || imagesContainerLayoutInt == FloatingFree)
        return true;
    else
        return false;
}

void BranchContainer::setBranchesContainerLayout(const Layout &layoutNew)
{
    branchesContainerLayoutInt = layoutNew;

    if (branchesContainer)
        branchesContainer->setLayout(branchesContainerLayoutInt);
    updateChildrenStructure();
}

Container::Layout BranchContainer::branchesContainerLayout()
{
    return branchesContainerLayoutInt;
}

bool BranchContainer::hasFloatingBranchesLayout()
{
    if (branchesContainer)
        return branchesContainer->hasFloatingLayout();

    if (branchesContainerLayoutInt == FloatingBounded || branchesContainerLayoutInt == FloatingFree)
        return true;
    else
        return false;
}



void BranchContainer::setBranchesContainerHorizontalAlignment(const HorizontalAlignment &a)
{
    branchesContainerHorizontalAlignment = a;
    if (branchesContainer)
        branchesContainer->setHorizontalAlignment(branchesContainerHorizontalAlignment);
}

void BranchContainer::setBranchesContainerBrush(const QBrush &b)
{
    branchesContainerBrush = b;
    if (branchesContainer)
        branchesContainer->setBrush(branchesContainerBrush);
}

QRectF BranchContainer::headingRect()
{
    // Returns scene coordinates of bounding rectanble
    return headingContainer->mapToScene(headingContainer->rect()).boundingRect();
}

QRectF BranchContainer::ornamentsRect()
{
    // Returns scene coordinates of bounding rectanble
    return ornamentsContainer->mapToScene(headingContainer->rect()).boundingRect();
}

void BranchContainer::setColumnWidthAutoDesign(const bool &b)
{
    columnWidthAutoDesignInt = b;
}

bool BranchContainer::columnWidthAutoDesign()
{
    return columnWidthAutoDesignInt;
}

void BranchContainer::setColumnWidth(const int &i)
{
    headingContainer->setColumnWidth(i);
    headingContainer->setHeading(branchItem->heading());
}

int BranchContainer::columnWidth()
{
    return headingContainer->columnWidth();
}

void BranchContainer::setRotationsAutoDesign(const bool &b, const bool &update) // FIXME-2 "update" parameter needed?
{
    rotationsAutoDesignInt = b;

    if (update)
        updateTransformations();
}

bool BranchContainer::rotationsAutoDesign()
{
    return rotationsAutoDesignInt;
}

void BranchContainer::setRotationHeading(const int &a)
{
    rotationHeadingInt = a;
    updateTransformations();
    //headingContainer->setScale(f + a * 1.1);      // FIXME-2 what about scaling?? Which transformCenter?
}

int BranchContainer::rotationHeading()
{
    return qRound(rotationHeadingInt);
}

int BranchContainer::rotationHeadingInScene()
{
    qreal r = rotationHeadingInt + rotationSubtreeInt;

    BranchContainer *pbc = parentBranchContainer();
    while (pbc) {
        r += pbc->rotationSubtree();
        pbc = pbc->parentBranchContainer();
    }

    return qRound(r);
}

void BranchContainer::setScaleAutoDesign(const bool &b, const bool &update)
{
    scaleAutoDesignInt = b;
    if (update)
        updateTransformations();
}

bool BranchContainer::scaleAutoDesign()
{
    return scaleAutoDesignInt;
}

void BranchContainer::setScaleHeading(const qreal &f)
{
    scaleHeadingInt = f;
    updateTransformations();
}

qreal BranchContainer::scaleHeading()
{
    return scaleHeadingInt;
}

void BranchContainer::setRotationSubtree(const int &a)
{
    rotationSubtreeInt = a;
    updateTransformations();
}

int BranchContainer::rotationSubtree()
{
    return qRound(rotationSubtreeInt);
}

void BranchContainer::setScaleSubtree(const qreal &f)
{
    scaleSubtreeInt = f;
    updateTransformations();
}

qreal BranchContainer::scaleSubtree()
{
    return scaleSubtreeInt;
}

void BranchContainer::setColor(const QColor &col)
{
    headingContainer->setColor(col);
    if (bulletPointContainer) // FIXME-3 duplicated code in updateChildrenStructure
        bulletPointContainer->setColor(col);
}

QUuid BranchContainer::findFlagByPos(const QPointF &p)
{
    QUuid uid;

    if (systemFlagRowContainer) {
        uid = systemFlagRowContainer->findFlagByPos(p);
        if (!uid.isNull())
            return uid;
    }

    if (standardFlagRowContainer)
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
    SelectableContainer::select(
	    ornamentsContainer,
	    branchItem->mapDesign()->selectionPen(),
	    branchItem->mapDesign()->selectionBrush());
}

bool BranchContainer::frameAutoDesign(const bool &useInnerFrame)
{
    if (useInnerFrame)
        return autoDesignInnerFrame;
    else
        return autoDesignOuterFrame;
}

void BranchContainer::setFrameAutoDesign(const bool &useInnerFrame, const bool &b)
{
    if (useInnerFrame)
        autoDesignInnerFrame = b;
    else
        autoDesignOuterFrame = b;
}

FrameContainer::FrameType BranchContainer::frameType(const bool &useInnerFrame)
{
    if (useInnerFrame) {
        if (innerFrame)
            return innerFrame->frameType();
        else
            return FrameContainer::NoFrame;
    }

    if (outerFrame)
        return outerFrame->frameType();
    else
        return FrameContainer::NoFrame;
}

QString BranchContainer::frameTypeString(const bool &useInnerFrame)
{
    if (useInnerFrame) {
        if (innerFrame)
            return innerFrame->frameTypeString();
    } else
        if (outerFrame)
            return outerFrame->frameTypeString();

    return "NoFrame";
}

void BranchContainer::setFrameType(const bool &useInnerFrame, const FrameContainer::FrameType &ftype)
{
    if (useInnerFrame) {
        // Inner frame around ornamentsContainer
        if (ftype == FrameContainer::NoFrame) {
            if (innerFrame) {
                innerContainer->addContainer(ornamentsContainer, Z_ORNAMENTS);
                ornamentsContainer->setRotation(innerFrame->rotation());
                ornamentsContainer->setPos(innerFrame->pos());
                delete innerFrame;
                innerFrame = nullptr;
            }
        } else {
            if (!innerFrame) {
                innerFrame = new FrameContainer;
                innerFrame->setUsage(FrameContainer::InnerFrame);
                innerFrame->addContainer(ornamentsContainer, Z_ORNAMENTS);
                innerFrame->setRotation(ornamentsContainer->rotation());
                innerFrame->setPos(ornamentsContainer->pos());
                innerContainer->addContainer(innerFrame, Z_INNER_FRAME);
                ornamentsContainer->setRotation(0);
            }
            innerFrame->setFrameType(ftype);
        }
    } else {
        // Outer frame around whole branchContainer including children
        if (ftype == FrameContainer::NoFrame) {
            // Remove outerFrame
            if (outerFrame) {
                int a = rotationSubtree();
                Container *c;
                if (outerContainer)
                    c = outerContainer;
                else
                    c = innerContainer;
                addContainer(c);
                delete outerFrame;
                outerFrame = nullptr;
                setRotationSubtree(a);
            }
        } else {
            // Set outerFrame
            if (!outerFrame) {
                outerFrame = new FrameContainer;
                outerFrame->setUsage(FrameContainer::OuterFrame);
                Container *c;
                if (outerContainer)
                    c = outerContainer;
                else
                    c = innerContainer;
                outerFrame->addContainer(c);
                addContainer(outerFrame, Z_OUTER_FRAME);
            }
            outerFrame->setFrameType(ftype);
        }
    }
    updateChildrenStructure();
}

void BranchContainer::setFrameType(const bool &useInnerFrame, const QString &s)
{
    FrameContainer::FrameType ftype = FrameContainer::frameTypeFromString(s);
    setFrameType(useInnerFrame, ftype);
}

int BranchContainer::framePadding(const bool &useInnerFrame)
{
    if (useInnerFrame) {
        if (innerFrame)
            return innerFrame->framePadding();
    } else {
        if (outerFrame)
            return outerFrame->framePadding();
    }

    return 0;
}

void BranchContainer::setFramePadding(const bool &useInnerFrame, const int &p)
{
    if (useInnerFrame) {
        if (innerFrame)
            innerFrame->setFramePadding(p);
    } else {
        if (outerFrame)
            outerFrame->setFramePadding(p);
    }
}

int BranchContainer::framePenWidth(const bool &useInnerFrame)
{
    if (useInnerFrame) {
        if (innerFrame)
            return innerFrame->framePenWidth();
    } else {
        if (outerFrame)
            return outerFrame->framePenWidth();
    }

    return 0;
}

void BranchContainer::setFramePenWidth(const bool &useInnerFrame, const int &w)
{
    if (useInnerFrame) {
       if (innerFrame)
           innerFrame->setFramePenWidth(w);
    } else {
        if (outerFrame)
           outerFrame->setFramePenWidth(w);
    }
}

QColor BranchContainer::framePenColor(const bool &useInnerFrame)
{
    if (useInnerFrame) {
        if (innerFrame)
            return innerFrame->framePenColor();
    } else {
        if (outerFrame)
            return outerFrame->framePenColor();
    }

    return QColor();
}

void BranchContainer::setFramePenColor(const bool &useInnerFrame, const QColor &c)
{
    if (useInnerFrame) {
        if (innerFrame)
            innerFrame->setFramePenColor(c);
    } else {
        if (outerFrame)
            outerFrame->setFramePenColor(c);
    }
}

QColor BranchContainer::frameBrushColor(const bool &useInnerFrame)
{
    if (useInnerFrame) {
        if (innerFrame)
            return innerFrame->frameBrushColor();
    } else {
        if (outerFrame)
            return outerFrame->frameBrushColor();
    }

    return QColor();
}

void BranchContainer::setFrameBrushColor(const bool &useInnerFrame, const QColor &c)
{
    if (useInnerFrame) {
        if (innerFrame)
            innerFrame->setFrameBrushColor(c);
    } else {
        if (outerFrame)
            outerFrame->setFrameBrushColor(c);
    }
}

QString BranchContainer::saveFrame()
{
    QString r;
    if (innerFrame && innerFrame->frameType() != FrameContainer::NoFrame)
        r = innerFrame->saveFrame();

    if (outerFrame && outerFrame->frameType() != FrameContainer::NoFrame)
        r += outerFrame->saveFrame();
    return r;
}

void BranchContainer::updateVisuals()
{
    // Update heading
    if (!branchItem)
        return;

    headingContainer->setHeading(branchItem->heading());

    // Update standard flags active in TreeItem
    QList<QUuid> TIactiveFlagUids = branchItem->activeFlagUids();
    if (TIactiveFlagUids.count() == 0) {
        if (standardFlagRowContainer) {
            delete standardFlagRowContainer;
            standardFlagRowContainer = nullptr;
        }
    } else {
        if (!standardFlagRowContainer) {
            standardFlagRowContainer = new FlagRowContainer;
            ornamentsContainer->addContainer(standardFlagRowContainer, Z_STANDARD_FLAGS);
        }
        standardFlagRowContainer->updateActiveFlagContainers(
            TIactiveFlagUids, standardFlagsMaster, userFlagsMaster);
    }

    // Add missing system flags active in TreeItem
    TIactiveFlagUids = branchItem->activeSystemFlagUids();
    if (TIactiveFlagUids.count() == 0) {
        if (systemFlagRowContainer) {
            delete systemFlagRowContainer;
            systemFlagRowContainer = nullptr;
        }
    } else {
        if (!systemFlagRowContainer) {
            systemFlagRowContainer = new FlagRowContainer;
            ornamentsContainer->addContainer(systemFlagRowContainer, Z_SYSTEM_FLAGS);
        }
        systemFlagRowContainer->updateActiveFlagContainers(
            TIactiveFlagUids, systemFlagsMaster);
    }
}

void BranchContainer::reposition()
{
    // Set orientation based layout or
    // in the process of being (temporary) relinked
    BranchContainer *pbc = parentBranchContainer();

    if (movingStateInt == Moving || movingStateInt == TemporaryLinked)
        // I am currently attached to tmpParentContainer
        orientation = ((BranchContainerBase*)(parentContainer()->parentContainer()))->getOrientation();
    else {
        // pbc is now either the temporary parent or the original one, depending on MovingState
        if (pbc) {
            if (pbc->hasFloatingBranchesLayout()) {
                if (pos().x() > 0)
                    orientation = RightOfParent;
                else
                    orientation = LeftOfParent;
            } else
                orientation = pbc->getOrientation();
        } else
            orientation = BranchContainerBase::UndefinedOrientation;
    }

    /*
    qdbg() << ind() << "BC::reposition  bc=" <<      info() << "  orient=" << orientation;
    if (pbc) {
        qdbg() << ind() << "          pbc=" << pbc->info();
        qdbg() << ind() << "          pbc->orientation=" << pbc->orientation;
    } else
        qdbg() << ind() << "          pbc=0";
    */
    //qdbg() << ind() << "          state=" << movingStateInt;
    // Settings depending on depth
    uint depth = 0;
    if (branchItem)
        depth = branchItem->depth();

    if (depth == 0)
    {
        // MapCenter
        setHorizontalDirection(LeftToRight);
        // FIXME-2 set in updateChildrenStructure: innerContainer->setHorizontalDirection(LeftToRight);

        // FIXME-2 set in updateChildrenStructure: innerContainer->setLayout(BoundingFloats);
    } else {
        // Branch or mainbranch
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
            case UndefinedOrientation:
                qWarning() << "BC::reposition - UndefinedOrientation in " << info();
                break;
            default:
                qWarning() << "BC::reposition - Unknown orientation " << orientation << " in " << info();
                break;
        }
    }

    Container::reposition();

    // Update links of children
    if (branchesContainer && branchCount() > 0) {
        foreach (Container *c, branchesContainer->childContainers()) {
            BranchContainer *bc = (BranchContainer*) c;
            bc->updateUpLink();
        }
    }

    // Update my own bottomLine, in case I am a MapCenter 
    if (depth == 0)
        updateUpLink();

    // Update XLinks
    if (branchItem) {
        XLinkObj *xlo;
        for (int i = 0; i < branchItem->xlinkCount(); ++i) {
            xlo = branchItem->getXLinkObjNum(i);
            if (xlo)
                xlo->updateGeometry();
        }
    }
}
