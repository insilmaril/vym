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
    // qDebug() << "Const XLinkItem () " << this;
    init();
}

XLinkItem::~XLinkItem()
{
    // qDebug() << "Destr XLinkItem begin  "<< this << "  pI=" << parentItem << " xlink=" << xlinkInt;
    if (xlinkInt)
        xlinkInt = nullptr;
}

void XLinkItem::init()
{
    setType(XLink);
    xlinkInt = nullptr;
    itemData[0] = "XLink";
}

void XLinkItem::setLink(Link *l) { xlinkInt = l; }

Link *XLinkItem::getLink() { return xlinkInt; }

void XLinkItem::updateXLink()
{
    if (xlinkInt)
        xlinkInt->updateLink();
}

XLinkObj *XLinkItem::getXLinkObj()
{
    if (xlinkInt)
        return xlinkInt->getXLinkObj();
    return nullptr;
}

QColor XLinkItem::headingColor()
{
    // Used in TreeModel::data() to get colors
    return xlinkInt->getPen().color();
}

void XLinkItem::setSelectionType()
{
    if (xlinkInt) {
        XLinkObj *xlo = getXLinkObj();
        if (xlo) {
            if (parentItem == xlinkInt->getBeginBranch())
                xlo->setSelectionType(XLinkObj::C0);
            else if (parentItem == xlinkInt->getEndBranch())
                xlo->setSelectionType(XLinkObj::C1);
        }
    }
}

BranchItem *XLinkItem::getPartnerBranch()
{
    if (xlinkInt && xlinkInt->getBeginBranch() && xlinkInt->getEndBranch()) {
        if (parentItem == xlinkInt->getBeginBranch())
            return xlinkInt->getEndBranch();
        else
            return xlinkInt->getBeginBranch();
    }
    return nullptr;
}
