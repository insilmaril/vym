#include "selectable-container.h"

#include "container.h"

void SelectableContainer::select(Container *c)
{
    if (!selectionContainer)
    {
        selectionContainer = new Container;
        selectionContainer->setContainerType(Selection);
        selectionContainer->setPen(QPen(Qt::red));
        selectionContainer->setBrush(Qt::yellow);
        selectionContainer->overlay = true;
    }
    selectionContainer->setParentItem(c);
    selectionContainer->setZValue(10);  // FIXME-2 align z values in ornamentsContainer

    // Initially set rectangle
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

