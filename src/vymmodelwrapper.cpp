#include "vymmodelwrapper.h"

#include <QMessageBox>

#include "attributeitem.h"
#include "attribute-wrapper.h"
#include "branchitem.h"
#include "branch-container.h"
#include "branch-wrapper.h"
#include "imageitem.h"
#include "image-wrapper.h"
#include "mainwindow.h"
#include "misc.h"
#include "vym-wrapper.h"
#include "scripting-xlink-wrapper.h"
#include "vymmodel.h"
#include "vymtext.h"
#include "xlink.h"
#include "xlinkitem.h"
#include "xmlobj.h" // include quoteQuotes

extern Main *mainWindow;

///////////////////////////////////////////////////////////////////////////
VymModelWrapper::VymModelWrapper(VymModel *m) { model = m; }

void VymModelWrapper::addMapCenterAtPos(qreal x, qreal y)
{
    if (!model->addMapCenterAtPos(QPointF(x, y)))
        mainWindow->abortScript(
                QJSValue::GenericError,
                "Couldn't add mapcenter");
}

void VymModelWrapper::addSlide() { model->addSlide(); }

int VymModelWrapper::centerCount()
{
    int r = model->centerCount();
    mainWindow->setScriptResult(r);
    return r;
}

void VymModelWrapper::centerOnID(const QString &id)
{
    if (!model->centerOnID(id))
        mainWindow->abortScript(
                QJSValue::GenericError,
                QString("Could not center on ID %1").arg(id));
}

void VymModelWrapper::copy() { model->copy(); }

void VymModelWrapper::cut() { model->cut(); }

int VymModelWrapper::depth()
{
    TreeItem *selti = model->getSelectedItem();
    if (selti) {
	int r = selti->depth();
	mainWindow->setScriptResult(r);
	return r;
    }

    mainWindow->abortScript(
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
        mainWindow->abortScript(
                QJSValue::GenericError,
                "Not enough arguments");
        mainWindow->setScriptResult(r);
	return r;
    }

    QString format;
    format = args[0].toString();

    if (argumentsCount == 1) {
        if (format == "Last") {
            model->exportLast();
            r = true;
        } else
            mainWindow->abortScript(
                    QJSValue::GenericError,
                    "Filename missing");
        mainWindow->setScriptResult(r);
	return r;
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
            mainWindow->abortScript(
                    QJSValue::GenericError,
                    QString("Confluence export new page: Only %1 instead of 3 parameters given")
                     .arg(argumentsCount));
            mainWindow->setScriptResult(r);
	    return r;
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
            mainWindow->abortScript(
                    QJSValue::GenericError,
                    "URL of new page missing in Confluence export");
            mainWindow->setScriptResult(r);
	    return r;
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
            mainWindow->abortScript(
                    QJSValue::GenericError,
                    "Path missing in HTML export");
            mainWindow->setScriptResult(r);
	    return r;
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
            mainWindow->abortScript(
                    QJSValue::GenericError,
                    QString("%1 not one of the known export formats: ")
                         .arg(imgFormat)
                         .arg(formats.join(",")));
            mainWindow->setScriptResult(r);
	    return r;
        }
        model->exportImage(filename, false, imgFormat);
    }
    else if (format == "Impress") {
        if (argumentsCount < 3) {
            mainWindow->abortScript(
                    QJSValue::GenericError,
                    "Template file  missing in export to Impress");
            mainWindow->setScriptResult(r);
	    return r;
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
            mainWindow->abortScript(
                    QJSValue::GenericError,
                    "path missing in export to Impress");
            mainWindow->setScriptResult(r);
	    return r;
        }
        QString dpath = args[2].toString();
        model->exportXML(filename, dpath, false);
    }
    else {
        mainWindow->abortScript(
                QJSValue::GenericError,
                QString("Unknown export format: %1").arg(format));
        mainWindow->setScriptResult(r);
	return r;
    }
    mainWindow->setScriptResult(r);
    return r;
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

AttributeWrapper* VymModelWrapper::findAttributeById(const QString &u)
{
    TreeItem *ti = model->findUuid(QUuid(u));
    if (ti && ti->hasTypeAttribute())
        return ((AttributeItem*)ti)->attributeWrapper();
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

BranchWrapper* VymModelWrapper::findBranchBySelection(const QString &s)
{
    TreeItem *ti = model->findBySelectString(s);
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

ImageWrapper* VymModelWrapper::findImageBySelection(const QString &s)
{
    TreeItem *ti = model->findBySelectString(s);
    if (ti && ti->hasTypeImage())
        return ((ImageItem*)ti)->imageWrapper();
    else
        return nullptr;
}

XLinkWrapper* VymModelWrapper::findXLinkById(const QString &u)
{
    TreeItem *ti = model->findUuid(QUuid(u));
    if (ti && ti->hasTypeXLink())
        return ((XLinkItem*)ti)->getXLink()->xlinkWrapper();
    else
        return nullptr;
}

QString VymModelWrapper::getDestPath()
{
    QString r = model->getDestPath();
    mainWindow->setScriptResult(r);
    return r;
}

QString VymModelWrapper::getFileDir()
{
    QString r = model->getFileDir();
    mainWindow->setScriptResult(r);
    return r;
}

QString VymModelWrapper::getFileName()
{
    QString r = model->getFileName();
    mainWindow->setScriptResult(r);
    return r;
}

QString VymModelWrapper::getHeadingXML()
{
    QString r = model->getHeading().saveToDir();
    mainWindow->setScriptResult(r);
    return r;
}

QString VymModelWrapper::getAuthor()
{
    QString r = model->getAuthor();
    mainWindow->setScriptResult(r);
    return r;
}

QString VymModelWrapper::getComment()
{
    QString r = model->getComment();
    mainWindow->setScriptResult(r);
    return r;
}

QString VymModelWrapper::getTitle()
{
    QString r = model->getTitle();
    mainWindow->setScriptResult(r);
    return r;
}

QString VymModelWrapper::getNotePlainText()
{
    QString r = model->getNote().getTextASCII();
    mainWindow->setScriptResult(r);
    return r;
}

QString VymModelWrapper::getNoteXML()
{
    QString r = model->getNote().saveToDir();
    mainWindow->setScriptResult(r);
    return r;
}

int VymModelWrapper::getRotationHeading()
{
    int r = -1;
    BranchItem *selbi = model->getSelectedBranch();
    if (selbi)
        r = selbi->getBranchContainer()->rotationHeading();
    else
        mainWindow->abortScript(QJSValue::RangeError, QString("No branch selected"));
    mainWindow->setScriptResult(r);
    return r;
}

int VymModelWrapper::getRotationSubtree()
{
    int r = -1;
    BranchItem *selbi = model->getSelectedBranch();
    if (selbi)
        r = selbi->getBranchContainer()->rotationSubtree();
    else
        mainWindow->abortScript(QJSValue::RangeError, QString("No branch selected"));
    mainWindow->setScriptResult(r);
    return r;
}

QString VymModelWrapper::getSelectionString()
{
    QString r = model->getSelectString();
    mainWindow->setScriptResult(r);
    return r;
}

bool VymModelWrapper::loadBranchReplace(QString fileName, BranchWrapper *bw)
{
    if (QDir::isRelativePath(fileName))
        fileName = QDir::currentPath() + "/" + fileName;

    bool r = model->addMapReplace(fileName, bw->branchItem());
    mainWindow->setScriptResult(r);
    return r;
}

bool VymModelWrapper::loadDataInsert(QString fileName)
{
    if (QDir::isRelativePath(fileName))
        fileName = QDir::currentPath() + "/" + fileName;

    bool r = model->addMapInsert(fileName);  // FIXME-2 No selectedBranch passed as arg like in replace above?
    mainWindow->setScriptResult(r);
    return r;
}

void VymModelWrapper::newBranchIterator(const QString &itname, bool deepLevelsFirst)
{
    model->newBranchIterator(itname, nullptr, deepLevelsFirst);
}

BranchWrapper* VymModelWrapper::nextBranch(const QString &itname)
{
    BranchItem *bi = model->nextBranchIterator(itname);
    if (bi)
        return bi->branchWrapper();
    else
        return nullptr;
}

void VymModelWrapper::moveSlideDown(int n)
{
    if (!model->moveSlideDown(n))
        mainWindow->abortScript(
                QJSValue::GenericError,
                "Could not move slide down");
}

void VymModelWrapper::moveSlideDown() { moveSlideDown(-1); }

void VymModelWrapper::moveSlideUp(int n)
{
    if (!model->moveSlideUp(n))
        mainWindow->abortScript(
                QJSValue::GenericError,
                "Could not move slide up");
}

void VymModelWrapper::moveSlideUp() { moveSlideUp(-1); }

void VymModelWrapper::note2URLs() { model->note2URLs(); }

void VymModelWrapper::paste() { model->paste(); }

void VymModelWrapper::redo() { model->redo(); }

void VymModelWrapper::remove() { model->deleteSelection(); }

void VymModelWrapper::removeAttribute(AttributeWrapper *aw)
{
    if (!aw) {
        mainWindow->abortScript(
                QJSValue::GenericError,
                "VymModelWrapper::removeAttribute(a) a is invalid");
        return;
    }
    model->deleteSelection(aw->attributeItem()->getID());
}

void VymModelWrapper::removeBranch(BranchWrapper *bw)
{
    if (!bw) {
        mainWindow->abortScript(
                QJSValue::GenericError,
                "VymModelWrapper::removeBranch(b) b is invalid");
        return;
    }
    model->deleteSelection(bw->branchItem()->getID());
}

void VymModelWrapper::removeImage(ImageWrapper *iw)
{
    if (!iw) {
        mainWindow->abortScript(
                QJSValue::GenericError,
                "VymModelWrapper::removeImage(i) i is invalid");
        return;
    }
    model->deleteSelection(iw->imageItem()->getID());
}

void VymModelWrapper::removeKeepChildren(BranchWrapper *bw)
{
    model->deleteKeepChildren(bw->branchItem());
}

void VymModelWrapper::removeSlide(int n)
{
    if (n < 0 || n >= model->slideCount() - 1)
        mainWindow->abortScript(
                QJSValue::GenericError,
                QString("Slide '%1' not available.").arg(n));
}

void VymModelWrapper::removeXLink(XLinkWrapper *xlw)
{
    if (!xlw) {
        mainWindow->abortScript(
                QJSValue::GenericError,
                "VymModelWrapper::removeXLink(xl) xlink is invalid");
        return;
    }
    model->deleteXLink(xlw->xlink());
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
        mainWindow->abortScript(QJSValue::GenericError, s);
        return;
    }
    model->saveMap(File::PartOfMap);
    model->renameMap(filename_org);
}

bool VymModelWrapper::select(const QString &s)
{
    bool r = model->select(s);
    if (!r)
        mainWindow->abortScript(
                QJSValue::GenericError,
                QString("Couldn't select %1").arg(s));
    mainWindow->setScriptResult(r);
    return r;
}

AttributeWrapper* VymModelWrapper::selectedAttribute()
{
    AttributeItem *ai = model->getSelectedAttribute();

    if (ai)
        return ai->attributeWrapper();
    else
        return nullptr;
}

BranchWrapper* VymModelWrapper::selectedBranch()
{
    BranchItem *selbi = model->getSelectedBranch();

    if (selbi)
        return selbi->branchWrapper();
    else
        return nullptr; // caught by QJSEngine
}

XLinkWrapper* VymModelWrapper::selectedXLink()
{
    XLinkItem *xli = model->getSelectedXLinkItem();

    if (xli)
        return xli->getXLink()->xlinkWrapper();
    else
        return nullptr;
}

bool VymModelWrapper::selectUids(QJSValueList args)
{
    int argumentsCount = args.count();

    bool r = false;
    if (argumentsCount == 0) {
        mainWindow->abortScript(
                QJSValue::GenericError,
                "Not enough arguments");
	mainWindow->setScriptResult(r);
	return r;
    }

    QStringList uids;
    foreach (auto arg, args)
        uids << arg.toString();

    r = model->selectUids(uids);
    if (!r)
        mainWindow->abortScript(
                QJSValue::GenericError,
                QString("Couldn't select Uuids: %1").arg(uids.join(",")));
    mainWindow->setScriptResult(r);
    return r;
}

bool VymModelWrapper::selectLatestAdded()
{
    bool r = model->selectLatestAdded();
    if (!r)
        mainWindow->abortScript(
                QJSValue::GenericError,
                "Couldn't select latest added item");
    mainWindow->setScriptResult(r);
    return r;
}

bool VymModelWrapper::selectToggle(const QString &selectString)
{
    bool r = model->selectToggle(selectString);
    if (!r)
        mainWindow->abortScript(
                QJSValue::GenericError,
                QString("Couldn't toggle item with select string \"%1\"").arg(selectString));
    mainWindow->setScriptResult(r);
    return r;
}

void VymModelWrapper::setDefaultLinkColor(const QString &color)
{
    QColor col(color);
    if (col.isValid()) {
        model->setDefaultLinkColor(col);
    }
    else
        mainWindow->abortScript(
                QJSValue::GenericError,
                QString("Could not set color to %1").arg(color));
}

void VymModelWrapper::setHeadingConfluencePageName()
{
    model->setHeadingConfluencePageName();
}

void VymModelWrapper::setHideExport(bool b) { model->setHideExport(b); }

void VymModelWrapper::setHideLinkUnselected(bool b)
{
    model->setHideLinkUnselected(b);
}

void VymModelWrapper::setAnimCurve(int n)
{
    if (n < 0 || n > QEasingCurve::OutInBounce)
        mainWindow->abortScript(
                QJSValue::RangeError,
                QString("Unknown animation curve type: ").arg(n));
    else {
        QEasingCurve c;
        c.setType((QEasingCurve::Type)n);
        model->setMapAnimCurve(c);
    }
}

void VymModelWrapper::setAnimDuration(int n)
{
    model->setMapAnimDuration(n);
}

void VymModelWrapper::setAuthor(const QString &s) { model->setAuthor(s); }

void VymModelWrapper::setBackgroundColor(const QString &color)
{
    QColor col(color);
    if (col.isValid()) {
        model->setBackgroundColor(col);
    }
    else
        mainWindow->abortScript(
                QJSValue::GenericError,
                QString("Could not set color to %1").arg(color));
}

void VymModelWrapper::setComment(const QString &s) { model->setComment(s); }

void VymModelWrapper::setLinkStyle(const QString &style)
{
    if (!model->setLinkStyle(style))
        mainWindow->abortScript(
                QJSValue::GenericError,
                QString("Could not set linkstyle to %1").arg(style));
}

void VymModelWrapper::setRotation(float a) { model->setMapRotation(a); }

void VymModelWrapper::setTitle(const QString &s) { model->setTitle(s); }

void VymModelWrapper::setZoom(float z) { model->setMapZoomFactor(z); }

void VymModelWrapper::setNotePlainText(const QString &s)
{
    VymNote vn;
    vn.setPlainText(s);
    model->setNote(vn);
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
        mainWindow->abortScript(
                QJSValue::GenericError,
                QString("Could not set color to %1").arg(color));
    else
        model->setSelectionBrushColor(col);
}

void VymModelWrapper::setSelectionPenColor(const QString &color)
{
    QColor col(color);
    if (!col.isValid())
        mainWindow->abortScript(
                QJSValue::GenericError,
                QString("Could not set color to %1").arg(color));
    else
        model->setSelectionPenColor(col);
}

void VymModelWrapper::setSelectionPenWidth(const qreal &w)
{
    model->setSelectionPenWidth(w);
}

void VymModelWrapper::sleep(int n)
{
    // FIXME-5 sleep is not avail on windows VCEE, workaround could be using
    // this->thread()->wait(x ms)
    sleep(n);
}

int VymModelWrapper::slideCount()
{
    int r = model->slideCount();
    mainWindow->setScriptResult(r);
    return r;
}

void VymModelWrapper::toggleTarget() { model->toggleTarget(); }

void VymModelWrapper::undo() { model->undo(); }

void VymModelWrapper::unselectAll() { model->unselectAll(); }

