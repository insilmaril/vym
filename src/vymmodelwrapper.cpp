#include "vymmodelwrapper.h"

#include "branchitem.h"
#include "branchobj.h"
#include "imageitem.h"
#include "misc.h"
#include "scripting.h"
#include "vymmodel.h"
#include "vymtext.h"
#include "xlink.h"
#include "xmlobj.h" // include quoteQuotes

///////////////////////////////////////////////////////////////////////////
VymModelWrapper::VymModelWrapper(VymModel *m) { model = m; }

/*
QString VymModelWrapper::setResult( const QString r )
{
    context()->engine()->globalObject().setProperty("lastResult", r );
}

bool VymModelWrapper::setResult( bool r )
{
    context()->engine()->globalObject().setProperty("lastResult", r );
}

int  VymModelWrapper::setResult( int r )
{
    context()->engine()->globalObject().setProperty("lastResult", r );
}
*/

BranchItem *VymModelWrapper::getSelectedBranch()
{
    BranchItem *selbi = model->getSelectedBranch();
    if (!selbi)
        logError(context(), QScriptContext::ReferenceError,
                 "No branch selected");
    return selbi;
}

QVariant VymModelWrapper::getParameter(bool &ok, const QString &key,
                                       const QStringList &parameters)
{
    // loop through parameters and try to find the one named "key"
    foreach (QString par, parameters) {
        if (par.startsWith(key)) {
            qDebug() << "getParam: " << key << "  has: " << par;
            ok = true;
            return QVariant(par);
        }
    }

    // Nothing found
    ok = false;
    return QVariant::Invalid;
}

void VymModelWrapper::addBranch()
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi) {
        if (argumentCount() > 1) {
            logError(context(), QScriptContext::SyntaxError,
                     "Too many arguments");
            return;
        }

        int pos = -2;
        if (argumentCount() == 1) {
            pos = argument(0).toInteger();
        }

        if (!model->addNewBranch(selbi, pos))
            logError(context(), QScriptContext::UnknownError,
                     "Couldn't add branch to map");
    }
}

void VymModelWrapper::addBranchBefore()
{
    if (!model->addNewBranchBefore())
        logError(context(), QScriptContext::UnknownError,
                 "Couldn't add branch before selection to map");
}

void VymModelWrapper::addMapCenter(qreal x, qreal y)
{
    if (!model->addMapCenter(QPointF(x, y)))
        logError(context(), QScriptContext::UnknownError,
                 "Couldn't add mapcenter");
}

void VymModelWrapper::addMapInsert(QString fileName, int pos, int contentFilter)
{
    if (QDir::isRelativePath(fileName))
        fileName = QDir::currentPath() + "/" + fileName;

    model->saveStateBeforeLoad(ImportAdd, fileName);

    if (File::Aborted ==
        model->loadMap(fileName, ImportAdd, VymMap, contentFilter, pos))
        logError(context(), QScriptContext::UnknownError,
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

    model->saveStateBeforeLoad(ImportReplace, fileName);

    if (File::Aborted == model->loadMap(fileName, ImportReplace, VymMap))
        logError(context(), QScriptContext::UnknownError,
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
                logError(context(), QScriptContext::UnknownError,
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
                logError(context(), QScriptContext::UnknownError,
                         QString("Couldn't set penstyle %1").arg(penstyle));
        }
        else
            logError(context(), QScriptContext::UnknownError,
                     "Begin or end of xLink are not branch or mapcenter");
    }
    else
        logError(context(), QScriptContext::UnknownError,
                 "Begin or end of xLink not found");
}

int VymModelWrapper::branchCount()
{
    int r;
    BranchItem *selbi = getSelectedBranch();
    if (selbi)
        r = selbi->branchCount();
    else
        r = -1;
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
        logError(context(), QScriptContext::UnknownError,
                 QString("Could not center on ID %1").arg(id));
}

void VymModelWrapper::clearFlags() { model->clearFlags(); }

void VymModelWrapper::colorBranch(const QString &color)
{
    QColor col(color);
    if (!col.isValid())
        logError(context(), QScriptContext::SyntaxError,
                 QString("Could not set color to %1").arg(color));
    else
        model->colorBranch(col);
}

void VymModelWrapper::colorSubtree(const QString &color)
{
    QColor col(color);
    if (!col.isValid())
        logError(context(), QScriptContext::SyntaxError,
                 QString("Could not set color to %1").arg(color));
    else
        model->colorSubtree(col);
}

void VymModelWrapper::copy() { model->copy(); }

void VymModelWrapper::cut() { model->cut(); }

void VymModelWrapper::cycleTask()
{
    if (!model->cycleTaskStatus())
        logError(context(), QScriptContext::SyntaxError,
                 "Couldn't cycle task status");
}

bool VymModelWrapper::exportMap()
{
    bool r = false;

    if (argumentCount() == 0) {
        logError(context(), QScriptContext::SyntaxError,
                 "Not enough arguments");
        return setResult(r);
    }

    QString format;
    format = argument(0).toString();

    if (argumentCount() == 1) {
        if (format == "Last") {
            model->exportLast();
            r = true;
        }
        else
            logError(context(), QScriptContext::SyntaxError,
                     "Filename missing");
        return setResult(r);
    }

    QString filename;

    filename = argument(1).toString();

    if (format == "AO") {
        model->exportAO(filename, false);
    }
    else if (format == "ASCII") {
        bool listTasks = false;
        if (argumentCount() == 3 && argument(2).toString() == "true")
            listTasks = true;
        model->exportASCII(filename, listTasks, false);
    }
    else if (format == "ConfluenceNewPage") {
        // 0: General export format
        // 1: URL of parent page (required)
        // 2: page title (required)
        if (argumentCount() < 3) {
            logError(context(), QScriptContext::SyntaxError,
                 QString("Confluence export new page: Only %1 instead of 3 parameters given")
                 .arg(argumentCount()));
            return setResult(r);
        }

        QString url = argument(2).toString();
        QString pageName = argument(3).toString();

        model->exportConfluence(true, url, pageName, false);
    }
    else if (format == "ConfluenceUpdatePage") {
        // 0: General export format
        // 1: URL of  page to be updated
        // 2: page title (optional)
        if (argumentCount() == 1) {
            logError(context(), QScriptContext::SyntaxError,
                     "URL of new page missing in Confluence export");
            return setResult(r);
        }
        QString url = argument(1).toString();

        QString title = "";
        if (argumentCount() == 3) {
            title = argument(2).toString();
        }

        model->exportConfluence(false, url, title, false);
    }
    else if (format == "CSV") {
        model->exportCSV(filename, false);
    }
    else if (format == "HTML") {
        if (argumentCount() < 3) {
            logError(context(), QScriptContext::SyntaxError,
                     "Path missing in HTML export");
            return setResult(r);
        }
        QString dpath = argument(2).toString();
        model->exportHTML(filename, dpath, false);
    }
    else if (format == "Image") {
        QString imgFormat;
        if (argumentCount() == 2)
            imgFormat = "PNG";
        else if (argumentCount() == 3)
            imgFormat = argument(2).toString();

        QStringList formats;
        formats << "PNG";
        formats << "GIF";
        formats << "JPG";
        formats << "JPEG", formats << "PNG", formats << "PBM", formats << "PGM",
            formats << "PPM", formats << "TIFF", formats << "XBM",
            formats << "XPM";
        if (formats.indexOf(imgFormat) < 0) {
            logError(context(), QScriptContext::SyntaxError,
                     QString("%1 not one of the known export formats: ")
                         .arg(imgFormat)
                         .arg(formats.join(",")));
            return setResult(r);
        }
        model->exportImage(filename, false, imgFormat);
    }
    else if (format == "Impress") {
        if (argumentCount() < 3) {
            logError(context(), QScriptContext::SyntaxError,
                     "Template file  missing in export to Impress");
            return setResult(r);
        }
        QString templ = argument(2).toString();
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
        if (argumentCount() < 3) {
            logError(context(), QScriptContext::SyntaxError,
                     "path missing in export to Impress");
            return setResult(r);
        }
        QString dpath = argument(2).toString();
        model->exportXML(filename, dpath, false);
    }
    else {
        logError(context(), QScriptContext::SyntaxError,
                 QString("Unknown export format: %1").arg(format));
        return setResult(r);
    }
    return setResult(true);
}

int VymModelWrapper::getBranchIndex()
{
    int r;
    BranchItem *selbi = getSelectedBranch();
    if (selbi) {
        r = selbi->num();
    } else
        r = -1;
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

QString VymModelWrapper::getFrameType()
{
    QString r;
    BranchItem *selbi = getSelectedBranch();
    if (selbi) {
        BranchObj *bo = (BranchObj *)(selbi->getLMO());
        if (!bo)
            logError(context(), QScriptContext::UnknownError,
                     QString("No BranchObj available"));
        else
            r = bo->getFrame()->getFrameTypeName();
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

QString VymModelWrapper::getMapAuthor()
{
    return setResult(model->getAuthor());
}

QString VymModelWrapper::getMapComment()
{
    return setResult(model->getComment());
}

QString VymModelWrapper::getMapTitle() { return setResult(model->getTitle()); }

QString VymModelWrapper::getNotePlainText()
{
    return setResult(model->getNote().getTextASCII());
}

QString VymModelWrapper::getNoteXML()
{
    return setResult(model->getNote().saveToDir());
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
    BranchItem *selbi = getSelectedBranch();
    if (selbi) {
        Task *task = selbi->getTask();
        if (task)
            r = task->getSleep().toString(Qt::ISODate);
        else
            logError(context(), QScriptContext::UnknownError,
                     "Branch has no task");
    }
    return setResult(r);
}

int VymModelWrapper::getTaskSleepDays()
{
    int r = -1;
    BranchItem *selbi = getSelectedBranch();
    if (selbi) {
        Task *task = selbi->getTask();
        if (task)
            r = task->getDaysSleep();
        else
            logError(context(), QScriptContext::UnknownError,
                     "Branch has no task");
    }
    return setResult(r);
}

QString VymModelWrapper::getURL() { return setResult(model->getURL()); }

QString VymModelWrapper::getVymLink() { return setResult(model->getVymLink()); }

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
    BranchItem *selbi = getSelectedBranch();
    if (selbi)
        r = selbi->hasActiveFlag(flag);
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
    BranchItem *selbi = getSelectedBranch();
    if (selbi) {
        Task *task = selbi->getTask();
        if (task)
            r = true;
    }
    else
        logError(context(), QScriptContext::UnknownError,
                 "Selected item is not a branch");

    return setResult(r);
}

void VymModelWrapper::importDir(const QString &path)
{
    model->importDir(
        path); // FIXME-3 error handling missing (in vymmodel and here)
}

bool VymModelWrapper::initIterator(const QString &iname, bool deepLevelsFirst)
{
    return model->initIterator(iname, deepLevelsFirst);
}

bool VymModelWrapper::nextIterator(const QString &iname)
{
    return model->nextIterator(iname);
}

bool VymModelWrapper::isScrolled()
{
    bool r = false;
    BranchItem *selbi = getSelectedBranch();
    if (selbi)
        r = selbi->isScrolled();
    return setResult(r);
}

void VymModelWrapper::loadImage(const QString &filename)
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi) {
        model->loadImage(
            selbi,
            filename); // FIXME-3 error handling missing (in vymmodel and here)
    }
}

void VymModelWrapper::loadNote(const QString &filename)
{
    model->loadNote(
        filename); // FIXME-3 error handling missing (in vymmodel and here)
}
void VymModelWrapper::moveDown() { model->moveDown(); }

void VymModelWrapper::moveUp() { model->moveUp(); }

void VymModelWrapper::moveSlideDown(int n)
{
    if (!model->moveSlideDown(n))
        logError(context(), QScriptContext::UnknownError,
                 "Could not move slide down");
}

void VymModelWrapper::moveSlideDown() { moveSlideDown(-1); }

void VymModelWrapper::moveSlideUp(int n)
{
    if (!model->moveSlideUp(n))
        logError(context(), QScriptContext::UnknownError,
                 "Could not move slide up");
}

void VymModelWrapper::moveSlideUp() { moveSlideUp(-1); }

void VymModelWrapper::nop() {}

void VymModelWrapper::note2URLs() { model->note2URLs(); }

bool VymModelWrapper::parseVymText(const QString &text)
{
    return setResult(model->parseVymText(unquoteQuotes(text)));
}

void VymModelWrapper::paste() { model->paste(); }

void VymModelWrapper::redo() { model->redo(); }

bool VymModelWrapper::relinkTo(const QString &parent, int num)
{
    bool r;
    r = model->relinkTo(parent, num);
    if (!r)
        logError(context(), QScriptContext::UnknownError, "Could not relink");
    return setResult(r);
}

bool VymModelWrapper::relinkTo(const QString &parent)
{
    bool r = relinkTo(parent, -1);
    return setResult(r);
}

void VymModelWrapper::remove() { model->deleteSelection(); }

void VymModelWrapper::removeChildren() { model->deleteChildren(); }

void VymModelWrapper::removeKeepChildren() { model->deleteKeepChildren(); }

void VymModelWrapper::removeSlide(int n)
{
    if (n < 0 || n >= model->slideCount() - 1)
        logError(context(), QScriptContext::RangeError,
                 QString("Slide '%1' not available.").arg(n));
}

QVariant VymModelWrapper::repeatLastCommand()
{
    return model->repeatLastCommand();
}

void VymModelWrapper::saveImage(const QString &filename)
{
    model->saveImage(NULL, filename);
}

void VymModelWrapper::saveNote(const QString &filename)
{
    model->saveNote(filename);
}

void VymModelWrapper::scroll()
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi) {
        if (!model->scrollBranch(selbi))
            logError(context(), QScriptContext::UnknownError,
                     "Couldn't scroll branch");
    }
}

bool VymModelWrapper::select(const QString &s)
{
    bool r = model->select(s);
    if (!r)
        logError(context(), QScriptContext::UnknownError,
                 QString("Couldn't select %1").arg(s));
    return setResult(r);
}

bool VymModelWrapper::selectID(const QString &s)
{
    bool r = model->selectID(s);
    if (!r)
        logError(context(), QScriptContext::UnknownError,
                 QString("Couldn't select ID %1").arg(s));
    return setResult(r);
}

bool VymModelWrapper::selectFirstBranch()
{
    bool r = false;
    BranchItem *selbi = getSelectedBranch();
    if (selbi) {
        r = model->selectFirstBranch();
        if (!r)
            logError(context(), QScriptContext::UnknownError,
                     "Couldn't select first branch");
    }
    return setResult(r);
}

bool VymModelWrapper::selectFirstChildBranch()
{
    bool r = false;
    BranchItem *selbi = getSelectedBranch();
    if (selbi) {
        r = model->selectFirstChildBranch();
        if (!r)
            logError(context(), QScriptContext::UnknownError,
                     "Couldn't select first child branch");
    }
    return setResult(r);
}

bool VymModelWrapper::selectLastBranch()
{
    bool r = false;
    BranchItem *selbi = getSelectedBranch();
    if (selbi) {
        r = model->selectLastBranch();
        if (!r)
            logError(context(), QScriptContext::UnknownError,
                     "Couldn't select last branch");
    }
    return setResult(r);
}

bool VymModelWrapper::selectLastChildBranch()
{
    bool r = false;
    BranchItem *selbi = getSelectedBranch();
    if (selbi) {
        r = model->selectLastChildBranch();
        if (!r)
            logError(context(), QScriptContext::UnknownError,
                     "Couldn't select last child branch");
    }
    return setResult(r);
}
bool VymModelWrapper::selectLastImage()
{
    bool r = false;
    BranchItem *selbi = getSelectedBranch();
    if (selbi) {
        ImageItem *ii = selbi->getLastImage();
        if (!ii)
            logError(context(), QScriptContext::UnknownError,
                     "Couldn't get last image");
        else {
            r = model->select(ii);
            if (!r)
                logError(context(), QScriptContext::UnknownError,
                         "Couldn't select last image");
        }
    }
    return setResult(r);
}

bool VymModelWrapper::selectParent()
{
    bool r = model->selectParent();
    if (!r)
        logError(context(), QScriptContext::UnknownError,
                 "Couldn't select parent item");
    return setResult(r);
}

bool VymModelWrapper::selectLatestAdded()
{
    bool r = model->selectLatestAdded();
    if (!r)
        logError(context(), QScriptContext::UnknownError,
                 "Couldn't select latest added item");
    return setResult(r);
}

bool VymModelWrapper::selectToggle(const QString &selectString)
{
    bool r = model->selectToggle(selectString);
    if (!r)
        logError(context(), QScriptContext::UnknownError,
                 "Couldn't toggle item with select string " + selectString);
    return setResult(r);
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
    const QString &text) // FIXME-3  what about RT?
{
    model->setHeadingPlainText(text);
}

void VymModelWrapper::setHideExport(bool b) { model->setHideExport(b); }

void VymModelWrapper::setIncludeImagesHorizontally(bool b)
{
    model->setIncludeImagesHor(b);
}

void VymModelWrapper::setIncludeImagesVertically(bool b)
{
    model->setIncludeImagesVer(b);
}

void VymModelWrapper::setHideLinkUnselected(bool b)
{
    model->setHideLinkUnselected(b);
}

void VymModelWrapper::setMapAnimCurve(int n)
{
    if (n < 0 || n > QEasingCurve::OutInBounce)
        logError(context(), QScriptContext::RangeError,
                 "Unknown animation curve type");
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

void VymModelWrapper::setMapAuthor(const QString &s) { model->setAuthor(s); }

void VymModelWrapper::setMapBackgroundColor(const QString &color)
{
    QColor col(color);
    if (col.isValid()) {
        model->setMapBackgroundColor(col);
    }
    else
        logError(context(), QScriptContext::UnknownError,
                 QString("Could not set color to %1").arg(color));
}

void VymModelWrapper::setMapComment(const QString &s) { model->setComment(s); }

void VymModelWrapper::setMapDefLinkColor(const QString &color)
{
    QColor col(color);
    if (col.isValid()) {
        model->setMapDefLinkColor(col);
    }
    else
        logError(context(), QScriptContext::UnknownError,
                 QString("Could not set color to %1").arg(color));
}

void VymModelWrapper::setMapLinkStyle(const QString &style)
{
    if (!model->setMapLinkStyle(style))
        logError(context(), QScriptContext::UnknownError,
                 QString("Could not set linkstyle to %1").arg(style));
}

void VymModelWrapper::setMapRotation(float a) { model->setMapRotationAngle(a); }

void VymModelWrapper::setMapTitle(const QString &s) { model->setTitle(s); }

void VymModelWrapper::setMapZoom(float z) { model->setMapZoomFactor(z); }

void VymModelWrapper::setNotePlainText(const QString &s)
{
    VymNote vn;
    vn.setPlainText(s);
    model->setNote(vn);
}

void VymModelWrapper::setPos(qreal x, qreal y)
{
    model->setPos(QPointF(x, y));
}

void VymModelWrapper::setFrameBorderWidth(int width)
{
    model->setFrameBorderWidth(width);
}

void VymModelWrapper::setFrameBrushColor(const QString &color)
{
    model->setFrameBrushColor(color);
}

void VymModelWrapper::setFrameIncludeChildren(bool b)
{
    model->setFrameIncludeChildren(b);
}

void VymModelWrapper::setFramePadding(int padding)
{
    model->setFramePadding(padding);
}

void VymModelWrapper::setFramePenColor(const QString &color)
{
    model->setFramePenColor(color);
}

void VymModelWrapper::setFrameType(const QString &type)
{
    model->setFrameType(type);
}

void VymModelWrapper::setScaleFactor(qreal f) { model->setScaleFactor(f); }

void VymModelWrapper::setSelectionColor(const QString &color)
{
    QColor col(color);
    if (!col.isValid())
        logError(context(), QScriptContext::SyntaxError,
                 QString("Could not set color to %1").arg(color));
    else
        model->setSelectionColor(col);
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

void VymModelWrapper::setURL(const QString &s) { model->setURL(s); }

void VymModelWrapper::setVymLink(const QString &s) { model->setVymLink(s); }

void VymModelWrapper::setXLinkColor(const QString &color)
{
    QColor col(color);
    if (!col.isValid())
        logError(context(), QScriptContext::SyntaxError,
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
    // sleep is not avail on windows VCEE, workaround could be using
    // this->thread()->wait(x ms)
    sleep(n);
}

void VymModelWrapper::sortChildren(bool b) { model->sortChildren(b); }

void VymModelWrapper::sortChildren() { sortChildren(false); }

void VymModelWrapper::toggleFlagByUid(const QString &s)
{
    model->toggleFlagByUid(QUuid(s));
}

void VymModelWrapper::toggleFlagByName(const QString &s)
{
    model->toggleFlagByName(s);
}

void VymModelWrapper::toggleFrameIncludeChildren()
{
    model->toggleFrameIncludeChildren();
}

void VymModelWrapper::toggleScroll() { model->toggleScroll(); }

void VymModelWrapper::toggleTarget() { model->toggleTarget(); }

void VymModelWrapper::toggleTask() { model->toggleTask(); }

void VymModelWrapper::undo() { model->undo(); }

bool VymModelWrapper::unscroll()
{
    bool r = false;
    BranchItem *selbi = getSelectedBranch();
    if (selbi) {
        r = model->unscrollBranch(selbi);
        if (!r)
            logError(context(), QScriptContext::UnknownError,
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
