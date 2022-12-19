#include "selectable-container.h"

#include "container.h"

void SelectableContainer::select(Container *parent, const QColor &color)
{
    if (!selectionContainer)
    {
        selectionContainer = new Container;
        selectionContainer->setContainerType(Selection);
        //selectionContainer->setPen(QPen(Qt::red));
        selectionContainer->setPen(QPen(Qt::NoPen));
        selectionContainer->setBrush(color);
        selectionContainer->overlay = true;
    }
    selectionContainer->setParentItem(parent);
    selectionContainer->setZValue(10);  // FIXME-2 align z values in ornamentsContainer

    // Initially set rectangle
    selectionContainer->setRect(parent->rect());
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

