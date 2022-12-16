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
    // qDebug()<< "Destructor LC  this=" << this << "children: " << childItems().count();

    // Unparent any children, will be taken care of in own destructors
    while (!childItems().isEmpty())
        childItems().first()->setParentItem(nullptr);
}

void LinkContainer::init()
{
    containerType = Link;
}

void LinkContainer::addLink(LinkObj *lo)
{
    if (lo->parentItem() != this)
        lo->setParentItem(this);
}

void LinkContainer::reposition()
{
    //qDebug() << "LC::reposition " << info();
    return;
}

