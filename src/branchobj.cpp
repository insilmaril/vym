#include <QDebug>


#include "attributeitem.h"
#include "branchitem.h"
#include "geometry.h"
#include "mainwindow.h"
#include "mapeditor.h"
#include "misc.h"

extern FlagRowMaster *standardFlagsMaster;
extern FlagRowMaster *userFlagsMaster;
extern FlagRowMaster *systemFlagsMaster;
extern bool debug;

/////////////////////////////////////////////////////////////////
// BranchObj
/////////////////////////////////////////////////////////////////

BranchObj::BranchObj(QGraphicsItem *parent, TreeItem *ti)
    : OrnamentedObj(parent, ti)
{
    //qDebug() << "Const BranchObj  (s,ti) this = " << this << " ti=" << ti << ti->depth();

    treeItem = ti;
    BranchItem *pi = (BranchItem *)(ti->parent());
    if (pi && pi != ti->getModel()->getRootItem())
        parObj = pi->getLMO();
    else
        parObj = NULL;
    init();
}

BranchObj::~BranchObj()
{
    //qDebug()<< "Destr BranchObj  of " << this;

    // If I'm animated, I need to un-animate myself first
    if (anim.isAnimated()) {
        anim.setAnimated(false);
        VymModel *model = treeItem->getModel();
        // FIXME-2 moved to MapEditor! model->stopAnimation(this);
    }
}

void BranchObj::init()
{
    if (parObj)
        absPos = parObj->getChildRefPos();
}

void BranchObj::copy(BranchObj *other)
{
    OrnamentedObj::copy(other);

    setVisibility(other->visible);

    positionBBox();
}

void BranchObj::setParObjTmp(LinkableMapObj *dst, QPointF m, int off)
{
    // Temporary link to dst
    // m is position of mouse pointer
    // offset 0: default 1: below dst   -1 above dst  (if possible)

    BranchItem *dsti = (BranchItem *)(dst->getTreeItem());

    BranchItem *pi = (BranchItem *)(dsti->parent());
    int pi_depth = pi->depth();
    BranchObj *bodst = (BranchObj *)dst;

    if (!tmpParent) {
        tmpParent = true;
        parObjTmpBuf = parObj;
    }

    if (pi_depth < 1)
        off = 0;
    if (off == 0)
        link2ParPos = false;
    else
        link2ParPos = true;
    parObj = bodst;

    setLinkStyle(dst->getDefLinkStyle(dsti));

    // Move temporary to new position at destination
    // Usually the positioning would be done by reposition(),
    // but then also the destination branch would "Jump" around...
    // Better just do it approximately
    if (dsti->depth() == 0) { // new parent is a mapcenter
        Vector v = (m - bodst->getChildRefPos());
        v.normalize();
        v.scale(150);
        move2RelPos(v.toQPointF());
    }
    else {
        qreal y;
        if (off == 0) {
            // Below is needed e.g. in a freshly loaded map,
            // bboxTotal seems not to be correct yet
            // relinking positions too far below then
            calcBBoxSizeWithChildren();

            // new parent is just a branch, link to it
            bodst->calcBBoxSizeWithChildren();
            QRectF t = bodst->getTotalBBox();
            if (dsti->getLastBranch())
                // Move below children of destination
                y = t.y() + t.height();
            else
                // Move left or right to destination
                y = t.y();
        }
        else {
            if (off < 0)
                // we want to link above dst
                y = bodst->y() - height() + 12;
            else
                // we want to link below dst
                // Bottom of sel should be 5 pixels above
                // the bottom of the branch _below_ the target:
                // Don't try to find that branch, guess 12 pixels
                y = bodst->getChildRefPos().y() - height() + 12;
        }
        if (bodst->getOrientation() == LinkableMapObj::LeftOfCenter)
            move(bodst->getChildRefPos().x() - linkwidth - bboxTotal.width(),
                 y);
        else
            move(bodst->getChildRefPos().x() + linkwidth, y);
    }

    // updateLinkGeometry is called implicitly in move
    requestReposition();
}

void BranchObj::unsetParObjTmp()
{
    if (tmpParent) {
        tmpParent = false;
        link2ParPos = false;
        parObj = parObjTmpBuf;
        parObjTmpBuf = NULL;
        setLinkStyle(getDefLinkStyle(treeItem->parent()));
        updateLinkGeometry();
    }
}

void BranchObj::setVisibility(bool v, int toDepth)
{
    /*
    BranchItem *bi = (BranchItem *)treeItem;
    if (bi->depth() <= toDepth) {
        frame->setVisibility(v);
        //heading->setVisibility(v);
        systemFlagRowObj->setVisibility(v);
        standardFlagRowObj->setVisibility(v);
        LinkableMapObj::setVisibility(v);
        int i;
        for (i = 0; i < treeItem->imageCount(); ++i)
            treeItem->getImageObjNum(i)->setVisibility(v);
        for (i = 0; i < treeItem->xlinkCount(); ++i)
            treeItem->getXLinkObjNum(i)->setVisibility();

        // Only change children, if I am not scrolled
        if (!bi->isScrolled() && (bi->depth() < toDepth)) {
            // Now go recursivly through all children
            for (i = 0; i < treeItem->branchCount(); ++i)
                treeItem->getBranchObjNum(i)->setVisibility(v, toDepth);
        }
    }
    */
}

void BranchObj::setVisibility(bool v) { setVisibility(v, MAX_DEPTH); }

void BranchObj::positionContents()
{
    /*
    OrnamentedObj::positionContents();
    updateLinkGeometry(); // required before positioning images
    for (int i = 0; i < treeItem->imageCount(); ++i)
        treeItem->getImageObjNum(i)->reposition();
    */
}

void BranchObj::move(double x, double y) { OrnamentedObj::move(x, y); }

void BranchObj::move(QPointF p) { move(p.x(), p.y()); }

void BranchObj::moveBy(double x, double y)
{
    /*
    OrnamentedObj::moveBy(x, y);
    for (int i = 0; i < treeItem->branchCount(); ++i)
        treeItem->getBranchObjNum(i)->moveBy(x, y);
    positionBBox();
    */
}

void BranchObj::moveBy(QPointF p) { moveBy(p.x(), p.y()); }

void BranchObj::positionBBox() // FIXME-3 consider dimensions of frame
                               // (thickness, geometry, padding...
{
    /*
    QPointF ap = getAbsPos();
    bbox.moveTopLeft(ap);
    positionContents(); // this positions FIOs

    // Update links to other branches
    XLinkObj *xlo;
    for (int i = 0; i < treeItem->xlinkCount(); ++i) {
        xlo = treeItem->getXLinkObjNum(i);
        if (xlo)
            xlo->updateXLink();
    }
    */
}

void BranchObj::calcBBoxSize()
{
    /*
    QSizeF heading_r; // = heading->getSize();
    qreal heading_w = (qreal)heading_r.width();
    qreal heading_h = (qreal)heading_r.height();
    QSizeF sysflags_r = systemFlagRowObj->getSize();
    qreal sysflags_h = sysflags_r.height();
    qreal sysflags_w = sysflags_r.width();
    QSizeF stanflags_r = standardFlagRowObj->getSize();
    qreal stanflags_h = stanflags_r.height();
    qreal stanflags_w = stanflags_r.width();
    qreal w;
    qreal h;

    // set width to sum of all widths
    w = heading_w + sysflags_w + stanflags_w;

    // set height to maximum needed height
    h = max(sysflags_h, stanflags_h);
    h = max(h, heading_h);

    // Save the dimension of flags and heading
    ornamentsBBox.setSize(QSizeF(w, h));

    // clickBox includes Flags and Heading
    clickPoly = QPolygonF(ornamentsBBox);

    // Floatimages
    QPointF rp;

    topPad = botPad = leftPad = rightPad = 0;
    bool incV = false; //((BranchItem *)treeItem)->getIncludeImagesVer();
    bool incH = false; //((BranchItem *)treeItem)->getIncludeImagesHor();
    if (incH || incV) {
        FloatImageObj *fio;
        for (int i = 0; i < treeItem->imageCount(); ++i) {
            fio = treeItem->getImageObjNum(i);
            rp = fio->getRelPos();
            if (incV) {
                qreal y;
                if (rp.y() > 0) {
                    y = rp.y() + fio->height() / 2 - ornamentsBBox.height() / 2;
                    botPad = max(botPad, y);
                }
                else {
                    y = -rp.y() + fio->height() / 2 -
                        ornamentsBBox.height() / 2;
                    topPad = max(topPad, y);
                }
            }
            if (incH) {
                qreal x;
                if (rp.x() > 0) {
                    x = rp.x() + fio->width() / 2 - ornamentsBBox.width() / 2;
                    rightPad = max(rightPad, x);
                }
                else {
                    x = -rp.x() + fio->width() / 2 - ornamentsBBox.width() / 2;
                    leftPad = max(leftPad, x);
                }
            }
        }
        h += topPad + botPad;
        w += leftPad + rightPad;
    }

    // Frame thickness
    w += frame->getTotalPadding() * 2;
    h += frame->getTotalPadding() * 2;

    // Finally set size
    bbox.setSize(QSizeF(w, h));
    // if (debug) qDebug()<<"BO: calcBBox "<<treeItem->getHeading()<<"
    // bbox="<<bbox;
    */
}

void BranchObj::setDockPos()
{
    /*
    floatRefPos = ornamentsBBox.center();

    if (treeItem->getType() == TreeItem::MapCenter) {
        // set childRefPos to middle of MapCenterObj
        QRectF r = clickPoly.boundingRect();
        childRefPos.setX(r.topLeft().x() + r.width() / 2);
        childRefPos.setY(r.topLeft().y() + r.height() / 2);
        parPos = childRefPos;
        for (int i = 0; i < treeItem->branchCount(); ++i)
            treeItem->getBranchObjNum(i)->updateLinkGeometry();
    }
    else {
        if (orientation == LinkableMapObj::LeftOfCenter) {
            // Left of center
            if (((BranchItem *)treeItem)->getFrameIncludeChildren()) {
                childRefPos = QPointF(ornamentsBBox.bottomLeft().x() - leftPad,
                                      bottomlineY);
                parPos = QPointF(bboxTotal.bottomRight().x() -
                                     frame->getPadding() / 2,
                                 bottomlineY);
            }
            else {
                childRefPos = QPointF(ornamentsBBox.bottomLeft().x() -
                                          frame->getPadding(),
                                      bottomlineY);
                parPos = QPointF(ornamentsBBox.bottomRight().x(), bottomlineY);
            }
        }
        else {
            // Right of center
            if (((BranchItem *)treeItem)->getFrameIncludeChildren()) {
                childRefPos = QPointF(
                    ornamentsBBox.bottomRight().x() + rightPad, bottomlineY);
                parPos = QPointF(bboxTotal.bottomLeft().x() +
                                     frame->getPadding() / 2,
                                 bottomlineY);
            }
            else {
                childRefPos = QPointF(ornamentsBBox.bottomRight().x() +
                                          frame->getPadding(),
                                      bottomlineY);
                parPos = QPointF(ornamentsBBox.bottomLeft().x(), bottomlineY);
            }
        }
    }
    */
}

void BranchObj::updateVisuals()
{
    if (!treeItem) {
        qWarning("BranchObj::udpateHeading treeItem==NULL");
        return;
    }

    // Update heading
    QString s = treeItem->getHeadingText();
    /*
    if (s != heading->text())
        heading->setText(s);
    */

    // Update standard flags active in TreeItemAbsage

    QList<QUuid> TIactiveFlagUids = treeItem->activeFlagUids();
    standardFlagRowObj->updateActiveFlagObjs(
        TIactiveFlagUids, standardFlagsMaster, userFlagsMaster);

    // Add missing system flags active in TreeItem
    TIactiveFlagUids = treeItem->activeSystemFlagUids();
    systemFlagRowObj->updateActiveFlagObjs(TIactiveFlagUids, systemFlagsMaster);

    calcBBoxSize();
}

void BranchObj::setDefAttr(BranchModification mod, bool keepFrame)
{
    /*
    QFont font = treeItem->getModel()->getMapDefaultFont();
    qreal fontsize = font.pointSizeF();
    switch (treeItem->depth()) {
    case 0:
        break;
    case 1:
        fontsize = fontsize - 2;
        break;
    case 2:
        fontsize = fontsize - 4;
        break;
    default:
        fontsize = fontsize - 6;
        break;
    }
    setLinkStyle(getDefLinkStyle(treeItem->parent()));
    setLinkColor();
    font.setPointSizeF(fontsize);
    //heading->setFont(font);

    if (mod == NewBranch && !keepFrame) {
        if (treeItem->depth() == 0)
            setFrameType(FrameObj::Rectangle);
        else
            setFrameType(FrameObj::NoFrame);
    }
    if (mod == NewBranch)
        setColor(treeItem->getHeadingColor());
    else {
        // Relinked mapcenters
        if (!keepFrame && getFrameType() != FrameObj::NoFrame)
            setFrameType(FrameObj::NoFrame);

        // Also set styles for children
        for (int i = 0; i < treeItem->branchCount(); ++i)
            treeItem->getBranchObjNum(i)->setDefAttr(MovedBranch, keepFrame);
    }
    calcBBoxSize();
    */
}

void BranchObj::alignRelativeTo(QPointF ref, bool alignSelf)
{
    /*
    // Define some heights
    qreal th = bboxTotal.height();
    qreal ch = 0; // Sum of childrens heights
    for (int i = 0; i < treeItem->branchCount(); ++i)
        ch += treeItem->getBranchObjNum(i)->getTotalBBox().height();

    int depth = 0;
    BranchItem::LayoutHint layoutHint = BranchItem::AutoPositioning;
    if (parObj) {
        TreeItem *pi = parObj->getTreeItem();
        depth = 1 + pi->depth();
        //layoutHint = ((BranchItem *)treeItem)->parentBranch()->getChildrenLayout();
    }

    // set useRelPos, depending on layout
    if (depth > 1) {
        if (layoutHint == BranchItem::FreePositioning) {
            if (!useRelPos) {
                useRelPos = true;
                // if we used relPos before, set known positions
                // "known" means any position != (0,0)
                if (relPos == QPointF(0, 0))
                    // use current position to get relPos()
                    setRelPos();
            }
        }
        else
            useRelPos = false;
    }
    */

    // TODO testing
    /*
        if (debug)
        {
            QString o;
            switch (orientation)
            {
                case UndefinedOrientation: o = "UndefOrientation"; break;
                case LeftOfCenter: o = "LeftOfCenter"; break;
                case RightOfCenter: o = "RightOfCenter"; break;
            }

            QString h=QString (depth+1,' ');
            h += treeItem->getHeadingPlain();
            h += QString (25,' ');
            h.truncate (25);
            QPointF pp;
            if (parObj) pp = parObj->getChildRefPos();
            qDebug() << "BO::alignRelTo for "<<h
        //    qDebug() << "    d="<<depth;
        //    qDebug() <<"   ref="<<ref;
        //    qDebug() <<"    th="<<th;
        //    qDebug() <<"    ch="<<ch;
        //    if (ch < th) qDebug()<<"   ch<th !";
        //    qDebug() <<"  parO="<<parObj;
            //qDebug() <<   "  bbox.tL="<<bboxTotal.topLeft();
            << "  useRelPos=" << useRelPos
            << " layoutHint= " << layoutHint
        //    qDebug() <<"absPos="<<absPos
            << "  relPos="<<relPos
        //	<< "  parPos="<<pp
        //	<< "  bbox="<<bbox
            << "  orient="<<o<<" "<<orientation;
        //	<< "  alignSelf="<<alignSelf
        //	<< "  scrolled="<<((BranchItem*)treeItem)->isScrolled()
        //	<< "  pad="<<topPad<<","<<botPad<<","<<leftPad<<","<<rightPad
        //	<< "  hidden="<<hidden
        //	<< "  th="<<th
            ;
        }
       */

    setOrientation();

    /*
    // Align myself
    if (depth == 0)
        move(getAbsPos()); // Trigger update of frames etc.
    else if (depth == 1)
        move2RelPos(getRelPos());
    else if (depth > 1) {
        if (layoutHint == BranchItem::FreePositioning)
            move2RelPos(getRelPos());
        else {
            if (anim.isAnimated())
                move2RelPos(anim);
            else {
                if (alignSelf)
                    switch (orientation) {
                    case LinkableMapObj::LeftOfCenter:
                        move(ref.x() - bbox.width(),
                             ref.y() + (th - bbox.height()) / 2);
                        break;
                    case LinkableMapObj::RightOfCenter:
                        move(ref.x(), ref.y() + (th - bbox.height()) / 2);
                        break;
                    default:
                        qWarning("LMO::alignRelativeTo: oops, no orientation "
                                 "given for BO...");
                        break;
                    }
            }
        }
    }

    // Without ancestors I am done
    if (((BranchItem *)treeItem)->isScrolled())
        return;

    // Set reference point for alignment of children
    QPointF ref2;
    if (orientation == LinkableMapObj::LeftOfCenter)
        ref2.setX(childRefPos.x() - linkwidth);
    else
        ref2.setX(childRefPos.x() + linkwidth);

    if (depth == 1)
        ref2.setY(absPos.y() + (bbox.height() - ch) / 2);
    else {
        if (ch > th)
            ref2.setY(ref.y() + frame->getPadding());
        else
            // Parent is bigger than all of childs, center childs vertically
            ref2.setY(ref.y() + (th - ch) / 2);
    }

    // Align the branch children depending on reference point
    for (int i = 0; i < treeItem->branchCount(); ++i) {
        if (!treeItem->getBranchNum(i)->isHidden()) {
            treeItem->getBranchObjNum(i)->alignRelativeTo(ref2, true);

            // append next branch below current one
            ref2.setY(ref2.y() +
                      treeItem->getBranchObjNum(i)->getTotalBBox().height());
        }
    }
    */
}

void BranchObj::reposition()
{
    /* TODO testing only
        if (debug)
        {
            if (!treeItem->getHeading().isEmpty())
                qDebug()<< "  BO::reposition  a) d="<<treeItem->depth()<<"
       "<<treeItem->getHeading(); else qDebug()<< "  BO::reposition  a)
       d="<<treeItem->depth()<<" ???";
        }
    */

    if (treeItem->depth() == 0)
        // only calculate the sizes once. If the deepest LMO
        // changes its height,
        // all upper LMOs have to change, too.
        calcBBoxSizeWithChildren();

    alignRelativeTo(QPointF(
        absPos.x(), absPos.y() - (bboxTotal.height() - bbox.height()) / 2));
}

void BranchObj::unsetAllRepositionRequests()
{
    /*
    repositionRequest = false;
    for (int i = 0; i < treeItem->branchCount(); ++i)
    treeItem->getBranchObjNum(i)->unsetAllRepositionRequests();
    */
}

QRectF BranchObj::getTotalBBox() { return bboxTotal; }

ConvexPolygon BranchObj::getBoundingPolygon()
{
    if (treeItem->branchCount() == 0 || treeItem->depth() == 0) {
        return MapObj::getBoundingPolygon();
    }

    QPolygonF p;
    p << bboxTotal.topLeft();
    p << bboxTotal.topRight();
    p << bboxTotal.bottomRight();
    p << bboxTotal.bottomLeft();
    return p;
}

void BranchObj::calcBBoxSizeWithChildren()
{
    /*
    // if branch is scrolled, ignore children, but still consider floatimages
    BranchItem *bi = (BranchItem *)treeItem;
    if (bi->isScrolled()) {
        bboxTotal.setWidth(bbox.width());
        bboxTotal.setHeight(bbox.height());
        return;
    }

    if (bi->isHidden()) {
        bboxTotal.setWidth(0);
        bboxTotal.setHeight(0);
        return;
    }

    QRectF r(0, 0, 0, 0);
    QRectF br;

    // Now calculate
    // sum of heights
    // maximum of widths
    // minimum of y
    for (int i = 0; i < treeItem->branchCount(); i++) {
        if (!bi->getBranchNum(i)->isHidden()) {
            BranchObj *bo = bi->getBranchObjNum(i);
            bo->calcBBoxSizeWithChildren();
            br = bo->getTotalBBox();
            r.setWidth(max(br.width(), r.width()));
            r.setHeight(br.height() + r.height());
        }
    }

    // Add myself and also
    // add width of link to sum if necessary
    if (bi->branchCount() < 1)
        bboxTotal.setWidth(bbox.width() + r.width());
    else
        bboxTotal.setWidth(bbox.width() + r.width() + linkwidth);

    // bbox already contains frame->padding()*2
    bboxTotal.setHeight(
        max(r.height() + frame->getPadding() * 2, bbox.height()));
        */
}

void BranchObj::setAnimation(const AnimPoint &ap) { anim = ap; }

void BranchObj::stopAnimation()
{
    anim.stop();
    if (useRelPos)
        setRelPos(anim);
    else
        move(anim);
}

bool BranchObj::animate()
{
    anim.animate();
    if (anim.isAnimated()) {
        if (useRelPos)
            setRelPos(anim);
        else
            move(anim);
        return true;
    }
    return false;
}

