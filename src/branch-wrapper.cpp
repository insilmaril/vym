#include "branch-wrapper.h"

#include "attributeitem.h"
#include "branchitem.h"
#include "branch-container.h"

#include "misc.h"
#include "scripting-xlink-wrapper.h"
#include "task.h"
#include "vymmodel.h"
#include "mainwindow.h"
#include "xlink.h"
#include "xlinkitem.h"

extern Main *mainWindow;

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

QPointF BranchWrapper::v_anim() // FIXME-3 experimental, no highlighting
{
    return branchItemInt->getBranchContainer()->v_anim;
}

qreal BranchWrapper::v_animX()  // FIXME-3 experimental, no highlighting
{
    return branchItemInt->getBranchContainer()->v_anim.x();
}

qreal BranchWrapper::v_animY()  // FIXME-3 experimental, no highlighting
{
    return branchItemInt->getBranchContainer()->v_anim.y();
}

void BranchWrapper::setV_anim(qreal x, qreal y) // FIXME-2 playing with elastic animation
{
    branchItemInt->getBranchContainer()->v_anim = QPointF(x, y);
    branchItemInt->getBranchContainer()->v.setLine(0, 0, x * 40, y * 40);
    branchItemInt->getBranchContainer()->v.setVisible(true);
}

BranchWrapper* BranchWrapper::addBranch()
{
    BranchItem* newbi = model()->addNewBranch(branchItemInt, -2);
    if (!newbi) {
        mainWindow->abortScript(QJSValue::GenericError,"Couldn't add branch to map");
        return nullptr;
    } else
        return newbi->branchWrapper();
}

BranchWrapper* BranchWrapper::addBranchAt(int pos)
{
    BranchItem* newbi = model()->addNewBranch(branchItemInt, -2);
    if (!newbi) {
        mainWindow->abortScript(QJSValue::GenericError,"Couldn't add branch to map");
        return nullptr;
    } else
        return newbi->branchWrapper();
}

BranchWrapper* BranchWrapper::addBranchBefore()
{
    BranchItem* newbi = model()->addNewBranchBefore(branchItemInt);
    if (!newbi) {
        mainWindow->abortScript(
                QJSValue::GenericError,
                "Couldn't add branch before selection to map");
        return nullptr;
    } else
        return newbi->branchWrapper();
}

XLinkWrapper* BranchWrapper::addXLink(BranchWrapper *bwEnd,
                               int width, const QString &color,
                               const QString &penstyle)
{
    BranchItem *biEnd = bwEnd->branchItem();
    XLink *li = new XLink(model());
    li->setBeginBranch(branchItemInt);
    li->setEndBranch(biEnd);

    model()->createXLink(li);
    QPen pen = li->getPen();
    if (width > 0)
        pen.setWidth(width);
    QColor col(color);
    if (col.isValid())
        pen.setColor(col);
    else {
        mainWindow->abortScript(
                QJSValue::GenericError,
                QString("Could not set color to %1").arg(color));
        return nullptr;
    }

    bool ok;
    Qt::PenStyle st1 = penStyle(penstyle, ok);
    if (ok) {
        pen.setStyle(st1);
        li->setPen(pen);
    } else {
        mainWindow->abortScript(
                QJSValue::GenericError,
                QString("Couldn't set penstyle %1").arg(penstyle));
        return nullptr;
    }
    return li->xlinkWrapper();
}

int BranchWrapper::attributeAsInt(const QString &key)
{
    QVariant v;
    AttributeItem *ai = model()->getAttributeByKey(key);
    if (ai) {
        v = ai->value();
    } else {
        mainWindow->abortScript(
                QJSValue::GenericError,
                QString("No attribute found with key '%1'").arg(key));
    }

    bool ok;
    int i = v.toInt(&ok);
    if (ok) {
        mainWindow->setScriptResult(i);
        return i;
    } else {
        mainWindow->abortScript(
                QJSValue::GenericError,
                QString("Could not convert attribute  with key '%1' to int.").arg(key));
    }
    return -1;
}

QString BranchWrapper::attributeAsString(const QString &key)
{
    QVariant v;
    AttributeItem *ai = model()->getAttributeByKey(key);
    if (ai) {
        v = ai->value();
    } else {
        mainWindow->abortScript(
                QJSValue::GenericError,
                QString("No attribute found with key '%1'").arg(key));
    }
    //
    // Returned string will be empty for unsupported variant types
    QString r = v.toString();
    mainWindow->setScriptResult(r);
    return r;
}

int BranchWrapper::branchCount()
{
    int r = branchItemInt->branchCount();
    mainWindow->setScriptResult(r);
    return r;
}

void BranchWrapper::clearFlags() { model()->clearFlags(branchItemInt); }

void BranchWrapper::colorBranch(const QString &color)
{
    QColor col(color);
    if (!col.isValid())
        mainWindow->abortScript(
                QJSValue::GenericError,
                QString("Could not set color to %1").arg(color));
    else
        model()->colorBranch(col, branchItemInt);
}

void BranchWrapper::colorSubtree(const QString &color)
{
    QColor col(color);
    if (!col.isValid())
        mainWindow->abortScript(
                QJSValue::GenericError,
                QString("Could not set color to %1").arg(color));
    else
        model()->colorSubtree(col, branchItemInt);
}

bool BranchWrapper::cycleTask(bool reverse)
{
    bool r = model()->cycleTaskStatus(branchItemInt, reverse);

    mainWindow->setScriptResult(r);
    return r;
}

int BranchWrapper::getFramePadding(const bool &useInnerFrame)
{
    int r =  branchItemInt->getBranchContainer()->framePadding(useInnerFrame);
    mainWindow->setScriptResult(r);
    return r;
}

int BranchWrapper::getFramePenWidth(const bool &useInnerFrame)
{
    int r =  branchItemInt->getBranchContainer()->framePenWidth(useInnerFrame);
    mainWindow->setScriptResult(r);
    return r;
}

QString BranchWrapper::getFrameType(const bool &useInnerFrame)
{
    QString r = branchItemInt->getBranchContainer()->frameTypeString(useInnerFrame);
    mainWindow->setScriptResult(r);
    return r;
}

QString BranchWrapper::getHeading()
{
    QString r = branchItemInt->heading().getText();
    mainWindow->setScriptResult(r);
    return r;
}

QString BranchWrapper::getHeadingXML()
{
    QString r = branchItemInt->heading().saveToDir();
    mainWindow->setScriptResult(r);
    return r;
}

void BranchWrapper::getJiraData(bool subtree)
{
    model()->getJiraData(subtree, branchItemInt);
}

QString BranchWrapper::getNoteText()
{
    QString r =  branchItemInt->getNote().getTextASCII();
    mainWindow->setScriptResult(r);
    return r;
}

QString BranchWrapper::getNoteXML()
{
    QString r = branchItemInt->getNote().saveToDir();
    mainWindow->setScriptResult(r);
    return r;
}

int BranchWrapper::getNum()
{
    int r = branchItemInt->num();
    mainWindow->setScriptResult(r);
    return r;
}

qreal BranchWrapper::getPosX()
{
    qreal r = branchItemInt->getBranchContainer()->pos().x();
    mainWindow->setScriptResult(r);
    return r;
}

qreal BranchWrapper::getPosY()
{
    qreal r = branchItemInt->getBranchContainer()->pos().y();
    mainWindow->setScriptResult(r);
    return r;
}

QPointF BranchWrapper::getScenePos()
{
    QPointF r = branchItemInt->getBranchContainer()->scenePos();
    mainWindow->setScriptResult(r);
    return r;
}

qreal BranchWrapper::getScenePosX()
{
    qreal r =  branchItemInt->getBranchContainer()->scenePos().x();
    mainWindow->setScriptResult(r);
    return r;
}

qreal BranchWrapper::getScenePosY()
{
    qreal r =  branchItemInt->getBranchContainer()->scenePos().y();
    mainWindow->setScriptResult(r);
    return r;
}

int BranchWrapper::getTaskPriorityDelta()
{
    int r =  model()->getTaskPriorityDelta(branchItemInt);
    mainWindow->setScriptResult(r);
    return r;
}

QString BranchWrapper::getTaskSleep()
{
    QString r;
    Task *task = branchItemInt->getTask();
    if (task)
        r = task->getSleep().toString(Qt::ISODate);
    else
        mainWindow->abortScript(
                QJSValue::GenericError,
                "Branch has no task");
    mainWindow->setScriptResult(r);
    return r;
}

int BranchWrapper::getTaskSleepDays()
{
    int r = -1;
        Task *task = branchItemInt->getTask();
    if (task)
        r = task->getDaysSleep();
    else
        mainWindow->abortScript(QJSValue::GenericError, "Branch has no task");
    mainWindow->setScriptResult(r);
    return r;
}

QString BranchWrapper::getTaskStatus()
{
    QString r;
    Task *task = branchItemInt->getTask();
    if (task) 
        r = task->getStatusString();
    else
        mainWindow->abortScript(QJSValue::GenericError, "Branch has no task");

    mainWindow->setScriptResult(r);
    return r;
}

QString BranchWrapper::getUid()
{
    QString r = branchItemInt->getUuid().toString();
    mainWindow->setScriptResult(r);
    return r;
}

QString BranchWrapper::getUrl()
{
    QString r = branchItemInt->url();
    mainWindow->setScriptResult(r);
    return r;
}

QString BranchWrapper::getVymLink()
{
    QString r = branchItemInt->vymLink();
    mainWindow->setScriptResult(r);
    return r;
}

bool BranchWrapper::hasActiveFlag(const QString &flag)
{
    bool r = branchItemInt->hasActiveFlag(flag);
    mainWindow->setScriptResult(r);
    return r;
}

bool BranchWrapper::hasNote()
{
    bool r = !branchItemInt->getNote().isEmpty();
    mainWindow->setScriptResult(r);
    return r;
}

bool BranchWrapper::hasRichTextHeading()
{
    bool r = branchItemInt->heading().isRichText();
    mainWindow->setScriptResult(r);
    return r;
}

bool BranchWrapper::hasRichTextNote()
{
    bool r = branchItemInt->getNote().isRichText();
    mainWindow->setScriptResult(r);
    return r;
}

bool BranchWrapper::hasTask()
{
    bool r = false;
    Task *task = branchItemInt->getTask();
    if (task)
        r = true;

    mainWindow->setScriptResult(r);
    return r;
}

QString BranchWrapper::headingText()
{
    QString r = branchItemInt->headingPlain();
    mainWindow->setScriptResult(r);
    return r;
}

int BranchWrapper::imageCount()
{
    int r = branchItemInt->imageCount();
    mainWindow->setScriptResult(r);
    return r;
}

void BranchWrapper::importDir(const QString &path) // FIXME-5 error handling missing (in vymmodel and here)
{
    model()->importDir(path, branchItemInt);
}

bool BranchWrapper::loadBranchInsert(QString fileName, int pos)
{
    if (QDir::isRelativePath(fileName))
        fileName = QDir::currentPath() + "/" + fileName;

    bool r = model()->addMapInsert(fileName, pos, branchItemInt);
    mainWindow->setScriptResult(r);
    return r;
}

bool BranchWrapper::loadImage(const QString &filename)
{
    bool r;
    ImageItem *ii = model()->loadImage(branchItemInt, filename);
    r = (ii) ? true : false;
    mainWindow->setScriptResult(r);
    if (ii)
        return true;

    mainWindow->abortScript("Failed to load " + filename);
    return false;
}

bool BranchWrapper::loadNote(const QString &filename)
{
    bool r = model()->loadNote(filename, branchItemInt);
    mainWindow->setScriptResult(r);
    return r;
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
    bool r = branchItemInt->isScrolled();
    mainWindow->setScriptResult(r);
    return r;
}

bool BranchWrapper::relinkToBranch(BranchWrapper *dst)
{
    bool r = model()->relinkBranch(branchItemInt, dst->branchItemInt);
    mainWindow->setScriptResult(r);
    return r;
}

bool BranchWrapper::relinkToBranchAt(BranchWrapper *dst, int pos)
{
    bool r = model()->relinkBranch(branchItemInt, dst->branchItemInt, pos);
    mainWindow->setScriptResult(r);
    return r;
}

void BranchWrapper::removeChildren()
{
    model()->deleteChildren(branchItemInt);
}

void BranchWrapper::removeChildrenBranches()
{
    model()->deleteChildrenBranches(branchItemInt);
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
    bool r = model()->selectFirstBranch(branchItemInt);
    mainWindow->setScriptResult(r);
    return r;
}

bool BranchWrapper::selectFirstChildBranch()
{
    bool r = model()->selectFirstChildBranch(branchItemInt);
    mainWindow->setScriptResult(r);
    return r;
}

bool BranchWrapper::selectLastChildBranch()
{
    bool r = model()->selectLastChildBranch(branchItemInt);
    mainWindow->setScriptResult(r);
    return r;
}

bool BranchWrapper::selectLastBranch()
{
    bool r = model()->selectLastBranch(branchItemInt);
    mainWindow->setScriptResult(r);
    return r;
}
bool BranchWrapper::selectParent()
{
    bool r = model()->selectParent(branchItemInt);
    if (!r)
        mainWindow->abortScript(
                QJSValue::GenericError,
                "Couldn't select parent item");
    mainWindow->setScriptResult(r);
    return r;
}

bool BranchWrapper::selectXLink(int n)
{
    bool r = false;
        XLinkItem *xli = branchItemInt->getXLinkItemNum(n);
    if (!xli)
        mainWindow->abortScript(QJSValue::RangeError,
             QString("Selected branch has no xlink with index %1").arg(n));
    else
        r = model()->select((TreeItem*)xli);
    mainWindow->setScriptResult(r);
    return r;
}

bool BranchWrapper::selectXLinkOtherEnd(int n)
{
    bool r = false;
    XLinkItem *xli = branchItemInt->getXLinkItemNum(n);
    if (!xli) {
        mainWindow->abortScript(
                QJSValue::RangeError,
                QString("Selected branch has no xlink with index %1").arg(n));
    } else {
        BranchItem *bi = xli->getPartnerBranch();
        if (!bi) {
            mainWindow->abortScript(
                    QJSValue::RangeError,
                    "Selected xlink has no other end ?!");
        } else
            r = model()->select(bi);
    }
    mainWindow->setScriptResult(r);
    return r;
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

void BranchWrapper::setHideExport(bool b)
{
    model()->setHideExport(b, branchItemInt);
}

void BranchWrapper::setHideLinkUnselected(bool b)
{
    model()->setHideLinkUnselected(b, branchItemInt);
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

void BranchWrapper::setRotationAutoDesign(const bool b)
{
    model()->setRotationAutoDesign(b, branchItemInt);
}

void BranchWrapper::setRotationHeading(const int &i)
{
    model()->setRotationHeading(i, branchItemInt);
}

void BranchWrapper::setRotationSubtree(const int &i)
{
    model()->setRotationSubtree(i, branchItemInt);
}

void BranchWrapper::setScaleAutoDesign(const bool b)
{
    model()->setScaleAutoDesign(b, branchItemInt);
}

void BranchWrapper::setScaleHeading(qreal f) { model()->setScaleHeading(f, false, branchItemInt); }

void BranchWrapper::setScaleSubtree(qreal f) { model()->setScaleSubtree(f, branchItemInt); }

void BranchWrapper::setTaskPriorityDelta(const int &n)
{
    model()->setTaskPriorityDelta(n, branchItemInt);
}

bool BranchWrapper::setTaskSleep(const QString &s)
{
    bool r = model()->setTaskSleep(s, branchItemInt);
    mainWindow->setScriptResult(r);
    return r;
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
    model()->sortChildren(b, branchItemInt);
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

void BranchWrapper::toggleTarget()
{
    model()->toggleTarget(branchItemInt);
}

void BranchWrapper::toggleTask() {
    model()->toggleTask(branchItemInt);
}

void BranchWrapper::unscroll()
{
    model()->unscrollBranch(branchItemInt);
}

void BranchWrapper::unscrollSubtree()
{
    model()->unscrollSubtree(branchItemInt);
}

void BranchWrapper::unsetFlagByName(const QString &s)
{
    model()->unsetFlagByName(s, branchItemInt);
}


int BranchWrapper::xlinkCount()
{
    int r = branchItemInt->xlinkCount();
    mainWindow->setScriptResult(r);
    return r;
}
