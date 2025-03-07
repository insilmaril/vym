#include <QDebug>

#include "linkable-container.h"
#include "linkobj.h"

LinkableContainer::LinkableContainer()
{
    // qDebug() << "* Constr LinkableContainer" << this;

    // Create an uplink for every branch or image container
    // This link will become the child of my parents
    // linkContainer later. This will allow moving when parent moves,
    // without recalculating geometry.
    upLink = new LinkObj;
}

LinkableContainer::~LinkableContainer()
{
    // qDebug() << "* Destr LinkableContainer" << this;
    if (upLink)
        delete upLink;
}


LinkObj *LinkableContainer::getLink() { return upLink; }

void LinkableContainer::setUpLinkPosHint(const LinkObj::PosHint &ph)
{
    upLinkPosHintInt = ph;
}

void LinkableContainer::setDownLinkPosHint(const LinkObj::PosHint &ph)
{
    downLinkPosHintInt = ph;
}

