#include "xlinkitem.h"
#include <QGraphicsScene>

#include "branchitem.h"
#include "vymmodel.h"
#include "xlink.h"
#include "xlinkobj.h"
//#include "xlink-wrapper.h"

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
    setType(XLinkType);
    xlinkInt = nullptr;
    itemData[0] = "XLink";
}

void XLinkItem::setXLink(XLink *l) { xlinkInt = l; }

XLink *XLinkItem::getXLink() { return xlinkInt; }

void XLinkItem::updateXLink()
{
    if (xlinkInt)
        xlinkInt->updateXLink();
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
