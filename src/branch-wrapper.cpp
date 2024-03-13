#include "branch-wrapper.h"

#include "branchitem.h"

BranchWrapper::BranchWrapper(BranchItem *bi)
{
    //qDebug() << "Constr BranchWrapper (BI)";
    branchItem = bi;
}

BranchWrapper::~BranchWrapper()
{
    //qDebug() << "Destr BranchWrapper";
}


QString BranchWrapper::headingText()
{
    return branchItem->getHeadingPlain();
}
