#include "mapitem.h"

#include "branchitem.h"
#include "linkablemapobj.h"
#include "ornamentedobj.h"

#include <QDebug>

extern FlagRowMaster *systemFlagsMaster;

MapItem::MapItem(TreeItem *parent)
    : TreeItem(parent)
{
    // qDebug() << "Constr. MapItem(" << parent << ")";
    init();
}

void MapItem::init()
{
    mo = nullptr;
    hideLinkUnselected = false;
}

void MapItem::appendChild(TreeItem *item)
{
    TreeItem::appendChild(item);

    // FIXME-4 maybe access parent in MapObjs directly via treeItem
    // and remove this here...

    // If lmo exists, also set parObj there
    LinkableMapObj *lmo = getLMO();
    if (lmo) {
        LinkableMapObj *itemLMO = ((MapItem *)item)->getLMO();
        if (itemLMO)
            itemLMO->setParObj(lmo);
    }
}

void MapItem::setRelPos(const QPointF &p)   // FIXME-2 add image containers
{
    if (isBranchLikeType()) 
        ((BranchItem*)this)->getBranchContainer()->setPos(p);
}

void MapItem::setAbsPos(const QPointF &p)// FIXME-2 add image containers
{
    if (isBranchLikeType()) {
        BranchContainer *bc = ((BranchItem*)this)->getBranchContainer();
        bc->setPos(bc->sceneTransform().inverted().map(p));
    }
}

void MapItem::setHideLinkUnselected(bool b) // FIXME-2 not working yet with containers
{
    hideLinkUnselected = b;
    LinkableMapObj *lmo = getLMO();
    if (lmo) {
        // lmo->setHideLinkUnselected();
        lmo->setVisibility(lmo->isVisibleObj());
        lmo->updateLinkGeometry();
    }
}

bool MapItem::getHideLinkUnselected() { return hideLinkUnselected; }

QString MapItem::getMapAttr()   // FIXME-2 Refactor to use container layouts
{
    QString s;
    /*
    LinkableMapObj *lmo = getLMO();

    if (parentItem == rootItem)
        posMode = Absolute;
    else {
        if (type == TreeItem::Image || depth() == 1 || lmo->getUseRelPos())
            posMode = Relative; // FIXME-2 shouldn't this be replaced by relPos?
        else
            posMode = Unused;
    }
    switch (posMode) {
    case Relative:
        if (lmo)
            pos = lmo->getRelPos();
        s = attribut("relPosX", QString().setNum(pos.x())) +
            attribut("relPosY", QString().setNum(pos.y()));
        break;
    case Absolute:
        if (mo)
            pos = mo->getAbsPos();
        s = attribut("absPosX", QString().setNum(pos.x())) +
            attribut("absPosY", QString().setNum(pos.y()));
        break;
    default:
        break;
    }
    if (hideLinkUnselected)
        s += attribut("hideLink", "true");

    // Rotation angle
    MapObj *mo = getMO();
    if (mo)
        angle = mo->getRotation();
    if (angle != 0)
        s += attribut("rotation", QString().setNum(angle));

        */
    return s;
}

QRectF MapItem::getBBoxURLFlag()
{
    QString s = "system-url";
    QStringList list = systemFlags.activeFlagNames().filter(s);
    if (list.count() > 1) {
        qWarning() << "MapItem::getBBoxURLFlag found more than one system-url*";
        return QRectF();
    }

    Flag *f = systemFlagsMaster->findFlagByName(s);
    if (f) {
        QUuid u = f->getUuid();
        LinkableMapObj *lmo = getLMO();
        if (lmo)
            return ((OrnamentedObj *)lmo)->getBBoxSystemFlagByUid(u);
    }
    return QRectF();
}

void MapItem::setRotation(const qreal &a)
{
    angle = a;
    MapObj *mo = getMO();
    if (mo)
        mo->setRotation(a);
}

MapObj *MapItem::getMO() { return mo; }   // FIXME-0 remove completely

LinkableMapObj *MapItem::getLMO()   // FIXME-0 remove completely
{
    if (isBranchLikeType() || type == Image)
        return (LinkableMapObj *)mo;
    else
        return NULL;
}

QPainterPath MapItem::getSelectionPath() // FIXME-3 should be in BranchContainer or ImageContainer
{
    QPainterPath p;
    QRectF r;
    qreal d = 3; // Thickness of selection "border"

    if (isBranchLikeType() )    // FIXME-2 Image type still missing!
    {
        r = ((BranchItem*)this)->getBranchContainer()->getHeadingRect();
        p.moveTo(r.topLeft() + QPointF(-d, -d));
        p.lineTo(r.topRight() + QPointF(d, -d));
        p.lineTo(r.bottomRight() + QPointF(d, d));
        p.lineTo(r.bottomLeft() + QPointF(-d, d));
        p.lineTo(r.topLeft() + QPointF(-d, -d));
    } else
        qWarning() << "MapITem::getSelectionPath - unknown item type!";

    return p;
}

QPointF MapItem::getEditPosition() // FIXME-3 should be directly in BranchContainer
{
    QPointF p;

    if (isBranchLikeType() )
        p = ((BranchItem*)this)->getBranchContainer()->scenePos();
    else
        qWarning() << "MapITem::getEditPosition - unknown item type!";

    return p;
}

