#include "branchitem.h"

#include "attributeitem.h"
#include "branchobj.h"
#include "heading-container.h"
#include "image-container.h"
#include "task.h"
#include "taskmodel.h"
#include "vymmodel.h"
#include "xlink.h"
#include "xlinkitem.h"

extern TaskModel *taskModel;

//#include <QDir>

BranchItem::BranchItem(TreeItem *parent)
    : MapItem(parent)
{
    qDebug()<< "Constr. BranchItem this=" << this << "parent:" << parent;

    // Set type if parent is known yet
    // if not, type is set in insertBranch or TreeItem::appendChild
    if (parent == rootItem)
        setType(MapCenter);
    else
        setType(Branch);

    scrolled = false;
    tmpUnscrolled = false;

    includeImagesVer = false;
    includeImagesHor = false;
    includeChildren = false;
    childrenLayout = BranchItem::AutoPositioning;

    lastSelectedBranchNum = 0;
    lastSelectedBranchNumAlt = 0;

    task = nullptr;


    branchContainer = nullptr;
}

BranchItem::~BranchItem()
{
    qDebug() << "Destr. BranchItem: this=" << this << "  " << getHeadingPlain() << "branchContainer=" << branchContainer;
    if (mo) {   // FIXME-2 remove MapObj completely...
        delete mo;
        mo = NULL;
    }

    if (branchContainer) {
        // This deletes only the first container here.
        // All other containers deeper down in tree will unlink themselves 
        // by calling BranchItem::unlinkContainer, which will set 
        // the branchContainer == nullptr;
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

    QString s, a;

    // Update of note is usually done while unselecting a branch

    QString scrolledAttr;
    if (scrolled)
        scrolledAttr = attribut("scrolled", "yes");
    else
        scrolledAttr = "";

    // save area, if not scrolled   // not needed if HTML is rewritten...
    // also we could check if _any_ of parents is scrolled
    QString areaAttr;   // FIXME-2 will not work with rotated containers
    if (branchContainer && parentItem->isBranchLikeType() &&
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

    // Free positioning of children         // FIXME-2 remove BI::FreePositioning
    QString layoutAttr;
    if (childrenLayout == BranchItem::FreePositioning)
        layoutAttr += attribut("childrenFreePos", "true");

    // Save rotation                        // FIXME-2 use rotation via container layouts
    /*
    QString rotAttr;
    if (mo && mo->getRotation() != 0)
        rotAttr = attribut("rotation", QString().setNum(mo->getRotation()));
    */

    // Save position
    QString posAttr;
    if (parentItem == rootItem)
        // Use absolute coordinates
        posAttr = attribut("absPosX", QString().setNum(branchContainer->scenePos().x())) +
                  attribut("absPosY", QString().setNum(branchContainer->scenePos().y()));
    else if ( branchContainer->isFloating())
        // Use relative coordinates
        posAttr = attribut("relPosX", QString().setNum(branchContainer->pos().x())) +
                  attribut("relPosY", QString().setNum(branchContainer->pos().y())); 

    s = beginElement(elementName + posAttr + getGeneralAttr() +
                     scrolledAttr + getIncludeImageAttr() + 
                     layoutAttr + idAttr);
    incIndent();

    // save heading
    s += heading.saveToDir();

    // save note
    if (!note.isEmpty())
        s += note.saveToDir();

    // Save frame  // not saved if there is no MO
    if (mo) {
        // Avoid saving NoFrame for objects other than MapCenter
        if (depth() == 0 || ((OrnamentedObj *)mo)->getFrame()->getFrameType() !=
                                FrameObj::NoFrame)
            s += ((OrnamentedObj *)mo)->getFrame()->saveToDir();
    }

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
    if (mo) {
        if (hasScrolledParent(this) || hidden)
            mo->setVisibility(false);
        else
            mo->setVisibility(true);
    }
}

void BranchItem::setHeadingColor(QColor color)
{
    TreeItem::setHeadingColor(color);
    branchContainer->getHeadingContainer()->setHeadingColor(color);
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

    BranchObj *bo;
    if (scrolled) {
        scrolled = false;
        systemFlags.deactivate(QString("system-scrolledright"));
        if (branchCounter > 0)
            for (int i = 0; i < branchCounter; ++i) {
                bo = (BranchObj *)(getBranchNum(i)->getMO());
                if (bo)
                    bo->setVisibility(true); // Recursively!
            }
    }
    else {
        scrolled = true;
        systemFlags.activate(QString("system-scrolledright"));
        if (branchCounter > 0)
            for (int i = 0; i < branchCounter; ++i) {
                bo = (BranchObj *)(getBranchNum(i)->getMO());
                if (bo)
                    bo->setVisibility(false); // Recursively!
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
    if (pi && pi->isBranchLikeType())
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
    if (pi && pi->isBranchLikeType())
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

void BranchItem::sortChildren(
    bool inverse) // FIXME-4 optimize by not using moveUp/Down
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
            }
            else if (prevChild->getHeadingPlain().compare(
                         curChild->getHeadingPlain(), Qt::CaseInsensitive) >
                     0) {
                model->moveUp(curChild);
                madeChanges = true;
            }
        }
    } while (madeChanges);
}

void BranchItem::setChildrenLayout(BranchItem::LayoutHint layoutHint)
{
    childrenLayout = layoutHint;
}

BranchItem::LayoutHint BranchItem::getChildrenLayout()
{
    return childrenLayout;
}

void BranchItem::setIncludeImagesVer(bool b) { includeImagesVer = b; }

bool BranchItem::getIncludeImagesVer() { return includeImagesVer; }

void BranchItem::setIncludeImagesHor(bool b) { includeImagesHor = b; }

bool BranchItem::getIncludeImagesHor() { return includeImagesHor; }

QString BranchItem::getIncludeImageAttr()
{
    QString a;
    if (includeImagesVer)
        a = attribut("incImgV", "true");
    if (includeImagesHor)
        a += attribut("incImgH", "true");
    return a;
}

BranchItem *BranchItem::getFramedParentBranch(BranchItem *start)    // FIXME-2 Used to determine background color in taskEditor
{
    /*
    BranchObj *bo = getBranchObj();
    if (bo && bo->getFrameType() != FrameObj::NoFrame) {
        if (bo->getFrame()->getFrameIncludeChildren())
            return this;
        if (this == start)
            return this;
    }
    BranchItem *bi = (BranchItem *)parentItem;
    if (bi && bi != rootItem)
        return bi->getFramedParentBranch(start);
    else
    */
        return nullptr;
}

void BranchItem::setFrameIncludeChildren(bool b) // FIXME-2 rework frames
{
    /*
    includeChildren = b; // FIXME-4 ugly: same information stored in FrameObj
    BranchObj *bo = getBranchObj();
    if (bo)
        bo->getFrame()->setFrameIncludeChildren(b);
        */
}

bool BranchItem::getFrameIncludeChildren() // FIXME-2 rework frames
{
    /*
    BranchObj *bo = getBranchObj();
    if (bo)
        return bo->getFrame()->getFrameIncludeChildren();
    else
        return includeChildren;
        */
    return false;
}

void BranchItem::setLastSelectedBranch() // FIXME-1 rework navigating the tree
{
    /*
    int d = depth();
    if (d >= 0) {
        if (d == 1)
            // Hack to save an additional lastSelected for mapcenters in
            // MapEditor depending on orientation this allows to go both left
            // and right from there
            if (mo && ((BranchObj *)mo)->getOrientation() ==
                          LinkableMapObj::LeftOfCenter) {
                ((BranchItem *)parentItem)->lastSelectedBranchNumAlt =
                    parentItem->num(this);
                return;
            }
        ((BranchItem *)parentItem)->lastSelectedBranchNum =
            parentItem->num(this);
    }
    */
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
        if (ti != NULL)
            return ti;
    }

    // Search images
    ImageItem *ii;
    ImageContainer *ic;
    for (int i = 0; i < imageCount(); ++i) {
        ii = getImageNum(i);
        ic = ii->getImageContainer();
        if (ic->mapToScene(ic->rect()).containsPoint(p, Qt::OddEvenFill)) return ii;
    }

    // Search my container     // FIXME-2   Check if container is visible!!
    if (branchContainer->isInClickBox(p) && !excludedItems.contains(this) ) //   &&
        //getBranchObj()->isVisibleObj())
        return this;

    return NULL;
}

void BranchItem::updateStyles(const bool &keepFrame)
{
    // Update styles when relinking branches  // FIXME-1 review, maybe done with layout in reposition automatically?
    /*
    if (mo) {
        BranchObj *bo = getBranchObj();
        if (parentItem != rootItem)
            bo->setParObj((LinkableMapObj *)(((MapItem *)parentItem)->getMO()));
        else
            bo->setParObj(NULL);
        bo->setDefAttr(BranchObj::MovedBranch, keepFrame);
    }
    */
}

void BranchItem::updateVisuals()
{
    branchContainer->updateVisuals();
}

BranchContainer *BranchItem::createBranchContainer(QGraphicsScene *scene)
{
    branchContainer = new BranchContainer(scene, nullptr, this);

    // Set visibility depending on parents  // FIXME-2
    /*
    if (parentItem != rootItem &&
        (((BranchItem *)parentItem)->scrolled ||
         !((MapItem *)parentItem)->getLMO()->isVisibleObj()))
        newbo->setVisibility(false);
    */
    if (depth() == 1) {  
        // Position new main branches on circle around center  // FIXME-2
        qreal r = 190;
        qreal a =
            -M_PI_4 + M_PI_2 * (num()) + (M_PI_4 / 2) * (num() / 4 % 4);
        QPointF p(r * cos(a), r * sin(a));
        // newbo->setRelPos(p);
    }

    // FIXME-2 for new branch set default font, color, link, frame, children styles
    // newbo->setDefAttr(BranchObj::NewBranch);

    if (!getHeading().isEmpty()) {  // FIXME-2 updateVisuals new container and color
        /*
        newbo->updateVisuals();
        newbo->setColor(heading.getColor());
        */
    }

    return branchContainer;
}

BranchContainer* BranchItem::getBranchContainer()
{
    return branchContainer;
}

void BranchItem::unlinkBranchContainer()
{
    // Called from destructor of containers to 
    // avoid double deletion 
    branchContainer = nullptr;
    qDebug() << "BI::unlinkBC in " << this << getHeadingPlain();
}

Container* BranchItem::getBranchesContainer() 
{
    return branchContainer->getBranchesContainer();
}

void BranchItem::updateContainerStackingOrder()
{
    int n = num();

    // It seems the QGraphicsItem::stackBefore only works, if an item is moved up. 
    // For moving below (or into another subtree), we have to reparent first  :-(

    // For simplicity we always reparent. The absolute position must not be changed here

    QPointF sp = branchContainer->scenePos();

    /* FIXME-2 debug only
    qDebug() << "      ## BI::updateContainerStackingOrder begin  bc.scenePos=" << branchContainer->scenePos() << 
        "pos=" << branchContainer->pos();
    qDebug() << "         - bc.parent: " << branchContainer->parentContainer()->info();
    qDebug() << "         - bc.p^2   : " << branchContainer->parentContainer()->parentContainer()->info();
    */

    branchContainer->setParentItem(nullptr);

    BranchItem *pi = parentBranch();

    if (pi && pi != rootItem) 
        branchContainer->setParentItem(parentBranch()->getBranchesContainer());
        // parentBranch()->addToChildrenContainer(branchContainer); // FIXME-2 maybe better? 
    else {
        qWarning() << "BI::updateStackingORder  pi = " << pi << "rootItem = " << rootItem << "(moved center?)"; // FIXME-2 testing
        return;
    }

    //branchContainer->setPos(branchContainer->sceneTransform().inverted().map(sp));
        
    if (n < parentBranch()->branchCount() - 1)
        branchContainer->stackBefore( (parentBranch()->getBranchNum(n + 1))->getBranchContainer() );

    /*
    qDebug() << "      ## BI::updateContainerStackingOrder end    bc.scenePos=" << branchContainer->scenePos() << 
        "pos=" << branchContainer->pos();
    qDebug() << "         - bc.parent: " << branchContainer->parentContainer()->info();
    qDebug() << "         - bc.p^2   : " << branchContainer->parentContainer()->parentContainer()->info();
    */
    return;
}

void BranchItem::addToBranchesContainer(BranchContainer *bc)
{
    branchContainer->addToBranchesContainer(bc);
}

void BranchItem::addToImagesContainer(ImageContainer *ic)
{
    qDebug() << "BI::add2ImgCont  this=" << this << "  ic=" << ic;
    branchContainer->addToImagesContainer(ic);
}

void BranchItem::repositionContainers()
{
    branchContainer->reposition();
}
