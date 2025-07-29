#include "vymmodelwrapper.h"

#include <QMessageBox>
#include <QQmlEngine>

#include "attributeitem.h"
#include "attribute-wrapper.h"
#include "branchitem.h"
#include "branch-container.h"
#include "branch-wrapper.h"
#include "imageitem.h"
#include "image-wrapper.h"
#include "itemlist-wrapper.h"
#include "mainwindow.h"
#include "mapeditor.h"
#include "misc.h"
#include "vym-wrapper.h"
#include "scripting-xlink-wrapper.h"
#include "vymmodel.h"
#include "xlink.h"
#include "xlinkitem.h"

extern Main *mainWindow;

///////////////////////////////////////////////////////////////////////////
VymModelWrapper::VymModelWrapper(VymModel *m)
{
    //std::cout << "Constr VMWrapper" << this << endl;
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    model = m;
}

VymModelWrapper::~VymModelWrapper()
{
    //std::cout << "Destr VMWrapper" << this << endl;
}

VymModel* VymModelWrapper::getModel()   // FIXME-2  rename model -> modelInt, getModel() -> model()
{
    return model;
}

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

    QString filePath;

    filePath = args[1].toString();

    if (format == "AO") {
        model->exportAO(filePath, false);
    }
    else if (format == "ASCII") {
        bool listTasks = false;
        if (argumentsCount == 3 && args[2].toString() == "true")
            listTasks = true;
        model->exportASCII(filePath, listTasks, false);
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
    } else if (format == "CSV") {
        model->exportCSV(filePath, false);
    } else if (format == "HTML") {
        if (argumentsCount < 3) {
            mainWindow->abortScript(
                    QJSValue::GenericError,
                    "Path missing in HTML export");
            mainWindow->setScriptResult(r);
	    return r;
        }
        QString dpath = args[2].toString();
        model->exportHTML(filePath, dpath, false);
    } else if (format == "Image") {
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
                         .arg(imgFormat, formats.join(",")));
            mainWindow->setScriptResult(r);
	    return r;
        }
        model->exportImage(filePath, false, imgFormat);
    } else if (format == "Impress") {
        if (argumentsCount < 3) {
            mainWindow->abortScript(
                    QJSValue::GenericError,
                    "Template file  missing in export to Impress");
            mainWindow->setScriptResult(r);
	    return r;
        }
        QString templ = args[2].toString();
        model->exportImpress(filePath, templ, false);
    } else if (format == "LaTeX") {
        model->exportLaTeX(filePath, false);
    } else if (format == "Markdown") {
        model->exportMarkdown(filePath, false);
    } else if (format == "OrgMode") {
        model->exportOrgMode(filePath, false);
    } else if (format == "PDF") {
        model->exportPDF(filePath, false);
        r = true;
    } else if (format == "SVG") {
        model->exportSVG(filePath, false);
        r = true;
    } else if (format == "TaskJuggler") {
        model->exportTaskJuggler(filePath, false);
    } else if (format == "XML") {
        model->exportXML(filePath, false);
        r = true;
    } else {
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

QString VymModelWrapper::getBackgroundColor()
{
    QString r = model->backgroundColor().name();
    mainWindow->setScriptResult(r);
    return r;
}

QString VymModelWrapper::getBackgroundImageName()
{
    QString r = model->backgroundImageName();
    mainWindow->setScriptResult(r);
    return r;
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

QString VymModelWrapper::getAuthor()
{
    QString r = model->mapAuthor();
    mainWindow->setScriptResult(r);
    return r;
}

QString VymModelWrapper::getComment()
{
    QString r = model->mapComment();
    mainWindow->setScriptResult(r);
    return r;
}

QString VymModelWrapper::getLinkColorHint()
{
    LinkObj::ColorHint hint = model->mapDesign()->linkColorHint();
    return LinkObj::linkColorHintName(hint);
}

QString VymModelWrapper::getTitle()
{
    QString r = model->mapTitle();
    mainWindow->setScriptResult(r);
    return r;
}

QString VymModelWrapper::getSelectionString()
{
    QString r = model->getSelectString();
    mainWindow->setScriptResult(r);
    return r;
}

double VymModelWrapper::getZoom()
{
    return model->getMapEditor()->zoomFactorTarget();
}

bool VymModelWrapper::hasBackgroundImage()
{
    bool r = model->hasBackgroundImage();
    mainWindow->setScriptResult(r);
    return r;
}

bool VymModelWrapper::loadBackgroundImage(const QString &imagePath)
{
    bool r =model->loadBackgroundImage(imagePath);
    if (!r)
        mainWindow->abortScript(
                QJSValue::GenericError,
                QString("Failed to load background image \"%1\"").arg(imagePath));
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

bool VymModelWrapper::loadDataInsert(QString fileName, int pos, BranchWrapper *bw)
{
    if (QDir::isRelativePath(fileName))
        fileName = QDir::currentPath() + "/" + fileName;

    BranchItem * bi = bw ? bw->branchItem() : nullptr;
    bool r = model->addMapInsert(fileName, pos, bi);
    mainWindow->setScriptResult(r);
    return r;
}

BranchWrapper* VymModelWrapper::resetBranch(const QString &itnam) // FIXME-2 uncomplete
{
    return nullptr;
}

bool VymModelWrapper::newBranchIteratorMap(const QString &itname, bool deepLevelsFirst)
{
    return model->newBranchIterator(itname, false, deepLevelsFirst);
}

bool VymModelWrapper::newBranchIteratorSelection(const QString &itname, bool deepLevelsFirst)
{
    return model->newBranchIterator(itname, true, deepLevelsFirst);
}

BranchWrapper* VymModelWrapper::nextBranch(const QString &itname)
{
    BranchItem *bi = model->nextBranchIterator(itname);
    if (bi)
        return bi->branchWrapper();
    else
        return nullptr;
}

ItemListWrapper* VymModelWrapper::newItemListMap()
{
    return new ItemListWrapper(model);
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

bool VymModelWrapper::saveSelection(const QString &filename)
{
    QString filename_org = model->getFilePath(); // Restore filename later
    if (!model->renameMap(filename)) {
        QString s = tr("Saving the selection in map failed:\nCouldn't rename map to %1").arg(filename);
        QMessageBox::critical(0,
            tr("Critical Error"), s);
        mainWindow->abortScript(QJSValue::GenericError, s);
        return false;
    }

    bool r = model->saveMap(File::PartOfMap);

    if (!model->renameMap(filename_org)) {
        QString s = tr("Saving the selection in map failed:\nCouldn't rename map to %1").arg(filename);
        QMessageBox::critical(0,
            tr("Critical Error"), s);
        mainWindow->abortScript(QJSValue::GenericError, s);
        return false;
    }

    return r;
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

void VymModelWrapper::setAuthor(const QString &s) { model->setMapAuthor(s); }

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

void VymModelWrapper::setBackgroundImageName(const QString &name)
{
    model->setBackgroundImageName(name);
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

void VymModelWrapper::setComment(const QString &s) { model->setMapComment(s); }

void VymModelWrapper::setLinkColorHint(const QString &hintName)
{
    LinkObj::ColorHint hint = LinkObj::linkColorHint(hintName);
    model->setLinkColorHint(hint);
}

void VymModelWrapper::setLinkStyle(const QString &style)
{
    if (!model->setLinkStyle(style))
        mainWindow->abortScript(
                QJSValue::GenericError,
                QString("Could not set linkstyle to %1").arg(style));
}

void VymModelWrapper::setRotationView(float a) { model->setMapRotation(a); }

void VymModelWrapper::setTitle(const QString &s) { model->setMapTitle(s); }

void VymModelWrapper::setZoom(float z) { model->setMapZoomFactor(z); }

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

void VymModelWrapper::undo() { model->undo(); }

void VymModelWrapper::unselectAll() { model->unselectAll(); }

void VymModelWrapper::unsetBackgroundImage()
{
    model->unsetBackgroundImage();
}
