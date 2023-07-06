#include "selectable-container.h"

#include <QDebug>

#include "container.h"

void SelectableContainer::select(Container *parent, const QPen &pen, const QBrush &brush)
{
    if (!selectionContainer)
    {
        selectionContainer = new Container;
        selectionContainer->setContainerType(Selection);
        selectionContainer->overlay = true;
    }
    selectionContainer->setPen(pen);
    selectionContainer->setBrush(brush);

    parent->addContainer(selectionContainer, Z_SELECTION);
    selectionContainer->setFlag(QGraphicsItem::ItemStacksBehindParent, true);

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

