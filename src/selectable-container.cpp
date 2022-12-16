#include "selectable-container.h"

#include "container.h"

void SelectableContainer::select(Container *c)  // FIXME-0 selection rect is lost when flag is toggled, scrolled (repositioned?)
{
    if (!selectionContainer)
    {
        selectionContainer = new Container; // FIXME-00 Container gets zero size in Container::reposition without childs and thus disappears
        selectionContainer->setContainerType(Selection);
        selectionContainer->setPen(QPen(Qt::red));
        selectionContainer->setBrush(Qt::yellow);
        selectionContainer->overlay = true;
        //selectionContainer->setFlag(ItemStacksBehindParent, true);
    }
    selectionContainer->setParentItem(c);
    selectionContainer->setZValue(10);  // FIXME-2 align z values in ornamentsContainer
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

