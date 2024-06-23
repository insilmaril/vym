#include "vymmodelwrapper.h"

#include <QMessageBox>

#include "attributeitem.h"
#include "branchitem.h"
#include "branch-wrapper.h"
#include "imageitem.h"
#include "image-wrapper.h"
#include "misc.h"
#include "scripting.h"
#include "vymmodel.h"
#include "vymtext.h"
#include "xlink.h"
#include "xlinkitem.h"
#include "xmlobj.h" // include quoteQuotes

#include <QJSEngine>
extern QJSEngine *scriptEngine;

///////////////////////////////////////////////////////////////////////////
VymModelWrapper::VymModelWrapper(VymModel *m) { model = m; }

void VymModelWrapper::addMapCenterAtPos(qreal x, qreal y)
{
    if (!model->addMapCenterAtPos(QPointF(x, y)))
        scriptEngine->throwError(
                QJSValue::GenericError,
                "Couldn't add mapcenter");
}

void VymModelWrapper::addMapInsert(QString fileName, int pos, int contentFilter)
{
    if (QDir::isRelativePath(fileName))
        fileName = QDir::currentPath() + "/" + fileName;

    model->saveStateBeforeLoad(File::ImportAdd, fileName);

    if (File::Aborted ==
        model->loadMap(fileName, File::ImportAdd, File::VymMap, contentFilter, pos))
        scriptEngine->throwError(
                QJSValue::GenericError,
                QString("Couldn't load %1").arg(fileName));
}

void VymModelWrapper::addMapInsert(const QString &fileName, int pos)
{
    addMapInsert(fileName, pos, 0x0000);
}

void VymModelWrapper::addMapInsert(const QString &fileName)
{
    addMapInsert(fileName, -1, 0x0000);
}

void VymModelWrapper::addMapReplace(QString fileName)
{
    if (QDir::isRelativePath(fileName))
        fileName = QDir::currentPath() + "/" + fileName;

    model->saveStateBeforeLoad(File::ImportReplace, fileName);

    if (File::Aborted == model->loadMap(fileName, File::ImportReplace, File::VymMap))
        scriptEngine->throwError(QJSValue::GenericError,
                 QString("Couldn't load %1").arg(fileName));
}

void VymModelWrapper::addSlide() { model->addSlide(); }

void VymModelWrapper::addXLink(const QString &begin, const QString &end,
                               int width, const QString &color,
                               const QString &penstyle)
{
    BranchItem *bbegin = (BranchItem *)(model->findBySelectString(begin));
    BranchItem *bend = (BranchItem *)(model->findBySelectString(end));
    if (bbegin && bend) {
        if (bbegin->hasTypeBranch() && bend->hasTypeBranch()) {
            Link *li = new Link(model);
            li->setBeginBranch((BranchItem *)bbegin);
            li->setEndBranch((BranchItem *)bend);

            model->createLink(li);
            QPen pen = li->getPen();
            if (width > 0)
                pen.setWidth(width);
            QColor col(color);
            if (col.isValid())
                pen.setColor(col);
            else {
                scriptEngine->throwError(
                        QJSValue::GenericError,
                        QString("Could not set color to %1").arg(color));
                return;
            }

            bool ok;
            Qt::PenStyle st1 = penStyle(penstyle, ok);
            if (ok) {
                pen.setStyle(st1);
                li->setPen(pen);
            }
            else
            scriptEngine->throwError(
                    QJSValue::GenericError,
                    QString("Couldn't set penstyle %1").arg(penstyle));
            return;
        }
        else
            scriptEngine->throwError(
                    QJSValue::GenericError,
                    "Begin or end of xLink are not branch or mapcenter");
    }
    else
        scriptEngine->throwError(
            QJSValue::GenericError,
            "Begin or end of xLink not found");
}

int VymModelWrapper::branchCount()
{
    int r;
    BranchItem *selbi = model->getSelectedBranch();
    if (selbi)
        r = selbi->branchCount();
    else {
        scriptEngine->throwError(
                QJSValue::RangeError,
                "No branch selected");
        r = -1;
    }
    return setResult(r);
}

int VymModelWrapper::centerCount()
{
    int r = model->centerCount();
    return setResult(r);
}

void VymModelWrapper::centerOnID(const QString &id)
{
    if (!model->centerOnID(id))
        scriptEngine->throwError(
                QJSValue::GenericError,
                QString("Could not center on ID %1").arg(id));
}

void VymModelWrapper::clearFlags() { model->clearFlags(); }

void VymModelWrapper::colorBranch(const QString &color)
{
    QColor col(color);
    if (!col.isValid())
        scriptEngine->throwError(
                QJSValue::GenericError,
                QString("Could not set color to %1").arg(color));
    else
        model->colorBranch(col);
}

void VymModelWrapper::colorSubtree(const QString &color)
{
    QColor col(color);
    if (!col.isValid())
        scriptEngine->throwError(
                QJSValue::GenericError,
                QString("Could not set color to %1").arg(color));
    else
        model->colorSubtree(col);
}

void VymModelWrapper::copy() { model->copy(); }

void VymModelWrapper::cut() { model->cut(); }

void VymModelWrapper::cycleTask()
{
    if (!model->cycleTaskStatus())
        scriptEngine->throwError(
                QJSValue::GenericError,
                "Couldn't cycle task status");
}

int VymModelWrapper::depth()
{
    TreeItem *selti = model->getSelectedItem();
    if (selti)
        return setResult(selti->depth());

    scriptEngine->throwError(
            QJSValue::GenericError,
            "Nothing selected");
    return -1;
}

void VymModelWrapper::detach()
{
    model->detach();
}

bool VymModelWrapper::exportMap(QJSValueList args)
{
    int argumentsCount = args.count();

    bool r = false;

    if (argumentsCount == 0) {
        scriptEngine->throwError(
                QJSValue::GenericError,
                "Not enough arguments");
        return setResult(r);
    }

    QString format;
    format = args[0].toString();

    if (argumentsCount == 1) {
        if (format == "Last") {
            model->exportLast();
            r = true;
        }
        else
            scriptEngine->throwError(
                    QJSValue::GenericError,
                    "Filename missing");
        return setResult(r);
    }

    QString filename;

    filename = args[1].toString();

    if (format == "AO") {
        model->exportAO(filename, false);
    }
    else if (format == "ASCII") {
        bool listTasks = false;
        if (argumentsCount == 3 && args[2].toString() == "true")
            listTasks = true;
        model->exportASCII(filename, listTasks, false);
    }
    else if (format == "ConfluenceNewPage") {
        // 0: General export format
        // 1: URL of parent page (required)
        // 2: page title (required)
        if (argumentsCount < 3) {
            scriptEngine->throwError(
                    QJSValue::GenericError,
                    QString("Confluence export new page: Only %1 instead of 3 parameters given")
                     .arg(argumentsCount));
            return setResult(r);
        }

        QString url = args[2].toString();
        QString pageName = args[3].toString();

        model->exportConfluence(true, url, pageName, false);
    }
    else if (format == "ConfluenceUpdatePage") {
        // 0: General export format
        // 1: URL of  page to be updated
        // 2: page title (optional)
        if (argumentsCount == 1) {
            scriptEngine->throwError(
                    QJSValue::GenericError,
                    "URL of new page missing in Confluence export");
            return setResult(r);
        }
        QString url = args[1].toString();

        QString title = "";
        if (argumentsCount == 3) {
            title = args[2].toString();
        }

        model->exportConfluence(false, url, title, false);
    }
    else if (format == "CSV") {
        model->exportCSV(filename, false);
    }
    else if (format == "HTML") {
        if (argumentsCount < 3) {
            scriptEngine->throwError(
                    QJSValue::GenericError,
                    "Path missing in HTML export");
            return setResult(r);
        }
        QString dpath = args[2].toString();
        model->exportHTML(filename, dpath, false);
    }
    else if (format == "Image") {
        QString imgFormat;
        if (argumentsCount == 2)
            imgFormat = "PNG";
        else if (argumentsCount == 3)
            imgFormat = args[2].toString();

        QStringList formats;
        formats << "PNG";
        formats << "GIF";
        formats << "JPG";
        formats << "JPEG", formats << "PNG", formats << "PBM", formats << "PGM",
            formats << "PPM", formats << "TIFF", formats << "XBM",
            formats << "XPM";
        if (formats.indexOf(imgFormat) < 0) {
            scriptEngine->throwError(
                    QJSValue::GenericError,
                    QString("%1 not one of the known export formats: ")
                         .arg(imgFormat)
                         .arg(formats.join(",")));
            return setResult(r);
        }
        model->exportImage(filename, false, imgFormat);
    }
    else if (format == "Impress") {
        if (argumentsCount < 3) {
            scriptEngine->throwError(
                    QJSValue::GenericError,
                    "Template file  missing in export to Impress");
            return setResult(r);
        }
        QString templ = args[2].toString();
        model->exportImpress(filename, templ);
    }
    else if (format == "LaTeX") {
        model->exportLaTeX(filename, false);
    }
    else if (format == "Markdown") {
        model->exportMarkdown(filename, false);
    }
    else if (format == "OrgMode") {
        model->exportOrgMode(filename, false);
    }
    else if (format == "PDF") {
        model->exportPDF(filename, false);
    }
    else if (format == "SVG") {
        model->exportSVG(filename, false);
    }
    else if (format == "XML") {
        if (argumentsCount < 3) {
            scriptEngine->throwError(
                    QJSValue::GenericError,
                    "path missing in export to Impress");
            return setResult(r);
        }
        QString dpath = args[2].toString();
        model->exportXML(filename, dpath, false);
    }
    else {
        scriptEngine->throwError(
                QJSValue::GenericError,
                QString("Unknown export format: %1").arg(format));
        return setResult(r);
    }
    return setResult(true);
}

BranchWrapper* VymModelWrapper::findBranchByAttribute(
        const QString &key,
        const QString &value)
{
    BranchItem *bi = model->findBranchByAttribute(key, value);
    if (bi)
        return bi->branchWrapper();
    else
        return nullptr;
}

BranchWrapper* VymModelWrapper::findBranchById(const QString &u)
{
    TreeItem *ti = model->findUuid(QUuid(u));
    if (ti && ti->hasTypeBranch())
        return ((BranchItem*)ti)->branchWrapper();
    else
        return nullptr;
}

ImageWrapper* VymModelWrapper::findImageById(const QString &u)
{
    TreeItem *ti = model->findUuid(QUuid(u));
    if (ti && ti->hasTypeImage())
        return ((ImageItem*)ti)->imageWrapper();
    else
        return nullptr;
}

int VymModelWrapper::getBranchIndex()
{
    int r;
    BranchItem *selbi = model->getSelectedBranch();
    if (selbi) {
        r = selbi->num();
    } else {
        scriptEngine->throwError(
                QJSValue::RangeError,
                "No branch selected");
        r = -1;
    }
    return setResult(r);
}

QString VymModelWrapper::getDestPath()
{
    QString r = model->getDestPath();
    return setResult(r);
}

QString VymModelWrapper::getFileDir() { return setResult(model->getFileDir()); }

QString VymModelWrapper::getFileName()
{
    return setResult(model->getFileName());
}

QString VymModelWrapper::getFrameType(const bool &useInnerFrame)
{
    QString r;
    BranchItem *selbi = model->getSelectedBranch();
    if (selbi) {
        r = selbi->getBranchContainer()->frameTypeString(useInnerFrame);
    }
    return setResult(r);
}

QString VymModelWrapper::getHeadingPlainText()
{
    QString r = model->getHeading().getTextASCII();
    return setResult(r);
}

QString VymModelWrapper::getHeadingXML()
{
    QString r = model->getHeading().saveToDir();
    return setResult(r);
}

QString VymModelWrapper::getAuthor()
{
    return setResult(model->getAuthor());
}

QString VymModelWrapper::getComment()
{
    return setResult(model->getComment());
}

QString VymModelWrapper::getTitle() { return setResult(model->getTitle()); }

QString VymModelWrapper::getNotePlainText()
{
    return setResult(model->getNote().getTextASCII());
}

QString VymModelWrapper::getNoteXML()
{
    return setResult(model->getNote().saveToDir());
}

qreal VymModelWrapper::getPosX()
{
    Container *c = nullptr;
    BranchItem *selbi = model->getSelectedBranch();
    if (selbi)
        c = (Container*)(selbi->getBranchContainer());
    else {
        ImageItem *selii = model->getSelectedImage();
        if (selii)
            c = (Container*)(selii->getImageContainer());
            scriptEngine->throwError(
                    QJSValue::RangeError,
                    "No branch selected");
    }

    if (c)
        return setResult(c->pos().x());

    scriptEngine->throwError(
         QJSValue::GenericError,
         "Could not get x-position from item");
    return 0;
}

qreal VymModelWrapper::getPosY()
{
    Container *c = nullptr;
    BranchItem *selbi = model->getSelectedBranch();
    if (selbi)
        c = (Container*)(selbi->getBranchContainer());
    else {
        ImageItem *selii = model->getSelectedImage();
        if (selii)
            c = (Container*)(selii->getImageContainer());
    }

    if (c)
        return setResult(c->pos().y());

    scriptEngine->throwError(
         QJSValue::GenericError,
         "Could not get y-position from item");
    return 0;
}

qreal VymModelWrapper::getScenePosX()
{
    Container *c = nullptr;
    BranchItem *selbi = model->getSelectedBranch();
    if (selbi)
        c = (Container*)(selbi->getBranchContainer());
    else {
        ImageItem *selii = model->getSelectedImage();
        if (selii)
            c = (Container*)(selii->getImageContainer());
    }

    if (c)
        return setResult(c->scenePos().x());

    scriptEngine->throwError(
            QJSValue::GenericError,
            "Could not get scenePos.x() from item");
    return 0;
}

qreal VymModelWrapper::getScenePosY()
{
    Container *c = nullptr;
    BranchItem *selbi = model->getSelectedBranch();
    if (selbi)
        c = (Container*)(selbi->getBranchContainer());
    else {
        ImageItem *selii = model->getSelectedImage();
        if (selii)
            c = (Container*)(selii->getImageContainer());
    }

    if (c)
        return setResult(c->scenePos().y());

    scriptEngine->throwError(
            QJSValue::GenericError,
            "Could not get scenePos.y() from item");
    return 0;
}

int VymModelWrapper::getRotationHeading()
{
    int r = -1;
    BranchItem *selbi = model->getSelectedBranch();
    if (selbi)
        r = selbi->getBranchContainer()->rotationHeading();
    else
        scriptEngine->throwError(QJSValue::RangeError, QString("No branch selected"));
    return setResult(r);
}

int VymModelWrapper::getRotationSubtree()
{
    int r = -1;
    BranchItem *selbi = model->getSelectedBranch();
    if (selbi)
        r = selbi->getBranchContainer()->rotationSubtree();
    else
        scriptEngine->throwError(QJSValue::RangeError, QString("No branch selected"));
    return setResult(r);
}

QString VymModelWrapper::getSelectionString()
{
    return setResult(model->getSelectString());
}

int VymModelWrapper::getTaskPriorityDelta()
{
    return model->getTaskPriorityDelta();
}

QString VymModelWrapper::getTaskSleep()
{
    QString r;
    BranchItem *selbi = model->getSelectedBranch();
    if (selbi) {
        Task *task = selbi->getTask();
        if (task)
            r = task->getSleep().toString(Qt::ISODate);
        else
            scriptEngine->throwError(
                    QJSValue::GenericError,
                    "Branch has no task");
    } else
        scriptEngine->throwError(QJSValue::RangeError, QString("No branch selected"));
    return setResult(r);
}

int VymModelWrapper::getTaskSleepDays()
{
    int r = -1;
    BranchItem *selbi = model->getSelectedBranch();
    if (selbi) {
        Task *task = selbi->getTask();
        if (task)
            r = task->getDaysSleep();
        else
            //logErrorOld(context(), QScriptContext::UnknownError,
        scriptEngine->throwError(QJSValue::GenericError,
                     "Branch has no task");
    } else
        scriptEngine->throwError(QJSValue::RangeError, QString("No branch selected"));
    return setResult(r);
}

QString VymModelWrapper::getXLinkColor()
{
    return setResult(model->getXLinkColor().name());
}

int VymModelWrapper::getXLinkWidth()
{
    return setResult(model->getXLinkWidth());
}

QString VymModelWrapper::getXLinkPenStyle()
{
    QString r = penStyleToString(model->getXLinkStyle());
    return setResult(r);
}

QString VymModelWrapper::getXLinkStyleBegin()
{
    return setResult(model->getXLinkStyleBegin());
}

QString VymModelWrapper::getXLinkStyleEnd()
{
    return setResult(model->getXLinkStyleEnd());
}

bool VymModelWrapper::hasActiveFlag(const QString &flag)
{
    bool r = false;
    BranchItem *selbi = model->getSelectedBranch();
    if (selbi)
        r = selbi->hasActiveFlag(flag);
    else
        scriptEngine->throwError(QJSValue::RangeError, QString("No branch selected"));
    return setResult(r);
}

bool VymModelWrapper::hasNote()
{
    bool r = !model->getNote().isEmpty();
    return setResult(r);
}

bool VymModelWrapper::hasRichTextNote()
{
    return setResult(model->hasRichTextNote());
}

bool VymModelWrapper::hasTask()
{
    bool r = false;
    BranchItem *selbi = model->getSelectedBranch();
    if (selbi) {
        Task *task = selbi->getTask();
        if (task)
            r = true;
    } else
        scriptEngine->throwError(QJSValue::RangeError, QString("No branch selected"));

    return setResult(r);
}

void VymModelWrapper::importDir(const QString &path)
{
    model->importDir(
        path); // FIXME-3 error handling missing (in vymmodel and here)
}

void VymModelWrapper::newBranchIterator(const QString &itname, bool deepLevelsFirst)
{
    model->newBranchIterator(itname, deepLevelsFirst);
}

BranchWrapper* VymModelWrapper::nextBranch(const QString &itname)
{
    BranchItem *bi = model->nextBranchIterator(itname);
    if (bi)
        return bi->branchWrapper();
    else
        return nullptr;
}

bool VymModelWrapper::isScrolled()
{
    bool r = false;
    BranchItem *selbi = model->getSelectedBranch();
    if (selbi)
        r = selbi->isScrolled();
    else
        scriptEngine->throwError(QJSValue::RangeError, QString("No branch selected"));
    return setResult(r);
}

void VymModelWrapper::loadImage(const QString &filename)
{
    BranchItem *selbi = model->getSelectedBranch();
    if (selbi) {
        model->loadImage(
            selbi,
            filename); // FIXME-3 error handling missing (in vymmodel and here)
    } else
        scriptEngine->throwError(QJSValue::RangeError, QString("No branch selected"));
}

void VymModelWrapper::loadNote(const QString &filename)
{
    model->loadNote(
        filename); // FIXME-3 error handling missing (in vymmodel and here)
}

void VymModelWrapper::moveSlideDown(int n)
{
    if (!model->moveSlideDown(n))
        scriptEngine->throwError(
                QJSValue::GenericError,
                "Could not move slide down");
}

void VymModelWrapper::moveSlideDown() { moveSlideDown(-1); }

void VymModelWrapper::moveSlideUp(int n)
{
    if (!model->moveSlideUp(n))
        scriptEngine->throwError(
                QJSValue::GenericError,
                "Could not move slide up");
}

void VymModelWrapper::moveSlideUp() { moveSlideUp(-1); }

void VymModelWrapper::note2URLs() { model->note2URLs(); }

void VymModelWrapper::paste() { model->paste(); }

void VymModelWrapper::redo() { model->redo(); }

bool VymModelWrapper::relinkTo(const QString &parent, int num)
{
    bool r;
    r = model->relinkTo(parent, num);
    if (!r)
        scriptEngine->throwError(QJSValue::GenericError,"Could not relink");
    return setResult(r);
}

bool VymModelWrapper::relinkTo(const QString &parent)
{
    bool r = relinkTo(parent, -1);
    return setResult(r);
}

void VymModelWrapper::remove() { model->deleteSelection(); }

void VymModelWrapper::removeChildren() { model->deleteChildren(); }

void VymModelWrapper::removeChildBranches() { model->deleteChildBranches(); }

void VymModelWrapper::removeKeepChildren() { model->deleteKeepChildren(); }

void VymModelWrapper::removeSlide(int n)
{
    if (n < 0 || n >= model->slideCount() - 1)
        scriptEngine->throwError(
                QJSValue::GenericError,
                QString("Slide '%1' not available.").arg(n));
}

QVariant VymModelWrapper::repeatLastCommand()
{
    return model->repeatLastCommand();
}

void VymModelWrapper::saveImage(const QString &filename)
{
    model->saveImage(nullptr, filename);
}

void VymModelWrapper::saveNote(const QString &filename)
{
    model->saveNote(filename);
}

void VymModelWrapper::saveSelection(const QString &filename)
{
    QString filename_org = model->getFilePath(); // Restore filename later
    if (!model->renameMap(filename)) {
        QString s = tr("Saving the selection in map failed:\nCouldn't rename map to %1").arg(filename);
        QMessageBox::critical(0,
            tr("Critical Error"), s);
        scriptEngine->throwError(QJSValue::GenericError, s);
        return;
    }
    model->saveMap(File::PartOfMap);
    model->renameMap(filename_org); // FIXME-0 check if this is ok, renaming map and data 2 times...
}

void VymModelWrapper::scroll()
{
    BranchItem *selbi = model->getSelectedBranch();
    if (selbi) {
        if (!model->scrollBranch(selbi))
            scriptEngine->throwError(
                    QJSValue::GenericError,
                    "Couldn't scroll branch");
    }
}

bool VymModelWrapper::select(const QString &s)
{
    bool r = model->select(s);
    if (!r)
        scriptEngine->throwError(
                QJSValue::GenericError,
                QString("Couldn't select %1").arg(s));
    return setResult(r);
}

BranchWrapper* VymModelWrapper::selectedBranch()
{
    BranchItem *selbi = model->getSelectedBranch();

    if (selbi)
        return selbi->branchWrapper();
    else
        return nullptr; // caught by QJSEngine
}

bool VymModelWrapper::selectID(const QString &s)
{
    bool r = model->selectID(s);
    if (!r)
        scriptEngine->throwError(
                QJSValue::GenericError,
                QString("Couldn't select ID %1").arg(s));
    return setResult(r);
}

bool VymModelWrapper::selectFirstChildBranch()
{
    bool r = false;
    BranchItem *selbi = model->getSelectedBranch();
    if (selbi) {
        r = model->selectFirstChildBranch();
        if (!r)
            scriptEngine->throwError(
                    QJSValue::GenericError,
                    "Couldn't select first child branch");
    }
    return setResult(r);
}

bool VymModelWrapper::selectLastChildBranch()
{
    bool r = false;
    BranchItem *selbi = model->getSelectedBranch();
    if (selbi) {
        r = model->selectLastChildBranch();
        if (!r)
            scriptEngine->throwError(
                    QJSValue::GenericError,
                    "Couldn't select last child branch");
    }
    return setResult(r);
}
bool VymModelWrapper::selectLastImage()
{
    bool r = false;
    BranchItem *selbi = model->getSelectedBranch();
    if (selbi) {
        ImageItem *ii = selbi->getLastImage();
        if (!ii)
            scriptEngine->throwError(
                    QJSValue::GenericError,
                    "Couldn't get last image");
        else {
            r = model->select(ii);
            if (!r)
                scriptEngine->throwError(
                        QJSValue::GenericError,
                        "Couldn't select last image");
        }
    }
    return setResult(r);
}

bool VymModelWrapper::selectLatestAdded()
{
    bool r = model->selectLatestAdded();
    if (!r)
        scriptEngine->throwError(
                QJSValue::GenericError,
                "Couldn't select latest added item");
    return setResult(r);
}

bool VymModelWrapper::selectToggle(const QString &selectString)
{
    bool r = model->selectToggle(selectString);
    if (!r)
        scriptEngine->throwError(
                QJSValue::GenericError,
                QString("Couldn't toggle item with select string \"%1\"").arg(selectString));
    return setResult(r);
}

bool VymModelWrapper::selectXLink(int n)
{
    bool r = false;
    BranchItem *selbi = model->getSelectedBranch();
    if (selbi) {
        XLinkItem *xli = selbi->getXLinkItemNum(n);
        if (!xli)
            scriptEngine->throwError(QJSValue::RangeError,
                 QString("Selected branch has no xlink with index %1").arg(n));
        else
            r = model->select((TreeItem*)xli);
    } else
        scriptEngine->throwError(QJSValue::RangeError, QString("No branch selected"));
    return setResult(r);
}

bool VymModelWrapper::selectXLinkOtherEnd(int n)
{
    bool r = false;
    BranchItem *selbi = model->getSelectedBranch();
    if (selbi) {
        XLinkItem *xli = selbi->getXLinkItemNum(n);
        if (!xli) {
            scriptEngine->throwError(
                    QJSValue::RangeError,
                    QString("Selected branch has no xlink with index %1").arg(n));
        } else {
            BranchItem *bi = xli->getPartnerBranch();
            if (!bi) {
                scriptEngine->throwError(
                        QJSValue::RangeError,
                        "Selected xlink has no other end ?!");
            } else
                r = model->select(bi);
        }
    } else
        scriptEngine->throwError(QJSValue::RangeError, QString("No branch selected"));

    return setResult(r);
}

void VymModelWrapper::setDefaultLinkColor(const QString &color)
{
    QColor col(color);
    if (col.isValid()) {
        model->setDefaultLinkColor(col);
    }
    else
        scriptEngine->throwError(
                QJSValue::GenericError,
                QString("Could not set color to %1").arg(color));
}

void VymModelWrapper::setFlagByName(const QString &s)
{
    model->setFlagByName(s);
}

void VymModelWrapper::setHeadingConfluencePageName()
{
    model->setHeadingConfluencePageName();
}

void VymModelWrapper::setHeadingPlainText(
    const QString &text)
{
    model->setHeadingPlainText(text);
}

void VymModelWrapper::setHideExport(bool b) { model->setHideExport(b); }

void VymModelWrapper::setHideLinkUnselected(bool b)
{
    model->setHideLinkUnselected(b);
}

void VymModelWrapper::setMapAnimCurve(int n)
{
    if (n < 0 || n > QEasingCurve::OutInBounce)
        scriptEngine->throwError(
                QJSValue::RangeError,
                QString("Unknown animation curve type: ").arg(n));
    else {
        QEasingCurve c;
        c.setType((QEasingCurve::Type)n);
        model->setMapAnimCurve(c);
    }
}

void VymModelWrapper::setMapAnimDuration(int n)
{
    model->setMapAnimDuration(n);
}

void VymModelWrapper::setAuthor(const QString &s) { model->setAuthor(s); }

void VymModelWrapper::setMapBackgroundColor(const QString &color)
{
    QColor col(color);
    if (col.isValid()) {
        model->setBackgroundColor(col);
    }
    else
        scriptEngine->throwError(
                QJSValue::GenericError,
                QString("Could not set color to %1").arg(color));
}

void VymModelWrapper::setComment(const QString &s) { model->setComment(s); }

void VymModelWrapper::setMapLinkStyle(const QString &style)
{
    if (!model->setLinkStyle(style))
        scriptEngine->throwError(
                QJSValue::GenericError,
                QString("Could not set linkstyle to %1").arg(style));
}

void VymModelWrapper::setMapRotation(float a) { model->setMapRotation(a); }

void VymModelWrapper::setTitle(const QString &s) { model->setTitle(s); }

void VymModelWrapper::setMapZoom(float z) { model->setMapZoomFactor(z); }

void VymModelWrapper::setNotePlainText(const QString &s)
{
    VymNote vn;
    vn.setPlainText(s);
    model->setNote(vn);
}

void VymModelWrapper::setFramePenWidth(const bool &useInnerFrame, int width)
{
    model->setFramePenWidth(useInnerFrame, width);
}

void VymModelWrapper::setFrameBrushColor(const bool &useInnerFrame, const QString &color)
{
    model->setFrameBrushColor(useInnerFrame, color);
}

void VymModelWrapper::setFramePadding(const bool &useInnerFrame, int padding)
{
    model->setFramePadding(useInnerFrame, padding);
}

void VymModelWrapper::setFramePenColor(const bool &useInnerFrame, const QString &color)
{
    model->setFramePenColor(useInnerFrame, color);
}

void VymModelWrapper::setFrameType(const bool &useInnerFrame, const QString &type)
{
    model->setFrameType(useInnerFrame, type);
}

void VymModelWrapper::setRotationHeading(const int &i)
{
    model->setRotationHeading(i);
}

void VymModelWrapper::setRotationSubtree(const int &i)
{
    model->setRotationSubtree(i);
}

void VymModelWrapper::setRotationsAutoDesign(const bool b)
{
    model->setRotationsAutoDesign(b);
}

void VymModelWrapper::setScale(qreal f) { model->setScale(f, false); }

void VymModelWrapper::setScaleSubtree(qreal f) { model->setScaleSubtree(f); }

void VymModelWrapper::setScalingAutoDesign(const bool b)
{
    model->setScaleAutoDesign(b);
}

void VymModelWrapper::setSelectionBrushColor(const QString &color)
{
    QColor col(color);
    if (!col.isValid())
        //logErrorOld(context(), QScriptContext::SyntaxError,
        scriptEngine->throwError(
                QJSValue::GenericError,
                QString("Could not set color to %1").arg(color));
    else
        model->setSelectionBrushColor(col);
}

void VymModelWrapper::setSelectionPenColor(const QString &color)
{
    QColor col(color);
    if (!col.isValid())
        scriptEngine->throwError(
                QJSValue::GenericError,
                QString("Could not set color to %1").arg(color));
    else
        model->setSelectionPenColor(col);
}

void VymModelWrapper::setSelectionPenWidth(const qreal &w)
{
    model->setSelectionPenWidth(w);
}

void VymModelWrapper::setTaskPriorityDelta(const int &n)
{
    model->setTaskPriorityDelta(n);
}

bool VymModelWrapper::setTaskSleep(const QString &s)
{
    bool r = model->setTaskSleep(s);
    return setResult(r);
}

void VymModelWrapper::setXLinkColor(const QString &color)
{
    QColor col(color);
    if (!col.isValid())
        scriptEngine->throwError(
                QJSValue::GenericError,
                QString("Could not set color to %1").arg(color));
    else
        model->setXLinkColor(color); // FIXME-3 try to use QColor here...
}

void VymModelWrapper::setXLinkStyle(const QString &style)
{
    model->setXLinkStyle(style);
}

void VymModelWrapper::setXLinkStyleBegin(const QString &style)
{
    model->setXLinkStyleBegin(style);
}

void VymModelWrapper::setXLinkStyleEnd(const QString &style)
{
    model->setXLinkStyleEnd(style);
}

void VymModelWrapper::setXLinkWidth(int w) { model->setXLinkWidth(w); }

void VymModelWrapper::sleep(int n)
{
    // FIXME-5 sleep is not avail on windows VCEE, workaround could be using
    // this->thread()->wait(x ms)
    sleep(n);
}

int VymModelWrapper::slideCount()
{
    return setResult(model->slideCount());
}

void VymModelWrapper::sortChildren(bool b) { model->sortChildren(b); }

void VymModelWrapper::sortChildren() { sortChildren(false); }

void VymModelWrapper::toggleFlagByName(const QString &s)
{
    model->toggleFlagByName(s);
}

void VymModelWrapper::toggleScroll() { model->toggleScroll(); }

void VymModelWrapper::toggleTarget() { model->toggleTarget(); }

void VymModelWrapper::toggleTask() { model->toggleTask(); }

void VymModelWrapper::undo() { model->undo(); }

bool VymModelWrapper::unscroll()
{
    bool r = false;
    BranchItem *selbi = model->getSelectedBranch();
    if (selbi) {
        r = model->unscrollBranch(selbi);
        if (!r)
            scriptEngine->throwError(
                    QJSValue::GenericError,
                    "Couldn't unscroll branch");
    }
    return setResult(r);
}

void VymModelWrapper::unscrollChildren() { model->unscrollChildren(); }

void VymModelWrapper::unselectAll() { model->unselectAll(); }

void VymModelWrapper::unsetFlagByName(const QString &s)
{
    model->unsetFlagByName(s);
}

int VymModelWrapper::xlinkCount()
{
    int r;
    BranchItem *selbi = model->getSelectedBranch();
    if (selbi) {
        r = selbi->xlinkCount();
    } else {
        scriptEngine->throwError(
                QJSValue::GenericError,
                "Selected item is not a branch");
        r = -1;
    }

    return setResult(r);
}
