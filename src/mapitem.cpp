#include "mapitem.h"

#include "branchitem.h"
#include "image-container.h"
#include "imageitem.h"

#include <QDebug>

MapItem::MapItem(TreeItem *parent)
    : TreeItem(parent)
{
    // qDebug() << "Constr. MapItem(" << parent << ")";
    init();
}

void MapItem::init()
{
    hideLinkUnselected = false;
}

Container* MapItem::getContainer()
{
    if (hasTypeBranch())
        return ((BranchItem*)this)->getBranchContainer();
    else if (hasTypeImage())
        return ((ImageItem*)this)->getImageContainer();

    return nullptr;
}

void MapItem::setPos(const QPointF &p)
{
    if (hasTypeBranch()) {
        ((BranchItem*)this)->getBranchContainer()->setPos(p);
        return;
    }

    if (hasTypeImage())
        ((ImageItem*)this)->getImageContainer()->setPos(p);
}

void MapItem::setHideLinkUnselected(bool b) // FIXME-2 not working yet with containers
{
    hideLinkUnselected = b;
}

bool MapItem::getHideLinkUnselected() { return hideLinkUnselected; }

QString MapItem::getPosAttr()
{
    QString s;
    QPointF pos = getContainer()->pos();

    s = attribute("posX", QString().setNum(pos.x())) +
        attribute("posY", QString().setNum(pos.y()));
    return s;
}

QString MapItem::getLinkableAttr()
{
    QString s;

    if (hideLinkUnselected)
        s += attribute("hideLink", "true");

    return s;
}
