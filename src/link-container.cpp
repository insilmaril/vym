#include "link-container.h"

#include "linkobj.h"

/////////////////////////////////////////////////////////////////
// LinkContainer
/////////////////////////////////////////////////////////////////

LinkContainer::LinkContainer()
{
    //qDebug() << "Const LinkContainer this=" << this;
    init();
}

LinkContainer::~LinkContainer()
{
    //qDebug()<< "Destructor LC  this=" << this;
}

void LinkContainer::init()
{
    containerType = Link;
}

void LinkContainer::addLink(LinkObj *lo)   // FIXME-0 only experimenting
{
    lo->setParentItem(this);
    lo->setFlag(ItemStacksBehindParent, true);
    if (childItems().count() > 1)
        lo->stackBefore(childItems().at(0));
}

void LinkContainer::reposition()
{
    //qDebug() << "LC::reposition " << info();
    return;
}

