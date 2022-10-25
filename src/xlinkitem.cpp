#include "xlinkitem.h"
#include <QGraphicsScene>

#include "branchitem.h"
#include "vymmodel.h"
#include "xlinkobj.h"

/////////////////////////////////////////////////////////////////
// XLinkItem
/////////////////////////////////////////////////////////////////

XLinkItem::XLinkItem(TreeItem *parent)
    : MapItem(parent)

{
    // qDebug() << "Const XLinkItem () "<<this;
    init();
}

XLinkItem::~XLinkItem()
{
    //   qDebug() << "Destr XLinkItem begin "<<this<<"  pI="<<parentItem<<"
    //   link="<<link;
    if (link) {
        XLinkItem *xli = link->getOtherEnd(this);
        if (xli)
            model->deleteLater(xli->getID());
        model->deleteLink(link);
    }
    //    qDebug() << "Destr XLinkItem end";
}

void XLinkItem::init()
{
    setType(XLink);
    link = nullptr;
}

void XLinkItem::clear() {}

void XLinkItem::setLink(Link *l) { link = l; }

Link *XLinkItem::getLink() { return link; }

void XLinkItem::updateXLink()
{
    if (link)
        link->updateLink();
}

XLinkObj *XLinkItem::getXLinkObj()
{
    if (link)
        return link->getXLinkObj();
    return nullptr;
}

void XLinkItem::setSelection()
{
    if (link) {
        XLinkObj *xlo = getXLinkObj();
        if (xlo) {
            if (parentItem == link->getBeginBranch())
                xlo->setSelection(XLinkObj::C0);
            else if (parentItem == link->getEndBranch())
                xlo->setSelection(XLinkObj::C1);
        }
    }
}

BranchItem *XLinkItem::getPartnerBranch()
{
    if (link && link->getBeginBranch() && link->getEndBranch()) {
        if (parentItem == link->getBeginBranch())
            return link->getEndBranch();
        else
            return link->getBeginBranch();
    }
    return NULL;
}
