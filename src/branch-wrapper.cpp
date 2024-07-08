#include "branch-wrapper.h"

#include "attributeitem.h"
#include "branchitem.h"

#include "vymmodel.h"

#include <QJSEngine>
extern QJSEngine *scriptEngine;

BranchWrapper::BranchWrapper(BranchItem *bi)
{
    // qDebug() << "Constr BranchWrapper (BI)";
    branchItemInt = bi;
}

BranchWrapper::~BranchWrapper()
{
    // qDebug() << "Destr BranchWrapper";
}

BranchItem* BranchWrapper::branchItem()
{
    return branchItemInt;
}

VymModel* BranchWrapper::model()
{
    return branchItemInt->getModel();
}

void BranchWrapper::addBranch()
{
    if (branchItemInt) {
        if (!model()->addNewBranch(branchItemInt, -2))
        scriptEngine->throwError(QJSValue::GenericError,"Couldn't add branch to map");
        return;
    } else {
        scriptEngine->throwError(QJSValue::RangeError, QString("No branch selected"));
        return;
    }
}

void BranchWrapper::addBranchAt(int pos)
{
    if (!model()->addNewBranch(branchItemInt, pos))
        scriptEngine->throwError(
                QJSValue::GenericError,
                QString("Could not add  branch at position %1").arg(pos));
}

void BranchWrapper::addBranchBefore()
{
    if (!model()->addNewBranchBefore(branchItemInt))
        scriptEngine->throwError(
                QJSValue::GenericError,
                "Couldn't add branch before selection to map");
}

bool BranchWrapper::addMapInsert(QString fileName, int pos)
{
    if (QDir::isRelativePath(fileName))
        fileName = QDir::currentPath() + "/" + fileName;

    return setResult(model()->addMapInsert(fileName, pos, branchItemInt));
}

int BranchWrapper::attributeAsInt(const QString &key)
{
    QVariant v;
    AttributeItem *ai = model()->getAttributeByKey(key);
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
    AttributeItem *ai = model()->getAttributeByKey(key);
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

int BranchWrapper::branchCount()
{
    return setResult(branchItemInt->branchCount());
}

void BranchWrapper::clearFlags() { model()->clearFlags(branchItemInt); }

void BranchWrapper::colorBranch(const QString &color)
{
    QColor col(color);
    if (!col.isValid())
        scriptEngine->throwError(
                QJSValue::GenericError,
                QString("Could not set color to %1").arg(color));
    else
        model()->colorBranch(col, branchItemInt);
}

void BranchWrapper::colorSubtree(const QString &color)
{
    QColor col(color);
    if (!col.isValid())
        scriptEngine->throwError(
                QJSValue::GenericError,
                QString("Could not set color to %1").arg(color));
    else
        model()->colorSubtree(col, branchItemInt);
}

bool BranchWrapper::cycleTask(bool reverse)
{
    bool r = model()->cycleTaskStatus(branchItemInt, reverse);

    return setResult(r);
}

int BranchWrapper::getFramePadding(const bool &useInnerFrame)
{
    return setResult(branchItemInt->getBranchContainer()->framePadding(useInnerFrame));
}

int BranchWrapper::getFramePenWidth(const bool &useInnerFrame)
{
    return setResult(branchItemInt->getBranchContainer()->framePenWidth(useInnerFrame));
}

QString BranchWrapper::getFrameType(const bool &useInnerFrame)
{
    return setResult(branchItemInt->getBranchContainer()->frameTypeString(useInnerFrame));
}

QString BranchWrapper::getUid()
{
    return setResult(branchItemInt->getUuid().toString());
}

void BranchWrapper::getJiraData(bool subtree)
{
    model()->getJiraData(subtree, branchItemInt);
}

QString BranchWrapper::getNoteText()
{
    return setResult(branchItemInt->getNote().getTextASCII());
}

QString BranchWrapper::getNoteXML()
{
    return setResult(branchItemInt->getNote().saveToDir());
}

int BranchWrapper::getNum()
{
    return setResult(branchItemInt->num());
}

qreal BranchWrapper::getPosX()
{
    return setResult(branchItemInt->getBranchContainer()->pos().x());
}

qreal BranchWrapper::getPosY()
{
    return setResult(branchItemInt->getBranchContainer()->pos().y());
}

int BranchWrapper::getTaskPriorityDelta()
{
    return model()->getTaskPriorityDelta(branchItemInt);
}

QString BranchWrapper::getTaskSleep()
{
    QString r;
    Task *task = branchItemInt->getTask();
    if (task)
        r = task->getSleep().toString(Qt::ISODate);
    else
        scriptEngine->throwError(
                QJSValue::GenericError,
                "Branch has no task");
    return setResult(r);
}

int BranchWrapper::getTaskSleepDays()
{
    int r = -1;
        Task *task = branchItemInt->getTask();
    if (task)
        r = task->getDaysSleep();
    else
        scriptEngine->throwError(QJSValue::GenericError, "Branch has no task");
    return setResult(r);
}

QString BranchWrapper::getTaskStatus()
{
    QString r;
    Task *task = branchItemInt->getTask();
    if (task) 
        r = task->getStatusString();
    else
        scriptEngine->throwError(QJSValue::GenericError, "Branch has no task");

    return setResult(r);
}

QString BranchWrapper::getUrl()
{
    return setResult(branchItemInt->url());
}

QString BranchWrapper::getVymLink()
{
    return setResult(branchItemInt->vymLink());
}

bool BranchWrapper::hasActiveFlag(const QString &flag)
{
    return setResult(branchItemInt->hasActiveFlag(flag));
}

bool BranchWrapper::hasNote()
{
    bool r = !branchItemInt->getNote().isEmpty();
    return setResult(r);
}

bool BranchWrapper::hasRichTextHeading()
{
    return setResult(branchItemInt->heading().isRichText());
}

bool BranchWrapper::hasRichTextNote()
{
    return setResult(branchItemInt->getNote().isRichText());
}

bool BranchWrapper::hasTask()
{
    bool r = false;
    Task *task = branchItemInt->getTask();
    if (task)
        r = true;

    return setResult(r);
}

QString BranchWrapper::headingText()
{
    return setResult(branchItemInt->headingPlain());
}

int BranchWrapper::imageCount()
{
    return setResult(branchItemInt->imageCount());
}

void BranchWrapper::importDir(const QString &path) // FIXME-4 error handling missing (in vymmodel and here)
{
    branchItemInt->getModel()->importDir(path, branchItemInt);
}

bool BranchWrapper::loadImage(const QString &filename)
{
    bool r;
    ImageItem *ii = model()->loadImage(branchItemInt, filename);
    r= (ii) ? true : false;
    return setResult(r);
}

bool BranchWrapper::loadNote(const QString &filename)
{
    return setResult(model()->loadNote(filename, branchItemInt));
}

void BranchWrapper::moveDown()
{
    model()->moveDown(branchItemInt);
}

void BranchWrapper::moveUp()
{
    model()->moveUp(branchItemInt);
}

BranchWrapper* BranchWrapper::parentBranch()
{
    return branchItemInt->parentBranch()->branchWrapper();
}

bool BranchWrapper::isScrolled()
{
    return setResult(branchItemInt->isScrolled());
}

bool BranchWrapper::relinkToBranch(BranchWrapper *dst)
{
    return setResult(model()->relinkBranch(branchItemInt, dst->branchItemInt));
}

bool BranchWrapper::relinkToBranchAt(BranchWrapper *dst, int pos)
{
    return setResult(model()->relinkBranch(branchItemInt, dst->branchItemInt, pos));
}

void BranchWrapper::removeChildren()
{
    branchItemInt->getModel()->deleteChildren(branchItemInt);
}

void BranchWrapper::removeChildrenBranches()
{
    branchItemInt->getModel()->deleteChildrenBranches(branchItemInt);
}

void BranchWrapper::scroll()
{
    model()->scrollBranch(branchItemInt);
}

void BranchWrapper::select()
{
    model()->select(branchItemInt);
}

bool BranchWrapper::selectFirstBranch()
{
    return setResult(model()->selectFirstBranch(branchItemInt));
}

bool BranchWrapper::selectFirstChildBranch()
{
    return setResult(model()->selectFirstChildBranch(branchItemInt));
}

bool BranchWrapper::selectLastChildBranch()
{
    return setResult(model()->selectLastChildBranch(branchItemInt));
}

bool BranchWrapper::selectLastBranch()
{
    return setResult(model()->selectLastBranch(branchItemInt));
}
bool BranchWrapper::selectParent()
{
    bool r = model()->selectParent(branchItemInt);
    if (!r)
        scriptEngine->throwError(
                QJSValue::GenericError,
                "Couldn't select parent item");
    return setResult(r);
}

void BranchWrapper::setAttribute(const QString &key, const QString &value)
{
    model()->setAttribute(branchItemInt, key, value);
}

void BranchWrapper::setFlagByName(const QString &s)
{
    model()->setFlagByName(s, branchItemInt);
}

void BranchWrapper::setFrameBrushColor(const bool &useInnerFrame, const QString &color)
{
    model()->setFrameBrushColor(useInnerFrame, color, branchItemInt);
}

void BranchWrapper::setFramePadding(const bool &useInnerFrame, int padding)
{
    model()->setFramePadding(useInnerFrame, padding, branchItemInt);
}

void BranchWrapper::setFramePenColor(const bool &useInnerFrame, const QString &color)
{
    model()->setFramePenColor(useInnerFrame, color, branchItemInt);
}

void BranchWrapper::setFramePenWidth(const bool &useInnerFrame, int width)
{
    model()->setFramePenWidth(useInnerFrame, width, branchItemInt);
}

void BranchWrapper::setFrameType(const bool &useInnerFrame, const QString &type)
{
    model()->setFrameType(useInnerFrame, type, branchItemInt);
}

void BranchWrapper::setHeadingRichText(const QString &text)
{
    // Set plaintext heading
    VymText vt;
    vt.setRichText(text);
    model()->setHeading(vt, branchItemInt);
}

void BranchWrapper::setHeadingText(const QString &text)
{
    // Set plaintext heading
    model()->setHeadingPlainText(text, branchItemInt);
}

void BranchWrapper::setNoteRichText(const QString &s)
{
    VymNote vn;
    vn.setRichText(s);
    model()->setNote(vn, branchItemInt);
}

void BranchWrapper::setNoteText(const QString &s)
{
    VymNote vn;
    vn.setPlainText(s);
    model()->setNote(vn, branchItemInt);
}

void BranchWrapper::setPos(qreal x, qreal y)
{
    model()->setPos(QPointF(x, y), branchItemInt);
}

void BranchWrapper::setTaskPriorityDelta(const int &n)
{
    model()->setTaskPriorityDelta(n, branchItemInt);
}

bool BranchWrapper::setTaskSleep(const QString &s)
{
    return setResult(model()->setTaskSleep(s, branchItemInt));
}

void BranchWrapper::setUrl(const QString &s)
{
    model()->setUrl(s, true, branchItemInt);
}

void BranchWrapper::setVymLink(const QString &s)
{
    model()->setVymLink(s, branchItemInt);
}

void BranchWrapper::sortChildren(bool b)
{
    branchItemInt->getModel()->sortChildren(b, branchItemInt);
}

void BranchWrapper::sortChildren()
{
    sortChildren(false);
}

void BranchWrapper::toggleFlagByName(const QString &s)
{
    model()->toggleFlagByName(s, branchItemInt);
}

void BranchWrapper::toggleFlagByUid(const QString &s)
{
    model()->toggleFlagByUid(QUuid(s), branchItemInt);
}

void BranchWrapper::toggleScroll()
{
    model()->toggleScroll(branchItemInt);
}

void BranchWrapper::toggleTask() {
    model()->toggleTask(branchItemInt);
}

void BranchWrapper::unscroll()
{
    model()->unscrollBranch(branchItemInt);
}

void BranchWrapper::unsetFlagByName(const QString &s)
{
    model()->unsetFlagByName(s, branchItemInt);
}

