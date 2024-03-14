#include "branch-wrapper.h"

#include "branchitem.h"

#include "vymmodel.h"

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

bool BranchWrapper::relinkTo(BranchWrapper *bw)
{
    qDebug() << "BW::relinkTo " << bw->headingText();
    return false;   // FIXME-2
}

void BranchWrapper::setFlagByName(const QString &s)
{
    branchItem->getModel()->setFlagByName(s, branchItem);
}

void BranchWrapper::toggleFlagByName(const QString &s)
{
    branchItem->getModel()->toggleFlagByName(s, branchItem);
}
void BranchWrapper::unsetFlagByName(const QString &s)
{
    branchItem->getModel()->unsetFlagByName(s, branchItem);
}
