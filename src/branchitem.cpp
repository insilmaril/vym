#include "branchitem.h"

#include "attributeitem.h"
#include "branch-container.h"
#include "branch-wrapper.h"
#include "frame-container.h"
#include "heading-container.h"
#include "image-container.h"
#include "link-container.h"
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

    branchWrapperInt = nullptr;

    branchContainer = nullptr;
}

BranchItem::~BranchItem()
{
    //qDebug() << "Destr. BranchItem: this=" << this << "  " << headingPlain() << "branchContainer=" << branchContainer;
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

    if (task)
        taskModel->deleteTask(task);

    if (branchWrapperInt) {
        delete branchWrapperInt;
        branchWrapperInt = nullptr;
    }
}

void BranchItem::copy(BranchItem *other) // TODO lacks most of data...
{
    scrolled = other->scrolled;
    tmpUnscrolled = other->tmpUnscrolled;
}

BranchItem *BranchItem::parentBranch()
{
    // For MapCenters this will return rootItem
    return (BranchItem *)parentItem;
}

BranchWrapper *BranchItem::branchWrapper()
{
    if (!branchWrapperInt)
        branchWrapperInt = new BranchWrapper(this);

    return branchWrapperInt;
}

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

void BranchItem::insertImage(int pos, ImageItem *image)
{
    if (pos < 0)
        pos = 0;
    if (pos > imageCounter)
        pos = imageCounter;
    childItems.insert(pos + imageOffset, image);

    // Set parentItem, rootItem and model
    image->setParentBranch(this);

    imageCounter++;
    branchOffset++;
}

QString BranchItem::saveToDir(const QString &tmpdir, const QString &prefix,
                              const QPointF &offset, QList<XLink *> &tmpXLinks, const bool &exportBoundingBoxes)
{
    // Cloudy stuff can be hidden during exports
    if (hidden)
        return QString();

    QString attr;

    // Save uuid
    attr += attribute("uuid", uuid.toString());

    // vymLink, Url, hideExport, localTarget
    attr += getGeneralAttr();

    // Update of note is usually done while unselecting a branch

    if (scrolled)
        attr += attribute("scrolled", "yes");

    // save area, if not scrolled
    // FIXME-5 we could check if _any_ of parents is scrolled
    if (exportBoundingBoxes && branchContainer && parentItem->hasTypeBranch() &&
        !((BranchItem *)parentItem)->isScrolled()) {
        QRectF r_bc = branchContainer->mapToScene(branchContainer->rect()).boundingRect();
        attr +=
            attribute("x1", QString().setNum(r_bc.topLeft().x() - offset.x())) +
            attribute("y1", QString().setNum(r_bc.topLeft().y() - offset.y())) +
            attribute("x2", QString().setNum(r_bc.bottomRight().x() - offset.x())) +
            attribute("y2", QString().setNum(r_bc.bottomRight().y() - offset.y()));
    }

    QString elementName;
    if (parentItem == rootItem)
        elementName = "mapcenter";
    else
        elementName = "branch";

    // Free positioning of children
    if (!branchContainer->branchesContainerAutoLayout)
        // Save the manually set layout for children branches
        attr += attribute("branchesLayout", branchContainer->layoutString(branchContainer->branchesContainerLayout()));

    if (!branchContainer->imagesContainerAutoLayout)
        // Save the manually set layout for children Images
        attr += attribute("imagesLayout", branchContainer->Container::layoutString(branchContainer->imagesContainerLayout()));

    if (!branchContainer->rotationsAutoDesign()) {
        attr += attribute("rotHeading", QString("%1").arg(branchContainer->rotationHeading()));

        attr += attribute("rotSubtree", QString("%1").arg(branchContainer->rotationSubtree()));
    }

    if (!branchContainer->scaleAutoDesign()) {
        attr += attribute("scaleHeading", QString("%1").arg(branchContainer->scaleHeading()));

        attr += attribute("scaleSubtree", QString("%1").arg(branchContainer->scaleSubtree()));
    }

    // width of heading
    if (!branchContainer->columnWidthAutoDesign())
        attr += attribute("colWidth", QString("%1").arg(branchContainer->getHeadingContainer()->columnWidth()));

    if (parentItem == rootItem || branchContainer->isFloating())
        attr += getPosAttr();

    QString s = beginElement(elementName + " " + attr);
    incIndent();

    // save heading
    s += headingInt.saveToDir();

    // save note
    if (!note.isEmpty())
        s += note.saveToDir();

    // Save frame
    if (branchContainer->frameType(true) != FrameContainer::NoFrame ||
        branchContainer->frameType(false) != FrameContainer::NoFrame)
        s += branchContainer->saveFrame();

    // save names of flag set
    s += standardFlags.saveState();
    s += userFlags.saveState();

    // Save Images
    for (int i = 0; i < imageCount(); ++i)
        s += getImageNum(i)->saveToDir(tmpdir);

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
        s += getBranchNum(i)->saveToDir(tmpdir, prefix, offset, tmpXLinks, exportBoundingBoxes);
        i++;
        ti = getBranchNum(i);
    }

    // Mark Links for save
    for (int i = 0; i < xlinkCount(); ++i) {
        XLink *xl = getXLinkItemNum(i)->getXLink();
        if (xl && !tmpXLinks.contains(xl))
            tmpXLinks.append(xl);
    }
    decIndent();
    s += endElement(elementName);
    return s;
}

void BranchItem::setHeadingColor(QColor color)
{
    TreeItem::setHeadingColor(color);
    branchContainer->setColor(color);
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

bool BranchItem::toggleScroll()
{
    // MapCenters are not scrollable
    if (depth() == 0)
        return false;

    if (scrolled) {
        scrolled = false;
        systemFlags.deactivate(QString("system-scrolledright"));
    }
    else {
        scrolled = true;
        systemFlags.activate(QString("system-scrolledright"));
    }

    branchContainer->updateChildrenStructure();     // needed to insert linkSpaceContainer
    branchContainer->updateVisibilityOfChildren();
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

void BranchItem::setBranchesLayout(const QString &s)
{
    branchContainer->setBranchesContainerLayout(Container::layoutFromString(s));
}

void BranchItem::setImagesLayout(const QString &s)
{
    branchContainer->setImagesContainerLayout(Container::layoutFromString(s));
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
        return model->mapDesign()->backgroundColor();
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
    if (!isScrolled()) {
        TreeItem *ti;
        for (int i = 0; i < branchCounter; ++i) {
            ti = getBranchNum(i)->findMapItem(p, excludedItems);
            if (ti != nullptr)
                return ti;
        }
    }

    // Search images
    ImageItem *ii;
    ImageContainer *ic;
    for (int i = 0; i < imageCount(); ++i) {
        ii = getImageNum(i);
        ic = ii->getImageContainer();
        if (!excludedItems.contains(ii) && ic->isVisible() && ic->mapToScene(ic->rect()).containsPoint(p, Qt::OddEvenFill)) return ii;
    }

    // Search my container
    if (branchContainer->isVisible() && branchContainer->isInClickBox(p) && !excludedItems.contains(this) ) //   &&
        //getBranchObj()->isVisibleObj())
        return this;

    return nullptr;
}

void BranchItem::updateVisuals()
{
    branchContainer->updateVisuals();
}

BranchContainer *BranchItem::createBranchContainer(QGraphicsScene *scene)
{
    branchContainer = new BranchContainer(scene, this);

    if (parentBranch() != rootItem) {
        // For floating branches get a position hint
        parentBranch()->addToBranchesContainer(branchContainer);
        BranchContainer *pbc = branchContainer->parentBranchContainer();
        if (pbc->hasFloatingBranchesLayout())
            branchContainer->setPos(pbc->getPositionHintNewChild(branchContainer));

        // Link to parent branch visually by
        // adding my upLink to parents linkContainer
        branchContainer->linkTo(parentBranch()->getBranchContainer());
    }

    return branchContainer;
}

BranchContainer* BranchItem::getBranchContainer()
{
    return branchContainer;
}

void BranchItem::unlinkBranchContainer()
{
    //qDebug() << "BI::unlinkBC in " << this << headingPlain();

    // Called from destructor of containers to 
    // avoid double deletion 
    branchContainer = nullptr;
}

Container* BranchItem::getBranchesContainer()
{
    return branchContainer->getBranchesContainer();
}

Container* BranchItem::getImagesContainer()
{
    return branchContainer->getImagesContainer();
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

    if (parentBranch() == rootItem) {
        // I am a MapCenter
        branchContainer->setPos(sp);
        return;
    }

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
    // Keep scene position while relinking image container
    branchContainer->addToImagesContainer(ic);
}

void BranchItem::repositionContainers()
{
    branchContainer->reposition();
}
