#include "branchitem.h"

#include "attributeitem.h"
#include "frame-container.h"
#include "heading-container.h"
#include "image-container.h"
#include "task.h"
#include "taskmodel.h"
#include "vymmodel.h"
#include "xlink.h"
#include "xlinkitem.h"

extern TaskModel *taskModel;

BranchItem::BranchItem(TreeItem *parent)
    : MapItem(parent)
{
    //qDebug()<< "Constr. BranchItem this=" << this << "parent:" << parent;

    // Set type if parent is known yet
    // if not, type is set in insertBranch or TreeItem::appendChild
    if (parent == rootItem)
        setType(MapCenter);
    else
        setType(Branch);

    scrolled = false;
    tmpUnscrolled = false;

    lastSelectedBranchNum = 0;
    lastSelectedBranchNumAlt = 0;

    task = nullptr;

    branchContainer = nullptr;
}

BranchItem::~BranchItem()
{
    //qDebug() << "Destr. BranchItem: this=" << this << "  " << getHeadingPlain() << "branchContainer=" << branchContainer;
    if (branchContainer) {
        // This deletes only the first container here.
        // All other containers deeper down in tree will unlink themselves 
        // by calling BranchItem::unlinkBranchContainer, which will set 
        // the branchContainer == nullptr;
        //
        // QGraphicsItems such as BranchContainer will delete all their children 
        // themselves
        delete branchContainer;
    }

    clear();
}

void BranchItem::clear()
{
    if (task)
        taskModel->deleteTask(task);
}

void BranchItem::copy(BranchItem *other) // TODO lacks most of data...
{
    scrolled = other->scrolled;
    tmpUnscrolled = other->tmpUnscrolled;
}

BranchItem *BranchItem::parentBranch() { return (BranchItem *)parentItem; }

void BranchItem::insertBranch(int pos, BranchItem *branch)
{
    if (pos < 0)
        pos = 0;
    if (pos > branchCounter)
        pos = branchCounter;
    childItems.insert(pos + branchOffset, branch);
    branch->parentItem = this;
    branch->rootItem = rootItem;
    branch->setModel(model);
    if (parentItem == rootItem)
        setType(MapCenter);
    else
        setType(Branch);

    if (branchCounter == 0)
        branchOffset = childItems.count() - 1;
    branchCounter++;
}

QString BranchItem::saveToDir(const QString &tmpdir, const QString &prefix,
                              const QPointF &offset, QList<Link *> &tmpLinks)
{
    // Cloudy stuff can be hidden during exports
    if (hidden)
        return QString();

    // Save uuid
    QString idAttr = attribut("uuid", uuid.toString());

    QString s;

    // Update of note is usually done while unselecting a branch

    QString scrolledAttr;
    if (scrolled)
        scrolledAttr = attribut("scrolled", "yes");
    else
        scrolledAttr = "";

    // save area, if not scrolled   // not needed if HTML is rewritten...
    // also we could check if _any_ of parents is scrolled
    QString areaAttr;   // FIXME-2 will not work with rotated containers and positions of headings
    if (branchContainer && parentItem->hasTypeBranch() &&
        !((BranchItem *)parentItem)->isScrolled()) {
        qreal x = branchContainer->scenePos().x();
        qreal y = branchContainer->scenePos().y();
        areaAttr =
            attribut("x1", QString().setNum(x - offset.x())) +
            attribut("y1", QString().setNum(y - offset.y())) +
            attribut("x2", QString().setNum(x + branchContainer->rect().width() - offset.x())) +
            attribut("y2", QString().setNum(y + branchContainer->rect().height() - offset.y()));
    }
    else
        areaAttr = "";

    QString elementName;
    if (parentItem == rootItem)
        elementName = "mapcenter";
    else
        elementName = "branch";

    // Free positioning of children         // FIXME-2 remove BI::FreePositioning, save laouts for branchesContainer and imagesContainer instead
    QString layoutBranchesAttr;
    QString autoLayoutBranchesAttr;
    if (!branchContainer->branchesContainerAutoLayout)
    {
        // Save the manually set layout for children branches
        layoutBranchesAttr = attribut("branchesLayout", branchContainer->getLayoutString(branchContainer->getBranchesContainerLayout()));
    }

    QString layoutImagesAttr;
    if (!branchContainer->imagesContainerAutoLayout)
    {
        // Save the manually set layout for children Images
        layoutImagesAttr = attribut("imagesLayout", branchContainer->Container::getLayoutString(branchContainer->getImagesContainerLayout()));
    }

    QString rotHeadingAttr;
    qreal a = branchContainer->getRotationHeading();
    if (a != 0)
        rotHeadingAttr = attribut("rotHeading", QString("%1").arg(a));

    QString rotContentAttr;
    a = branchContainer->getRotationSubtree();
    if (a != 0)
        rotContentAttr = attribut("rotContent", QString("%1").arg(a));

    QString posAttr;
    if (parentItem == rootItem || branchContainer->isFloating())
        posAttr = getPosAttr();

    s = beginElement(elementName +
            posAttr +
            MapItem::getLinkableAttr() +
            TreeItem::getGeneralAttr() +
                     scrolledAttr +
                     layoutBranchesAttr +
                     layoutImagesAttr +
                     rotHeadingAttr +
                     rotContentAttr +
                     idAttr);
    incIndent();

    // save heading
    s += heading.saveToDir();

    // save note
    if (!note.isEmpty())
        s += note.saveToDir();

    // Save frame
    if (branchContainer->frameType(true) != FrameContainer::NoFrame ||
        branchContainer->frameType(false) != FrameContainer::NoFrame)
        s += branchContainer->saveFrame();

    // save names of flags set
    s += standardFlags.saveState();
    s += userFlags.saveState();

    // Save Images
    for (int i = 0; i < imageCount(); ++i)
        s += getImageNum(i)->saveToDir(tmpdir, prefix);

    // save attributes
    for (int i = 0; i < attributeCount(); ++i)
        s += getAttributeNum(i)->getDataXML();

    // save task
    if (task)
        s += task->saveToDir();

    // Save branches
    int i = 0;
    TreeItem *ti = getBranchNum(i);
    while (ti) {
        s += getBranchNum(i)->saveToDir(tmpdir, prefix, offset, tmpLinks);
        i++;
        ti = getBranchNum(i);
    }

    // Mark Links for save
    for (int i = 0; i < xlinkCount(); ++i) {
        Link *l = getXLinkItemNum(i)->getLink();
        if (l && !tmpLinks.contains(l))
            tmpLinks.append(l);
    }
    decIndent();
    s += endElement(elementName);
    return s;
}

void BranchItem::updateVisibility()
{
    // Needed to hide relinked branch, if parent is scrolled
    if (hasScrolledParent(this) || hidden)
        branchContainer->setVisibility(false);
    else
        branchContainer->setVisibility(true);
}

void BranchItem::setHeadingColor(QColor color)
{
    TreeItem::setHeadingColor(color);
    branchContainer->getHeadingContainer()->setHeadingColor(color);
    branchContainer->updateStyles(MapDesign::NotCreated, MapDesign::ColorChanged);
}

void BranchItem::updateTaskFlag()
{
    systemFlags.deactivateGroup("system-tasks");
    if (task) {
        QString s = "system-" + task->getIconString();
        systemFlags.activate(s);
        model->emitDataChanged(this);
    }
    // else: During initialization the task is not yet attached to branch,
    // so ignore it for now
}

void BranchItem::setTask(Task *t)
{
    task = t;
    updateTaskFlag();
}

Task *BranchItem::getTask() { return task; }

void BranchItem::scroll()
{
    if (tmpUnscrolled)
        resetTmpUnscroll();
    if (!scrolled)
        toggleScroll();
}
void BranchItem::unScroll()
{
    if (tmpUnscrolled)
        resetTmpUnscroll();
    if (scrolled)
        toggleScroll();
}

bool BranchItem::toggleScroll() // FIXME-2 check visibility of LinkObjs
{
    // MapCenters are not scrollable
    if (depth() == 0)
        return false;

    BranchContainer *bc;
    if (scrolled) {
        scrolled = false;
        systemFlags.deactivate(QString("system-scrolledright"));
        if (branchCounter > 0)
            for (int i = 0; i < branchCounter; ++i) {
                bc = getBranchNum(i)->getBranchContainer();
                if (bc)
                    bc->setVisibility(true);
            }
    }
    else {
        scrolled = true;
        systemFlags.activate(QString("system-scrolledright"));
        if (branchCounter > 0)
            for (int i = 0; i < branchCounter; ++i) {
                bc = getBranchNum(i)->getBranchContainer();
                if (bc)
                    bc->setVisibility(false);
            }
    }

    return true;
}

bool BranchItem::isScrolled() { return scrolled; }

bool BranchItem::hasScrolledParent(BranchItem *start)
{
    // Calls parents recursivly to
    // find out, if we are scrolled at all.
    // But ignore myself, just look at parents.

    if (!start)
        start = this;

    if (this != start && scrolled)
        return true;

    BranchItem *bi = (BranchItem *)parentItem;
    if (bi && bi != rootItem)
        return bi->hasScrolledParent(start);
    else
        return false;
}

bool BranchItem::tmpUnscroll(BranchItem *start)
{
    bool result = false;

    if (!start)
        start = this;

    // Unscroll parent (recursivly)
    BranchItem *pi = (BranchItem *)parentItem;
    if (pi && pi->hasTypeBranch())
        result = pi->tmpUnscroll(start);

    // Unscroll myself
    if (start != this && scrolled) {
        tmpUnscrolled = true;
        systemFlags.activate(QString("system-tmpUnscrolledRight"));
        toggleScroll();
        model->emitDataChanged(this);
        result = true;
    }
    return result;
}

bool BranchItem::resetTmpUnscroll()
{
    bool result = false;

    // Unscroll parent (recursivly)
    BranchItem *pi = (BranchItem *)parentItem;
    if (pi && pi->hasTypeBranch())
        result = pi->resetTmpUnscroll();

    // Unscroll myself
    if (tmpUnscrolled) {
        tmpUnscrolled = false;
        systemFlags.deactivate(QString("system-tmpUnscrolledRight"));
        toggleScroll();
        model->emitDataChanged(this);
        result = true;
    }
    return result;
}

void BranchItem::sortChildren(bool inverse) // FIXME-4 optimize by not using moveUp/Down
{
    int childCount = branchCounter;
    int curChildIndex;
    bool madeChanges = false;
    do {
        madeChanges = false;
        for (curChildIndex = 1; curChildIndex < childCount; curChildIndex++) {
            BranchItem *curChild = getBranchNum(curChildIndex);
            BranchItem *prevChild = getBranchNum(curChildIndex - 1);
            if (inverse) {
                if (prevChild->getHeadingPlain().compare(
                        curChild->getHeadingPlain(), Qt::CaseInsensitive) < 0) {
                    model->moveUp(curChild);
                    madeChanges = true;
                }
            } else if (prevChild->getHeadingPlain().compare(
                         curChild->getHeadingPlain(), Qt::CaseInsensitive) >
                     0) {
                model->moveUp(curChild);
                madeChanges = true;
            }
        }
    } while (madeChanges);
}

void BranchItem::setBranchesLayout(const QString &s)
{
    branchContainer->setBranchesContainerLayout(Container::getLayoutFromString(s));
}

void BranchItem::setImagesLayout(const QString &s)
{
    branchContainer->setImagesContainerLayout(Container::getLayoutFromString(s));
}

QColor BranchItem::getBackgroundColor(BranchItem *start, bool checkInnerFrame)
{
    // Determine background color in taskEditor, first try inner frame
    if (checkInnerFrame && branchContainer->frameType(true) != FrameContainer::NoFrame)
            return branchContainer->frameBrushColor(true);

    // Outer frame
    if (branchContainer->frameType(false) != FrameContainer::NoFrame)
            return branchContainer->frameBrushColor(false);

    BranchItem *pb = parentBranch();
    if (pb && pb != rootItem)
        // Recursively try parents and check for frames there
        return pb->getBackgroundColor(start, false);
    else
        // No frame found
        return model->getMapBackgroundColor();
}

void BranchItem::setLastSelectedBranch()
{
    int d = depth();
    if (d >= 0) {
        if (d == 1)
            // Hack to save an additional lastSelected for mapcenters in
            // MapEditor depending on orientation this allows to go both left
            // and right from there

            if (branchContainer->getOrientation() ==
                          BranchContainer::LeftOfParent) {
                ((BranchItem *)parentItem)->lastSelectedBranchNumAlt =
                    parentItem->num(this);
                return;
            }

        ((BranchItem *)parentItem)->lastSelectedBranchNum =
            parentItem->num(this);
    }
}

void BranchItem::setLastSelectedBranch(int i) { lastSelectedBranchNum = i; }

BranchItem *BranchItem::getLastSelectedBranch()
{
    if (lastSelectedBranchNum >= branchCounter)
        return getBranchNum(branchCounter - 1);
    else
        return getBranchNum(lastSelectedBranchNum);
}

BranchItem *BranchItem::getLastSelectedBranchAlt()
{
    return getBranchNum(lastSelectedBranchNumAlt);
}

TreeItem *BranchItem::findMapItem(QPointF p, QList <TreeItem*> excludedItems)
{
    // Search branches
    TreeItem *ti;
    for (int i = 0; i < branchCount(); ++i) {
        ti = getBranchNum(i)->findMapItem(p, excludedItems);
        if (ti != nullptr)
            return ti;
    }

    // Search images
    ImageItem *ii;
    ImageContainer *ic;
    for (int i = 0; i < imageCount(); ++i) {
        ii = getImageNum(i);
        ic = ii->getImageContainer();
        if (!excludedItems.contains(ii) && ic->mapToScene(ic->rect()).containsPoint(p, Qt::OddEvenFill)) return ii;
    }

    // Search my container     // FIXME-2   Check if container is visible!! (Maybe done automatically)
    if (branchContainer->isInClickBox(p) && !excludedItems.contains(this) ) //   &&
        //getBranchObj()->isVisibleObj())
        return this;

    return nullptr;
}

void BranchItem::updateStylesRecursively(
        MapDesign::CreationMode creationMode,
        MapDesign::RelinkMode relinkMode)
{
    // Update my own container
    if (branchContainer)
        branchContainer->updateStyles(creationMode, relinkMode);

    // Recursively update subtree
    for (int i = 0; i < branchCounter; i++) {
        getBranchNum(i)->updateStylesRecursively(creationMode, relinkMode);
    }
}

void BranchItem::updateVisuals()
{
    branchContainer->updateVisuals();
}

BranchContainer *BranchItem::createBranchContainer(QGraphicsScene *scene)
{
    branchContainer = new BranchContainer(scene, this);

    // Set visibility depending on parents  // FIXME-2
    /*
    if (parentItem != rootItem &&
        (((BranchItem *)parentItem)->scrolled ||
         !((MapItem *)parentItem)->getLMO()->isVisibleObj()))
        newbo->setVisibility(false);
    */

    // For mainbranches get a position hint // FIXME-2 probably also other floating branches...
    if (depth() == 1)
        branchContainer->setPos(parentBranch()->getBranchContainer()->getPositionHintNewChild(branchContainer));

    // FIXME-2 for new branch set default font, color, link, frame, children styles
    // newbo->setDefAttr(BranchObj::NewBranch);
    // FIXME-2 should be ok after contstructer without manual update - branchContainer->updateStyles(BranchContainer::NewBranch);

    if (!getHeading().isEmpty()) {  // FIXME-2 updateVisuals new container and color
        /*
        newbo->updateVisuals();
        newbo->setColor(heading.getColor());
        */
    }

    // Link to parent branch visually by
    // adding my upLink to parents linkContainer
    if (parentBranch())
        branchContainer->linkTo(parentBranch()->getBranchContainer());

    return branchContainer;
}

BranchContainer* BranchItem::getBranchContainer()
{
    return branchContainer;
}

void BranchItem::unlinkBranchContainer()
{
    //qDebug() << "BI::unlinkBC in " << this << getHeadingPlain();

    // Called from destructor of containers to 
    // avoid double deletion 
    branchContainer = nullptr;
}

Container* BranchItem::getBranchesContainer()
{
    return branchContainer->getBranchesContainer();
}

void BranchItem::updateContainerStackingOrder()
{
    // After relinking branches (also moving up/down), the order of the 
    // BranchContainers does not match the order of BranchItems any longer and
    // needs to be adjusted. Or the BranchContainer has (temporarily) been linked to 
    // a completely different parent.
    //
    // It seems the QGraphicsItem::stackBefore only works, if an item is moved up. 
    // For moving below (or into another subtree), we have to reparent first  :-(

    // For simplicity we always reparent. The absolute position will not be changed here

    int n = num();

    QPointF sp = branchContainer->scenePos();

    branchContainer->setParentItem(nullptr);

    if (parentBranch() == rootItem)
        // I am the center
        return;

    parentBranch()->addToBranchesContainer(branchContainer);

    while (n < parentBranch()->branchCount() - 1) {
        // Insert container of this branch above others

        // The next sibling container might currently still be temporarily 
        // linked to tmpParentContainer, in that case it is not a sibling and 
        // cannot be inserted using QGraphicsItem::stackBefore
        //
        // We try the next sibling then, if this fails, just append at the end.
        if ( (parentBranch()->getBranchNum(n + 1))->getContainer()->parentItem() != parentBranch()->getBranchesContainer() )
            n++;
        else {
            branchContainer->stackBefore( (parentBranch()->getBranchNum(n + 1))->getContainer() );
            break;
        }
    }

    branchContainer->setPos(branchContainer->parentItem()->sceneTransform().inverted().map(sp));
}

void BranchItem::addToBranchesContainer(BranchContainer *bc)
{
    branchContainer->addToBranchesContainer(bc);
}

void BranchItem::addToImagesContainer(ImageContainer *ic)
{
    // Keep scene position while relinking image container, so pass "true"
    branchContainer->addToImagesContainer(ic, true);
}

void BranchItem::repositionContainers()
{
    branchContainer->reposition();
}
