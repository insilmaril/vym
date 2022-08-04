#include "mapitem.h"

#include "branchitem.h"
#include "image-container.h"
#include "imageitem.h"
#include "linkablemapobj.h"

#include <QDebug>

extern FlagRowMaster *systemFlagsMaster;    // FIXME-2 used here?

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

void MapItem::appendChild(TreeItem *item) // FIXME-2 no longer used
{
    TreeItem::appendChild(item);

    // If lmo exists, also set parObj there // FIXME-2 remove
    /*
    LMbj *lmo = getLMO();
    if (lmo) {
        LMObj *itemLMO = ((MapItem *)item)->getLMO();
        if (itemLMO)
            itemLMO->setParObj(lmo);
    }
    */
}

Container* MapItem::getContainer()
{
    if (hasTypeBranch())
        return ((BranchItem*)this)->getBranchContainer();
    else if (hasTypeImage())
        return ((ImageItem*)this)->getImageContainer();

    return nullptr;
}

void MapItem::setPos(const QPointF &p)   // FIXME-2 add image containers
{
    if (hasTypeBranch())
        ((BranchItem*)this)->getBranchContainer()->setPos(p);

    if (hasTypeImage())
        ((ImageItem*)this)->getImageContainer()->setPos(p);
}

void MapItem::setHideLinkUnselected(bool b) // FIXME-2 not working yet with containers
{
    hideLinkUnselected = b;
    /*
    LMO *lmo = getLMO();
    if (lmo) {
        // lmo->setHideLinkUnselected();
        lmo->setVisibility(lmo->isVisibleObj());
        lmo->updateLinkGeometry();
    }
    */
}

bool MapItem::getHideLinkUnselected() { return hideLinkUnselected; }

QString MapItem::getPosAttr()
{
    QString s;
    QPointF pos = getContainer()->pos();

    s = attribut("posX", QString().setNum(pos.x())) +
        attribut("posY", QString().setNum(pos.y()));
    return s;
}

QString MapItem::getLinkableAttr()
{
    QString s;

    if (hideLinkUnselected)
        s += attribut("hideLink", "true");

    // Save rotation                        // FIXME-2 use rotation via container layouts
    /*
    QString rotAttr;
    if (mo && mo->getRotation() != 0)
        rotAttr = attribut("rotation", QString().setNum(mo->getRotation()));
    */
    
    // FIXME-2 Also save scale factor?  

    return s;
}

QRectF MapItem::getBBoxURLFlag()    // FIXME-2 not ported yet to containers
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
        /*
        LMO *lmo = getLMO();
        if (lmo)
            return ((OrnamentedObj *)lmo)->getBBoxSystemFlagByUid(u);
            */
    }
    return QRectF();
}

#include "heading-container.h"
QPainterPath MapItem::getSelectionPath() // FIXME-1 should be in BranchContainer or ImageContainer
{
    qreal d = 3;    // Margins around rectangle of item
    QPolygonF polygon;
    if (hasTypeBranch() )
    {
        HeadingContainer *hc = ((BranchItem*)this)->getBranchContainer()->getHeadingContainer();
        polygon = hc->mapToScene(hc->rect().marginsAdded(QMarginsF(d, d, d, d)));
    } else if (hasTypeImage()) {
        ImageContainer *ic =((ImageItem*)this)->getImageContainer();
        polygon = ic->mapToScene(ic->rect().marginsAdded(QMarginsF(d, d, d, d)));
    } else 
        qWarning() << "MapITem::getSelectionPath - unknown item type!";

    QPainterPath path;
    path.addPolygon(polygon);
    return path;
}

QPointF MapItem::getEditPosition() // FIXME-2 should be directly in BranchContainer
{
    QPointF p;

    if (hasTypeBranch() )
        p = ((BranchItem*)this)->getBranchContainer()->scenePos();
    else
        qWarning() << "MapITem::getEditPosition - unknown item type!";

    return p;
}

