#include "selectable-container.h"

#include <QDebug>

#include "container.h"

void SelectableContainer::select(Container *parent, QGraphicsItem* successorItem, const QColor &color)
{
    if (!selectionContainer)
    {
        selectionContainer = new Container;
        selectionContainer->setContainerType(Selection);
        selectionContainer->setPen(QPen(Qt::NoPen));
        selectionContainer->setBrush(color);
        selectionContainer->overlay = true;
    }
    selectionContainer->setParentItem(parent);
    if (successorItem)
	selectionContainer->stackBefore(successorItem);

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

