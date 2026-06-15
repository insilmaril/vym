#include "itemlist-wrapper.h"

#include <QQmlEngine>

#include "vymmodel.h"

#include <iostream>

#include "branchitem.h"
#include "vymmodelwrapper.h"

ItemListWrapper::ItemListWrapper(VymModel* model)
{
    std::cout << "Constr ItemListWrapper (VM) " << this << std::endl;
    init();

    modelInt = model;
}

ItemListWrapper::~ItemListWrapper()
{
    std::cout << "Destr ItemListWrapper " << this << std::endl;
}

void ItemListWrapper::init()
{
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    itemList.clear();
    deepLevelsFirstInt = false;
    reset();
}

void ItemListWrapper::reset()
{
    currentIndex = -1;
}

void ItemListWrapper::setModeBranches(bool deepLevelsFirst)
{
    init();

    deepLevelsFirstInt = deepLevelsFirst;
    BranchItem *cur = nullptr;
    BranchItem *prev = nullptr;
    modelInt->nextBranch(cur, prev, deepLevelsFirst);
    while (cur) {
        itemList << cur->getID();
        //qDebug() << " - adding: " << modelInt->headingText(cur);
        modelInt->nextBranch(cur, prev, deepLevelsFirst);
    }
}

void ItemListWrapper::setModeSelectedBranches()
{
    init();

    QList <BranchItem*> selbis = modelInt->getSelectedBranches();

    foreach (BranchItem *bi, selbis)
        itemList << bi->getID();
}

void ItemListWrapper::setModeSelectedSubtrees(bool deepLevelsFirst)
{
    init();

    deepLevelsFirstInt = deepLevelsFirst;

    QList <BranchItem*> selbis = modelInt->getSelectedBranches();

    foreach (BranchItem *bi, selbis) {
        BranchItem *cur = nullptr;
        BranchItem *prev = nullptr;
        modelInt->nextBranch(cur, prev, deepLevelsFirst, bi);
        while (cur) {
            itemList << cur->getID();
            modelInt->nextBranch(cur, prev, deepLevelsFirst, bi);
        }
    }
}

BranchWrapper* ItemListWrapper::nextBranch()    // FIXME-3 assumes currently only branches in list
{
    if (itemList.count() == 0)
        return nullptr;

    currentIndex++;

    if (currentIndex > itemList.count() - 1)
        return nullptr;

    TreeItem *ti = modelInt->findID(itemList.at(currentIndex));

    if (ti && ti->hasTypeBranch())
        return ((BranchItem*)ti)->branchWrapper();
    else
        return nullptr;
}

uint ItemListWrapper::count()
{
    return itemList.count();
}

