#include "itemlist-wrapper.h"

#include <QQmlEngine>

#include "vymmodel.h"

#include <iostream>

#include "branchitem.h"

ItemListWrapper::ItemListWrapper(VymModel* model)
{
    std::cout << "Constr ItemListWrapper () " << this << std::endl;
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    modelInt = model;

    itemList.clear();
    
    BranchItem *cur = nullptr;
    BranchItem *prev = nullptr;
    modelInt->nextBranch(cur, prev);
    while (cur) {
        itemList << cur->getID();
        qDebug() << " - adding: " << modelInt->headingText(cur);
        modelInt->nextBranch(cur, prev);
    }

}

ItemListWrapper::~ItemListWrapper()
{
    std::cout << "Destr ItemListWrapper " << this << std::endl;
}

BranchWrapper* ItemListWrapper::nextBranch()
{
    return nullptr;
}

uint ItemListWrapper::count()
{
    return itemList.count();
}

