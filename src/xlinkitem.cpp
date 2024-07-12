#include "xlinkitem.h"
#include <QGraphicsScene>

#include "branchitem.h"
#include "vymmodel.h"
#include "xlinkobj.h"
#include "xlink-wrapper.h"

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

    if (xlinkWrapperInt) {
        delete xlinkWrapperInt;
        xlinkWrapperInt = nullptr;
    }
    //    qDebug() << "Destr XLinkItem end";
}

void XLinkItem::init()
{
    setType(XLink);
    link = nullptr;
    itemData[0] = "XLink";
    xlinkWrapperInt = nullptr;
}

void XLinkItem::setLink(Link *l) { link = l; }

Link *XLinkItem::getLink() { return link; }

/*
XLinkWrapper* XLinkItem::xlinkWrapper()
{
    if (!xlinkWrapperInt)
        xlinkWrapperInt = new XLinkWrapper(this);

    return xlinkWrapperInt;
}
*/

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

QColor XLinkItem::headingColor()
{
    // Used in TreeModel::data() to get colors
    return link->getPen().color();
}

void XLinkItem::setSelectionType()
{
    if (link) {
        XLinkObj *xlo = getXLinkObj();
        if (xlo) {
            if (parentItem == link->getBeginBranch())
                xlo->setSelectionType(XLinkObj::C0);
            else if (parentItem == link->getEndBranch())
                xlo->setSelectionType(XLinkObj::C1);
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
    return nullptr;
}
