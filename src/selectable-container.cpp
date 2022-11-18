#include "selectable-container.h"

#include "container.h"

void SelectableContainer::select(Container *c)
{
    if (!selectionContainer)
    {
        selectionContainer = new Container();
        selectionContainer->setContainerType(Selection);
        selectionContainer->setPen(QPen(Qt::red));
        selectionContainer->setBrush(Qt::yellow);
        selectionContainer->overlay = true;
        selectionContainer->setFlag(ItemStacksBehindParent, true);
        selectionContainer->setZValue(10);
    }
    selectionContainer->setParentItem(c);
    selectionContainer->setRect(c->rect());
}

void SelectableContainer::unselect()
{
    if (!selectionContainer) return;

    delete selectionContainer;
    selectionContainer = nullptr;
}

bool SelectableContainer::isSelected()
{
    if (selectionContainer)
        return true;
    else
        return false;
}

