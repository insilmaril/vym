#include "mapitem.h"

#include "branch-container.h"
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
    hideLinkUnselectedInt = false;
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

void MapItem::setHideLinkUnselected(bool b) // FIXME-2 upLink not available for images yet
{
    hideLinkUnselectedInt = b;
    if (hasTypeBranch()) {
        ((BranchItem*)this)->getBranchContainer()->updateVisibility();
        return;
    }
}

bool MapItem::hideLinkUnselected() { return hideLinkUnselectedInt; }

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

    if (hideLinkUnselectedInt)
        s += attribute("hideLink", "true");

    return s;
}
