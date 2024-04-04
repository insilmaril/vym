#include "branch-wrapper.h"

#include "attributeitem.h"
#include "branchitem.h"

#include "vymmodel.h"

#include <QJSEngine>
extern QJSEngine *scriptEngine;

BranchWrapper::BranchWrapper(BranchItem *bi)
{
    //qDebug() << "Constr BranchWrapper (BI)";
    branchItem = bi;
}

BranchWrapper::~BranchWrapper()
{
    //qDebug() << "Destr BranchWrapper";
}

int BranchWrapper::attributeAsInt(const QString &key)
{
    QVariant v;
    AttributeItem *ai = branchItem->getModel()->getAttributeByKey(key);
    if (ai) {
        v = ai->value();
    } else {
        scriptEngine->throwError(
                QJSValue::GenericError,
                QString("No attribute found with key '%1'").arg(key));
        return setResult(-1);
    }

    bool ok;
    int i = v.toInt(&ok);
    if (ok)
        return setResult(v.toInt());
    else {
        scriptEngine->throwError(
                QJSValue::GenericError,
                QString("Could not convert attribute  with key '%1' to int.").arg(key));
        return setResult(-1);
    }
}

QString BranchWrapper::attributeAsString(const QString &key)
{
    QVariant v;
    AttributeItem *ai = branchItem->getModel()->getAttributeByKey(key);
    if (ai) {
        v = ai->value();
    } else {
        scriptEngine->throwError(
                QJSValue::GenericError,
                QString("No attribute found with key '%1'").arg(key));
        return setResult(QString());
    }
    //
    // Returned string will be empty for unsupported variant types
    return setResult(v.toString());
}


QString BranchWrapper::headingText()
{
    return branchItem->headingPlain();
}

bool BranchWrapper::relinkTo(BranchWrapper *bw)
{
    branchItem->getModel()->relinkBranch(branchItem, bw->branchItem);
    return false;
}

void BranchWrapper::select()
{
    branchItem->getModel()->select(branchItem);
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
