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

void BranchWrapper::addBranch()
{
    if (branchItem) {
        if (!branchItem->getModel()->addNewBranch(branchItem, -2))
        scriptEngine->throwError(QJSValue::GenericError,"Couldn't add branch to map");
        return;
    } else {
        scriptEngine->throwError(QJSValue::RangeError, QString("No branch selected"));
        return;
    }
}

void BranchWrapper::addBranchAt(int pos)
{
    if (!branchItem->getModel()->addNewBranch(branchItem, pos))
        scriptEngine->throwError(
                QJSValue::GenericError,
                QString("Could not add  branch at position %1").arg(pos));
}

void BranchWrapper::addBranchBefore()
{
    if (!branchItem->getModel()->addNewBranchBefore(branchItem))
        scriptEngine->throwError(
                QJSValue::GenericError,
                "Couldn't add branch before selection to map");
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

void BranchWrapper::getJiraData(bool subtree)
{
    branchItem->getModel()->getJiraData(subtree, branchItem);
}

QString BranchWrapper::getNoteText()
{
    return setResult(branchItem->getNote().getTextASCII());
}

QString BranchWrapper::getNoteXML()
{
    return setResult(branchItem->getNote().saveToDir());
}

bool BranchWrapper::hasNote()
{
    bool r = !branchItem->getNote().isEmpty();
    return setResult(r);
}

bool BranchWrapper::hasRichTextNote()
{
    return setResult(branchItem->getNote().isRichText());
}

QString BranchWrapper::headingText()
{
    return setResult(branchItem->headingPlain());
}

bool BranchWrapper::isScrolled()
{
    return setResult(branchItem->isScrolled());
}

bool BranchWrapper::relinkTo(BranchWrapper *bw)
{
    return setResult(branchItem->getModel()->relinkBranch(branchItem, bw->branchItem));
}

void BranchWrapper::remove()
{
    branchItem->getModel()->deleteSelection(branchItem->getID());
}

void BranchWrapper::scroll()
{
    branchItem->getModel()->scrollBranch(branchItem);
}

void BranchWrapper::select()
{
    branchItem->getModel()->select(branchItem);
}

void BranchWrapper::setAttribute(const QString &key, const QString &value)
{
    branchItem->getModel()->setAttribute(branchItem, key, value);
}

void BranchWrapper::setFlagByName(const QString &s)
{
    branchItem->getModel()->setFlagByName(s, branchItem);
}

void BranchWrapper::setHeadingRichText(const QString &text)
{
    // Set plaintext heading
    VymText vt;
    vt.setRichText(text);
    branchItem->getModel()->setHeading(vt, branchItem);
}

void BranchWrapper::setHeadingText(const QString &text)
{
    // Set plaintext heading
    branchItem->getModel()->setHeadingPlainText(text, branchItem);
}

void BranchWrapper::setNoteRichText(const QString &s)
{
    VymNote vn;
    vn.setRichText(s);
    branchItem->getModel()->setNote(vn, branchItem);
}

void BranchWrapper::setNoteText(const QString &s)
{
    VymNote vn;
    vn.setPlainText(s);
    branchItem->getModel()->setNote(vn, branchItem);
}

void BranchWrapper::setPos(qreal x, qreal y)
{
    branchItem->getModel()->setPos(QPointF(x, y), branchItem);
}

void BranchWrapper::toggleFlagByName(const QString &s)
{
    branchItem->getModel()->toggleFlagByName(s, branchItem);
}

void BranchWrapper::toggleScroll()
{
    branchItem->getModel()->toggleScroll(branchItem);
}

void BranchWrapper::unscroll()
{
    branchItem->getModel()->unscrollBranch(branchItem);
}

void BranchWrapper::unsetFlagByName(const QString &s)
{
    branchItem->getModel()->unsetFlagByName(s, branchItem);
}
