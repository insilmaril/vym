#include <QGraphicsScene>
#include "xlinkitem.h"

#include "branchitem.h"
#include "linkablemapobj.h"
#include "vymmodel.h"
#include "xlinkobj.h"

/////////////////////////////////////////////////////////////////
// XLinkItem
/////////////////////////////////////////////////////////////////

XLinkItem::XLinkItem (const QList<QVariant> &data, TreeItem *parent):MapItem (data,parent)

{
    //qDebug() << "Const XLinkItem () "<<this;
    init();
}

XLinkItem::~XLinkItem ()
{
 //   qDebug() << "Destr XLinkItem begin "<<this<<"  pI="<<parentItem<<"  link="<<link;
    if (link)
    {
	XLinkItem *xli=link->getOtherEnd (this);
	if (xli) model->deleteLater (xli->getID());
	model->deleteLink (link);
    }	
//    qDebug() << "Destr XLinkItem end";
}


void XLinkItem::init () 
{
    setType (XLink);
    link=NULL;
}

void XLinkItem::setLink (Link *l)
{
    link=l;
}

Link* XLinkItem::getLink ()
{
    return link;
}

void XLinkItem::updateXLink()
{
    qDebug()<<"XLI::updateXLink";
    if (link)
	link->updateLink();
}

BranchItem* XLinkItem::getPartnerBranch()
{
    if (link && link->getBeginBranch() && link->getEndBranch())
    {
	if (parentItem==link->getBeginBranch())
	    return link->getEndBranch();
	else	
	    return link->getBeginBranch();
    }
    return NULL;
}

