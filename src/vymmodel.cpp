#include <QApplication>
#include <QSvgGenerator>

#if defined(VYM_DBUS)
#include <QtDBus/QDBusConnection>
#endif

#ifndef Q_OS_WINDOWS
#include <unistd.h>
#else
#define sleep Sleep
#endif

#include <QColorDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QPrinter>

#include "vymmodel.h"

#include "attributeitem.h"
#include "branchitem.h"
#include "confluence-agent.h"
#include "download-agent.h"
#include "editxlinkdialog.h"
#include "export-ao.h"
#include "export-ascii.h"
#include "export-confluence.h"
#include "export-csv.h"
#include "export-firefox.h"
#include "export-html.h"
#include "export-impress.h"
#include "export-latex.h"
#include "export-markdown.h"
#include "export-orgmode.h"
#include "file.h"
#include "findresultmodel.h"
#include "jira-agent.h"
#include "lockedfiledialog.h"
#include "mainwindow.h"
#include "misc.h"
#include "noteeditor.h"
#include "options.h"
#include "scripteditor.h"
#include "slideitem.h"
#include "slidemodel.h"
#include "taskeditor.h"
#include "taskmodel.h"
#include "treeitem.h"
#include "vymprocess.h"
#include "warningdialog.h"
#include "xlinkitem.h"
#include "xlinkobj.h"
#include "xml-freemind.h"
#include "xml-vym.h"
#include "xmlobj.h"

#ifdef Q_OS_WINDOWS
#include <windows.h>
#endif

extern bool debug;
extern bool testmode;
extern bool recoveryMode;
extern QStringList ignoredLockedFiles;

extern Main *mainWindow;

extern Settings settings;
extern QDir tmpVymDir;

extern NoteEditor *noteEditor;
extern TaskEditor *taskEditor;
extern ScriptEditor *scriptEditor;
extern FlagRowMaster *standardFlagsMaster;
extern FlagRowMaster *userFlagsMaster;

extern Options options;

extern QString clipboardDir;
extern QString clipboardFile;
extern uint clipboardItemCount;

extern ImageIO imageIO;

extern TaskModel *taskModel;

extern QString vymName;
extern QString vymVersion;
extern QDir vymBaseDir;

extern QDir lastImageDir;
extern QDir lastMapDir;
extern QDir lastExportDir;

extern bool jiraClientAvailable;
extern bool confluenceAgentAvailable;
extern QString confluencePassword;

extern Settings settings;

uint VymModel::idLast = 0; // make instance

VymModel::VymModel()
{
    // qDebug()<< "Const VymModel" << this;
    init();
    rootItem->setModel(this);
    wrapper = new VymModelWrapper(this);
}

VymModel::~VymModel()
{
    // qDebug() << "Destr VymModel begin this="<<this<<"  "<<mapName<<flush;
    mapEditor = NULL;
    repositionBlocked = true;
    autosaveTimer->stop();
    fileChangedTimer->stop();
    stopAllAnimation();

    // qApp->processEvents();	// Update view (scene()->update() is not enough)
    // qDebug() << "Destr VymModel end   this="<<this;

    delete (wrapper);
}

void VymModel::clear()
{
    while (rootItem->childCount() > 0) {
        // qDebug()<<"VM::clear  ri="<<rootItem<<"
        // ri->count()="<<rootItem->childCount();
        deleteItem(rootItem->getChildNum(0));
    }
}

void VymModel::init()
{
    // No MapEditor yet
    mapEditor = NULL;

    // Use default author
    author =
        settings
            .value("/user/name", tr("unknown user",
                                    "default name for map author in settings"))
            .toString();

    // States and IDs
    idLast++;
    modelID = idLast;
    mapChanged = false;
    mapDefault = true;
    mapUnsaved = false;

    // Selection history
    selModel = NULL;
    selectionBlocked = false;
    resetSelectionHistory();

    resetHistory();

    // Create tmp dirs
    makeTmpDirectories();

    // Files
    readonly = false;
    zipped = true;
    filePath = "";
    fileName = tr("unnamed");
    mapName = fileName;
    repositionBlocked = false;
    saveStateBlocked = false;

    autosaveTimer = new QTimer(this);
    connect(autosaveTimer, SIGNAL(timeout()), this, SLOT(autosave()));

    fileChangedTimer = new QTimer(this);
    connect(fileChangedTimer, SIGNAL(timeout()), this, SLOT(fileChanged()));
    fileChangedTimer->start(3000);

    taskAlarmTimer = new QTimer(this);
    connect(taskAlarmTimer, SIGNAL(timeout()), this, SLOT(updateTasksAlarm()));
    taskAlarmTimer->start(3000);

    // animations   // FIXME-4 switch to new animation system
    animationUse =
        settings.value("/animation/use", false)
            .toBool(); // FIXME-4 add options to control _what_ is animated
    animationTicks = settings.value("/animation/ticks", 20).toInt();
    animationInterval = settings.value("/animation/interval", 5).toInt();
    animObjList.clear();
    animationTimer = new QTimer(this);
    connect(animationTimer, SIGNAL(timeout()), this, SLOT(animate()));

    // View - map
    defaultFont.setPointSizeF(16);
    defLinkColor = QColor(0, 0, 255);
    linkcolorhint = LinkableMapObj::DefaultColor;
    linkstyle = LinkableMapObj::PolyParabel;
    defXLinkPen.setWidth(1);
    defXLinkPen.setColor(QColor(50, 50, 255));
    defXLinkPen.setStyle(Qt::DashLine);
    defXLinkStyleBegin = "HeadFull";
    defXLinkStyleEnd = "HeadFull";

    hasContextPos = false;

    hidemode = TreeItem::HideNone;

    // Animation in MapEditor
    zoomFactor = 1;
    rotationAngle = 0;
    animDuration = 2000;
    animCurve = QEasingCurve::OutQuint;

    // Initialize presentation slides
    slideModel = new SlideModel(this);
    blockSlideSelection = false;

    // Avoid recursions later
    cleaningUpLinks = false;

    // Network
    netstate = Offline;

#if defined(VYM_DBUS)
    // Announce myself on DBUS
    new AdaptorModel(this); // Created and not deleted as documented in Qt
    if (!QDBusConnection::sessionBus().registerObject(
            QString("/vymmodel_%1").arg(modelID), this))
        qWarning("VymModel: Couldn't register DBUS object!");
#endif
}

void VymModel::makeTmpDirectories()
{
    // Create unique temporary directories
    tmpMapDirPath = tmpVymDir.path() + QString("/model-%1").arg(modelID);
    histPath = tmpMapDirPath + "/history";
    QDir d;
    d.mkdir(tmpMapDirPath);
}

QString VymModel::tmpDirPath() { return tmpMapDirPath; }

MapEditor *VymModel::getMapEditor() { return mapEditor; }

VymModelWrapper *VymModel::getWrapper() { return wrapper; }

bool VymModel::isRepositionBlocked() { return repositionBlocked; }

void VymModel::updateActions()
{
    // Tell mainwindow to update states of actions
    mainWindow->updateActions();
}

bool VymModel::setData(const QModelIndex &, const QVariant &value, int role)
{
    if (role != Qt::EditRole)
        return false;

    setHeadingPlainText(value.toString());

    return true;
}

void VymModel::resetUsedFlags()
{
    standardFlagsMaster->resetUsedCounter();
    userFlagsMaster->resetUsedCounter();
}

QString VymModel::saveToDir(const QString &tmpdir, const QString &prefix,
                            FlagRowMaster::WriteMode flagMode, const QPointF &offset,
                            TreeItem *saveSel)
{
    // tmpdir	    temporary directory to which data will be written
    // prefix	    mapname, which will be appended to images etc.
    //
    // writeflags   Only write flags for "real" save of map, not undo
    // offset	    offset of bbox of whole map in scene.
    //		    Needed for XML export

    XMLObj xml;

    // Save Header
    QString ls;
    switch (linkstyle) {
    case LinkableMapObj::Line:
        ls = "StyleLine";
        break;
    case LinkableMapObj::Parabel:
        ls = "StyleParabel";
        break;
    case LinkableMapObj::PolyLine:
        ls = "StylePolyLine";
        break;
    default:
        ls = "StylePolyParabel";
        break;
    }

    QString header =
        "<?xml version=\"1.0\" encoding=\"utf-8\"?><!DOCTYPE vymmap>\n";
    QString colhint = "";
    if (linkcolorhint == LinkableMapObj::HeadingColor)
        colhint = xml.attribut("linkColorHint", "HeadingColor");

    QString mapAttr = xml.attribut("version", vymVersion);
    if (!saveSel)
        mapAttr +=
            xml.attribut("author", author) + xml.attribut("title", title) +
            xml.attribut("comment", comment) + xml.attribut("date", getDate()) +
            xml.attribut("branchCount", QString().number(branchCount())) +
            xml.attribut(
                "backgroundColor",
                mapEditor->getScene()->backgroundBrush().color().name()) +
            xml.attribut("defaultFont", defaultFont.toString()) +
            xml.attribut("selectionColor",
                         mapEditor->getSelectionColor().name()) +
            xml.attribut("linkStyle", ls) +
            xml.attribut("linkColor", defLinkColor.name()) +
            xml.attribut("defXLinkColor", defXLinkPen.color().name()) +
            xml.attribut("defXLinkWidth",
                         QString().setNum(defXLinkPen.width(), 10)) +
            xml.attribut("defXLinkPenStyle",
                         penStyleToString(defXLinkPen.style())) +
            xml.attribut("defXLinkStyleBegin", defXLinkStyleBegin) +
            xml.attribut("defXLinkStyleEnd", defXLinkStyleEnd) +
            xml.attribut("mapZoomFactor",
                         QString().setNum(mapEditor->getZoomFactorTarget())) +
            xml.attribut("mapRotationAngle",
                         QString().setNum(mapEditor->getAngleTarget())) +
            colhint;
    header += xml.beginElement("vymmap", mapAttr);
    xml.incIndent();

    // Find the used flags while traversing the tree
    resetUsedFlags();

    // Temporary list of links
    QList<Link *> tmpLinks;

    QString tree;
    // Build xml recursivly
    if (!saveSel) {
        // Save all mapcenters as complete map, if saveSel not set
        tree += saveTreeToDir(tmpdir, prefix, offset, tmpLinks);

        // Save local settings
        tree += settings.getDataXML(destPath);

        // Save selection
        if (getSelectedItem() && !saveSel)
            tree += xml.valueElement("select", getSelectString());
    }
    else {
        switch (saveSel->getType()) {
        case TreeItem::Branch:
            // Save Subtree
            tree += ((BranchItem *)saveSel)
                        ->saveToDir(tmpdir, prefix, offset, tmpLinks);
            break;
        case TreeItem::MapCenter:
            // Save Subtree
            tree += ((BranchItem *)saveSel)
                        ->saveToDir(tmpdir, prefix, offset, tmpLinks);
            break;
        case TreeItem::Image:
            // Save Image
            tree += ((ImageItem *)saveSel)->saveToDir(tmpdir, prefix);
            break;
        default:
            // other types shouldn't be safed directly...
            break;
        }
    }

    QString flags;

    // Write images and definitions of of used user flags
    if (flagMode != FlagRowMaster::NoFlags) {
        // First find out, which flags are used
        // Definitions
        flags += userFlagsMaster->saveDef(flagMode);

        userFlagsMaster->saveDataToDir(tmpdir + "flags/user/", flagMode);
        standardFlagsMaster->saveDataToDir(tmpdir + "flags/standard/",
                                           flagMode);
    }

    QString footer;
    // Save XLinks
    for (int i = 0; i < tmpLinks.count(); ++i)
        footer += tmpLinks.at(i)->saveToDir();

    // Save slides
    footer += slideModel->saveToDir();

    xml.decIndent();
    footer += xml.endElement("vymmap");

    return header + flags + tree + footer;
}

QString VymModel::saveTreeToDir(const QString &tmpdir, const QString &prefix,
                                const QPointF &offset, QList<Link *> &tmpLinks)
{
    QString s;
    for (int i = 0; i < rootItem->branchCount(); i++)
        s += rootItem->getBranchNum(i)->saveToDir(tmpdir, prefix, offset,
                                                  tmpLinks);
    return s;
}

void VymModel::setFilePath(QString fpath, QString destname)
{
    if (fpath.isEmpty() || fpath == "") {
        filePath = "";
        fileName = "";
        destPath = "";
    }
    else {
        filePath = fpath;    // becomes absolute path
        fileName = fpath;    // gets stripped of path
        destPath = destname; // needed for vymlinks and during load to reset
                             // fileChangedTime

        // If fpath is not an absolute path, complete it
        filePath = QDir(fpath).absolutePath();
        fileDir = filePath.left(1 + filePath.lastIndexOf("/"));

        // Set short name, too. Search from behind:
        fileName = basename(fileName);

        // Forget the .vym (or .xml) for name of map
        mapName =
            fileName.left(fileName.lastIndexOf(".", -1, Qt::CaseSensitive));
    }
}

void VymModel::setFilePath(QString fpath) { setFilePath(fpath, fpath); }

QString VymModel::getFileDir() { return fileDir; }

QString VymModel::getFilePath() { return filePath; }

QString VymModel::getFileName() { return fileName; }

QString VymModel::getMapName() { return mapName; }

QString VymModel::getDestPath() { return destPath; }

bool VymModel::parseVymText(const QString &s)
{
    bool ok = false;
    BranchItem *bi = getSelectedBranch();
    if (bi) {
        parseBaseHandler *handler = new parseVYMHandler;

        bool saveStateBlockedOrg = saveStateBlocked;
        repositionBlocked = true;
        saveStateBlocked = true;
        QXmlInputSource source;
        source.setData(s);
        QXmlSimpleReader reader;
        reader.setContentHandler(handler);
        reader.setErrorHandler(handler);

        handler->setInputString(s);
        handler->setModel(this);
        handler->setLoadMode(ImportReplace, 0);

        ok = reader.parse(source);
        repositionBlocked = false;
        saveStateBlocked = saveStateBlockedOrg;
        if (ok) {
            if (s.startsWith("<vymnote"))
                emitNoteChanged(bi);
            emitDataChanged(bi);
            reposition(); // to generate bbox sizes
        }
        else {
            QMessageBox::critical(0, tr("Critical Parse Error"),
                                  tr(handler->errorProtocol().toUtf8()));
            // returnCode=1;
            // Still return "success": the map maybe at least
            // partially read by the parser
        }
    }
    return ok;
}

File::ErrorCode VymModel::loadMap(QString fname, const LoadMode &lmode,
                                  const FileType &ftype,
                                  const int &contentFilter, int pos)
{
    File::ErrorCode err = File::Success;

    // Get updated zoomFactor, before applying one read from file in the end
    if (mapEditor) {
        zoomFactor = mapEditor->getZoomFactorTarget();
        rotationAngle = mapEditor->getAngleTarget();
    }

    parseBaseHandler *handler;
    fileType = ftype;
    switch (fileType) {
    case VymMap:
        handler = new parseVYMHandler;
        ((parseVYMHandler *)handler)->setContentFilter(contentFilter);
        break;
    case FreemindMap:
        handler = new parseFreemindHandler;
        break;
    default:
        QMessageBox::critical(0, tr("Critical Parse Error"),
                              "Unknown FileType in VymModel::load()");
        return File::Aborted;
    }

    if (lmode == NewMap) {
        // Reset timestamp to check for later updates of file
        fileChangedTime = QFileInfo(destPath).lastModified();

        selModel->clearSelection();
    }

    bool zipped_org = zipped;

    // Create temporary directory for packing
    bool ok;
    QString tmpZipDir = makeTmpDir(ok, tmpDirPath(), "unzip");
    if (!ok) {
        QMessageBox::critical(
            0, tr("Critical Load Error"),
            tr("Couldn't create temporary directory before load\n"));
        return File::Aborted;
    }

    QString xmlfile;
    if (fname.right(4) == ".xml" || fname.right(3) == ".mm") {
        xmlfile = fname;
        zipped = false;

        if (lmode == NewMap || lmode == DefaultMap)
            zipped_org = false;
    }
    else {
        // Try to unzip file
        err = unzipDir(tmpZipDir, fname);
    }

    if (zipped) {
        // Look for mapname.xml
        xmlfile = fname.left(fname.lastIndexOf(".", -1, Qt::CaseSensitive));
        xmlfile = xmlfile.section('/', -1);
        QFile mfile(tmpZipDir + "/" + xmlfile + ".xml");
        if (!mfile.exists()) {
            // mapname.xml does not exist, well,
            // maybe someone renamed the mapname.vym file...
            // Try to find any .xml in the toplevel
            // directory of the .vym file
            QStringList filters;
            filters << "*.xml";
            QStringList flist = QDir(tmpZipDir).entryList(filters);
            if (flist.count() == 1) {
                // Only one entry, take this one
                xmlfile = tmpZipDir + "/" + flist.first();
            }
            else {
                for (QStringList::Iterator it = flist.begin();
                     it != flist.end(); ++it)
                    *it = tmpZipDir + "/" + *it;
                // FIXME-4 Multiple entries, load all (but only the first one
                // into this ME)
                // mainWindow->fileLoadFromTmp (flist);
                // returnCode = 1;	// Silently forget this attempt to load
                qWarning("MainWindow::load (fn)  multimap found...");
            }

            if (flist.isEmpty()) {
                QMessageBox::critical(
                    0, tr("Critical Load Error"),
                    tr("Couldn't find a map (*.xml) in .vym archive.\n"));
                err = File::Aborted;
            }
        } // file doesn't exist
        else
            xmlfile = mfile.fileName();
    }

    QFile file(xmlfile);

    // I am paranoid: file should exist anyway
    // according to check in mainwindow.
    if (!file.exists()) {
        QMessageBox::critical(
            0, tr("Critical Parse Error"),
            tr(QString("Couldn't open map %1").arg(file.fileName()).toUtf8()));
        err = File::Aborted;
    }
    else {
        bool saveStateBlockedOrg = saveStateBlocked;
        repositionBlocked = true;
        saveStateBlocked = true;
        mapEditor->setViewportUpdateMode(QGraphicsView::NoViewportUpdate);
        QXmlInputSource source(&file);
        QXmlSimpleReader reader;
        reader.setContentHandler(handler);
        reader.setErrorHandler(handler);
        handler->setModel(this);

        // We need to set the tmpDir in order  to load files with rel. path
        QString tmpdir;
        if (zipped)
            tmpdir = tmpZipDir;
        else
            tmpdir = fname.left(fname.lastIndexOf("/", -1));
        handler->setTmpDir(tmpdir);
        handler->setInputFile(file.fileName());
        if (lmode == ImportReplace)
            handler->setLoadMode(ImportReplace, pos);
        else
            handler->setLoadMode(lmode, pos);

        // Here we actually parse the XML file
        bool ok = reader.parse(source);

        // Aftermath
        repositionBlocked = false;
        saveStateBlocked = saveStateBlockedOrg;
        mapEditor->setViewportUpdateMode(QGraphicsView::MinimalViewportUpdate);
        file.close();
        if (ok) {
            reposition(); // to generate bbox sizes
            emitSelectionChanged();

            if (lmode == NewMap) // no lockfile for default map!
            {
                mapDefault = false;
                mapChanged = false;
                mapUnsaved = false;
                autosaveTimer->stop();

                resetHistory();
                resetSelectionHistory();

                if (!tryVymLock() && debug)
                    qWarning() << "VM::loadMap  no lockfile created!";
            }

            // Recalc priorities and sort
            taskModel->recalcPriorities();
        }
        else {
            QMessageBox::critical(0, tr("Critical Parse Error"),
                                  tr(handler->errorProtocol().toUtf8()));
            // returnCode=1;
            // Still return "success": the map maybe at least
            // partially read by the parser
        }
    }

    // Delete tmpZipDir
    removeDir(QDir(tmpZipDir));

    // Restore original zip state
    zipped = zipped_org;

    updateActions();

    if (lmode != NewMap)
        emitUpdateQueries();

    if (mapEditor) {
        mapEditor->setZoomFactorTarget(zoomFactor);
        mapEditor->setAngleTarget(rotationAngle);
    }

    if (vymView)
        vymView->readSettings();

    qApp->processEvents(); // Update view (scene()->update() is not enough)
    return err;
}

File::ErrorCode VymModel::save(const SaveMode &savemode)
{
    QString tmpZipDir;
    QString mapFileName;
    QString safeFilePath;

    File::ErrorCode err = File::Success;

    if (zipped)
        // save as .xml
        mapFileName = mapName + ".xml";
    else
        // use name given by user, even if he chooses .doc
        mapFileName = fileName;

    // Look, if we should zip the data:
    if (!zipped)
    {
        QMessageBox mb(vymName,
                       tr("The map %1\ndid not use the compressed "
                          "vym file format.\nWriting it uncompressed will also "
                          "write images \n"
                          "and flags and thus may overwrite files into the "
                          "given directory\n\nDo you want to write the map")
                           .arg(filePath),
                       QMessageBox::Warning,
                       QMessageBox::Yes | QMessageBox::Default, QMessageBox::No,
                       QMessageBox::Cancel | QMessageBox::Escape);
        mb.setButtonText(QMessageBox::Yes, tr("compressed (vym default)"));
        mb.setButtonText(
            QMessageBox::No,
            tr("uncompressed, potentially overwrite existing data"));
        mb.setButtonText(QMessageBox::Cancel, tr("Cancel"));
        switch (mb.exec()) {
        case QMessageBox::Yes:
            // save compressed (default file format)
            zipped = true;
            break;
        case QMessageBox::No:
            // save uncompressed
            zipped = false;
            break;
        case QMessageBox::Cancel:
            // do nothing
            return File::Aborted;
            break;
        }
    }

    // First backup existing file, we
    // don't want to add to old zip archives
    QFile f(destPath);
    if (f.exists()) {
        if (settings.value("/system/writeBackupFile").toBool()) {
            QString backupFileName(destPath + "~");
            QFile backupFile(backupFileName);
            if (backupFile.exists() && !backupFile.remove()) {
                QMessageBox::warning(
                    0, tr("Save Error"),
                    tr("%1\ncould not be removed before saving")
                        .arg(backupFileName));
            }
            else {
                if (!f.rename(backupFileName)) {
                    QMessageBox::warning(
                        0, tr("Save Error"),
                        tr("%1\ncould not be renamed before saving")
                            .arg(destPath));
                }
            }
        }
    }

    if (zipped) {
        // Create temporary directory for packing
        bool ok;
        tmpZipDir = makeTmpDir(ok, tmpDirPath(), "zip");
        if (!ok) {
            QMessageBox::critical(
                0, tr("Critical Save Error"),
                tr("Couldn't create temporary directory before save\n"));
            return File::Aborted;
        }

        safeFilePath = filePath;
        setFilePath(tmpZipDir + "/" + mapName + ".xml", safeFilePath);
    } // zipped

    // Create mapName and fileDir
    makeSubDirs(fileDir);

    QString saveFile;
    if (savemode == CompleteMap || selModel->selection().isEmpty()) {
        // Save complete map
        if (zipped)
            // Use defined name for map within zipfile to avoid problems
            // with zip library and umlauts (see #98)
            saveFile =
                saveToDir(fileDir, "", FlagRowMaster::UsedFlags, QPointF(), NULL);
        else
            saveFile = saveToDir(fileDir, mapName + "-", FlagRowMaster::UsedFlags,
                                 QPointF(), NULL);
        mapChanged = false;
        mapUnsaved = false;
        autosaveTimer->stop();
    }
    else {
        // Save part of map
        if (selectionType() == TreeItem::Image)
            saveImage();
        else
            saveFile = saveToDir(fileDir, mapName + "-", FlagRowMaster::UsedFlags,
                                 QPointF(), getSelectedBranch());
        // TODO take care of multiselections
    }

    bool saved;
    if (zipped)
        // Use defined map name "map.xml", if zipped. Introduce in 2.6.6
        saved = saveStringToDisk(fileDir + "map.xml", saveFile);
    else
        // Use regular mapName, when saved as XML
        saved = saveStringToDisk(fileDir + mapFileName, saveFile);
    if (!saved) {
        err = File::Aborted;
        qWarning("ME::saveStringToDisk failed!");
    }

    if (zipped) {
        // zip
        if (err == File::Success)
            err = zipDir(tmpZipDir, destPath);

        // Delete tmpDir
        removeDir(QDir(tmpZipDir));

        // Restore original filepath outside of tmp zip dir
        setFilePath(safeFilePath);
    }

    updateActions();

    fileChangedTime = QFileInfo(destPath).lastModified();
    return err;
}

void VymModel::loadImage(BranchItem *dst, const QString &fn)
{
    if (!dst)
        dst = getSelectedBranch();
    if (dst) {
        QString filter = QString(tr("Images") +
                                 " (*.png *.bmp *.xbm *.jpg *.png *.xpm *.gif "
                                 "*.pnm *.svg *.svgz);;" +
                                 tr("All", "Filedialog") + " (*.*)");
        QStringList fns;
        if (fn.isEmpty())
            fns = QFileDialog::getOpenFileNames(
                NULL, vymName + " - " + tr("Load image"), lastImageDir.path(),
                filter);
        else
            fns.append(fn);

        if (!fns.isEmpty()) {
            lastImageDir.setPath(
                fns.first().left(fns.first().lastIndexOf("/")));
            QString s;
            for (int j = 0; j < fns.count(); j++) {
                s = fns.at(j);
                ImageItem *ii = createImage(dst);
                if (ii && ii->load(s)) {
                    saveState((TreeItem *)ii, "remove()", dst,
                              QString("loadImage (\"%1\")").arg(s),
                              QString("Add image %1 to %2")
                                  .arg(s)
                                  .arg(getObjectName(dst)));
                    // Find nice position for new image, take childPos
                    FloatImageObj *fio = (FloatImageObj *)(ii->getMO());
                    if (fio) {
                        LinkableMapObj *parLMO = dst->getLMO();

                        if (parLMO) {
                            fio->move(parLMO->getChildRefPos());
                            fio->setRelPos();
                        }
                    }

                    // On default include image // FIXME-4 check, if we change
                    // default settings...
                    select(dst);
                    setIncludeImagesHor(false);
                    setIncludeImagesVer(true);

                    reposition();
                }
                else {
                    qWarning() << "vymmodel: Failed to load " + s;
                    deleteItem(ii);
                }
            }
        }
    }
}

void VymModel::saveImage(ImageItem *ii, QString fn)
{
    if (!ii)
        ii = getSelectedImage();
    if (ii) {
        QString filter = QString(
            tr("Images") +
            " (*.png *.bmp *.xbm *.jpg *.png *.xpm *.gif *.pnm *.svg);;" +
            tr("All", "Filedialog") + " (*.*)");
        if (fn.isEmpty())
            fn = QFileDialog::getSaveFileName(
                NULL, vymName + " - " + tr("Save image"), lastImageDir.path(),
                filter, NULL, QFileDialog::DontConfirmOverwrite);

        if (!fn.isEmpty()) {
            lastImageDir.setPath(fn.left(fn.lastIndexOf("/")));
            if (QFile(fn).exists()) {
                QMessageBox mb(vymName,
                               tr("The file %1 exists already.\n"
                                  "Do you want to overwrite it?")
                                   .arg(fn),
                               QMessageBox::Warning,
                               QMessageBox::Yes | QMessageBox::Default,
                               QMessageBox::Cancel | QMessageBox::Escape,
                               QMessageBox::NoButton);

                mb.setButtonText(QMessageBox::Yes, tr("Overwrite"));
                mb.setButtonText(QMessageBox::No, tr("Cancel"));
                switch (mb.exec()) {
                case QMessageBox::Yes:
                    // save
                    break;
                case QMessageBox::Cancel:
                    // do nothing
                    return;
                    break;
                }
            }
            if (!ii->saveImage(fn))
                QMessageBox::critical(0, tr("Critical Error"),
                                      tr("Couldn't save %1").arg(fn));
        }
    }
}

void VymModel::importDirInt(BranchItem *dst, QDir d)
{
    bool oldSaveState = saveStateBlocked;
    saveStateBlocked = true;
    BranchItem *bi = dst;
    if (bi) {
        int beginDepth = bi->depth();

        d.setFilter(QDir::AllEntries | QDir::Hidden);
        QFileInfoList list = d.entryInfoList();
        QFileInfo fi;

        // Traverse directories
        for (int i = 0; i < list.size(); ++i) {
            fi = list.at(i);
            if (fi.isDir() && fi.fileName() != "." && fi.fileName() != "..") {
                bi = addNewBranchInt(dst, -2);
                bi->setHeadingPlainText(fi.fileName());
                bi->setHeadingColor(QColor("blue"));
                if (debug)
                    qDebug() << "Added subdir: " << fi.fileName();
                if (!d.cd(fi.fileName()))
                    QMessageBox::critical(
                        0, tr("Critical Import Error"),
                        tr("Cannot find the directory %1").arg(fi.fileName()));
                else {
                    // Recursively add subdirs
                    importDirInt(bi, d);
                    d.cdUp();
                }
                emitDataChanged(bi);
            }
        }

        for (int i = 0; i < list.size(); ++i) {
            fi = list.at(i);
            if (fi.isFile()) {
                bi = addNewBranchInt(dst, -2);
                bi->setHeadingPlainText(fi.fileName());
                bi->setHeadingColor(QColor("black"));
                if (fi.fileName().right(4) == ".vym")
                    bi->setVymLink(fi.filePath());
                emitDataChanged(bi);
            }
        }

        // Scroll at least some stuff
        if (dst->branchCount() > 1 && dst->depth() - beginDepth > 2)
            dst->toggleScroll();
    }
    saveStateBlocked = oldSaveState;
}

void VymModel::importDir(const QString &s)
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi) {
        saveStateChangingPart(
            selbi, selbi, QString("importDir (\"%1\")").arg(s),
            QString("Import directory structure from %1").arg(s));

        QDir d(s);
        importDirInt(selbi, d);
    }
}

void VymModel::importDir()
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi) {
        QStringList filters;
        filters << "VYM map (*.vym)";
        QFileDialog fd;
        fd.setWindowTitle(vymName + " - " +
                          tr("Choose directory structure to import"));
        fd.setFileMode(QFileDialog::DirectoryOnly);
        fd.setNameFilters(filters);
        fd.setWindowTitle(vymName + " - " +
                          tr("Choose directory structure to import"));
        fd.setAcceptMode(QFileDialog::AcceptOpen);

        QString fn;
        if (fd.exec() == QDialog::Accepted && !fd.selectedFiles().isEmpty()) {
            importDir(fd.selectedFiles().first());
            reposition();
        }
    }
}

bool VymModel::removeVymLock()
{
    if (vymLock.removeLock()) {
        mainWindow->statusMessage(tr("Removed lockfile for %1").arg(mapName));
        setReadOnly(false);
        return true;
    }
    else {
        QMessageBox::warning(
            0, tr("Warning"),
            tr("Couldn't remove lockfile for %1").arg(mapName));
        return false;
    }
}

bool VymModel::tryVymLock()
{
    // Defaults for author and host in vymLock
    QString defAuthor =
        settings
            .value("/user/name",
                   tr("unknown user", "Default for lockfiles of maps"))
            .toString();
    QString defHost = QHostInfo::localHostName();
    vymLock.setMapPath(filePath);
    vymLock.setAuthor(settings.value("/user/name", defAuthor).toString());
    if (getenv("HOST") != 0)
        vymLock.setHost(getenv("HOST"));
    else
        vymLock.setHost(defHost);

    // Now try to lock
    if (!vymLock.tryLock()) {
        if (debug)
            qDebug() << "VymModel::tryLock failed!";
        setReadOnly(true);
        if (vymLock.getState() == VymLock::lockedByOther) {
            if (recoveryMode) {
                ignoredLockedFiles << filePath;
                return removeVymLock();
            }
            else {
                LockedFileDialog dia;
                QString a = vymLock.getAuthor();
                QString h = vymLock.getHost();
                QString s =
                    QString(
                        tr("Map seems to be already opened in another vym "
                           "instance!\n\n "
                           "Map is locked by \"%1\" on \"%2\"\n\n"
                           "Please only delete the lockfile, if you are sure "
                           "nobody else is currently working on this map."))
                        .arg(a)
                        .arg(h);
                dia.setText(s);
                dia.setWindowTitle(
                    tr("Warning: Map already opended", "VymModel"));
                if (dia.execDialog() == LockedFileDialog::DeleteLockfile) {
                    return removeVymLock();
                }
            }
        }
        else if (vymLock.getState() == VymLock::notWritable) {
            WarningDialog dia;
            QString s = QString(tr("Cannot create lockfile of map! "
                                   "It will be opened in readonly mode.\n\n"));
            dia.setText(s);
            dia.setWindowTitle(tr("Warning", "VymModel"));
            dia.showCancelButton(false);
            // dia.setShowAgainName("/mainwindow/mapIsLocked");
            dia.exec();
        }
        return false;
    }
    return true;
}

bool VymModel::renameMap(const QString &newPath)
{
    QString oldPath = filePath;
    setFilePath(newPath);
    if (vymLock.getState() == VymLock::lockedByMyself) {
        // vymModel owns the lockfile, try to rename it
        if (!vymLock.rename(fileName)) {
            qWarning("Warning: VymModel::renameMap failed");
            setFilePath(oldPath);
            return false;
        }
        else
            return true;
    }

    // try to create new lockfile for the lock states: lockedByOther and
    // notWritable
    return tryVymLock();
}

void VymModel::setReadOnly(bool b)
{
    readonly = b;
    mainWindow->updateTabName(this);
}

bool VymModel::isReadOnly() { return readonly; }

void VymModel::autosave()
{
    // Check if autosave is disabled due to testmode
    if (testmode)
    {
        qWarning()
            << QString("VymModel::autosave disabled in testmode!  Current map: %1")
                   .arg(filePath);
        return;
    }

    // Check if autosave is disabled globally
    if (!mainWindow->useAutosave()) {
        qWarning()
            << QString("VymModel::autosave disabled globally!  Current map: %1")
                   .arg(filePath);
        return;
    }

    QDateTime now = QDateTime().currentDateTime();

    // Disable autosave, while we have gone back in history
    int redosAvail = undoSet.numValue(QString("/history/redosAvail"));
    if (redosAvail > 0)
        return;

    // Also disable autosave for new map without filename
    if (filePath.isEmpty()) {
        if (debug)
            qWarning()
                << "VymModel::autosave rejected due to missing filePath\n";
        return;
    }

    if (mapUnsaved && mapChanged && mainWindow->useAutosave() && !testmode) {
        if (QFileInfo(filePath).lastModified() <= fileChangedTime)
            mainWindow->fileSave(this);
        else if (debug)
            qDebug() << "  ME::autosave  rejected, file on disk is newer than "
                        "last save.\n";
    }
}

void VymModel::fileChanged()
{
    // Check if file on disk has changed meanwhile
    if (!filePath.isEmpty()) {
        if (readonly) {
            // unset readonly if lockfile is gone
            if (vymLock.tryLock())
                setReadOnly(false);
        }
        else {
            // We could check, if somebody else removed/replaced lockfile
            // (A unique vym ID would be needed)

            QDateTime tmod = QFileInfo(filePath).lastModified();
            if (tmod > fileChangedTime) {
                // FIXME-4 VM switch to current mapeditor and finish
                // lineedits...
                QMessageBox mb(
                    vymName,
                    tr("The file of the map  on disk has changed:\n\n"
                       "   %1\n\nDo you want to reload that map with the new "
                       "file?")
                        .arg(filePath),
                    QMessageBox::Question, QMessageBox::Yes,
                    QMessageBox::Cancel | QMessageBox::Default,
                    QMessageBox::NoButton);

                mb.setButtonText(QMessageBox::Yes, tr("Reload"));
                mb.setButtonText(QMessageBox::No, tr("Ignore"));
                switch (mb.exec()) {
                case QMessageBox::Yes:
                    // Reload map
                    mainWindow->initProgressCounter(1);
                    loadMap(filePath);
                    mainWindow->removeProgressCounter();
                    break;
                case QMessageBox::Cancel:
                    fileChangedTime =
                        tmod; // allow autosave to overwrite newer file!
                }
            }
        }
    }
}

void VymModel::blockReposition()
{
    repositionBlocked = true;
}

void VymModel::unblockReposition()
{
    repositionBlocked = false;
    reposition();
}

bool VymModel::isDefault() { return mapDefault; }

void VymModel::makeDefault()
{
    mapChanged = false;
    mapDefault = true;
}

bool VymModel::hasChanged() { return mapChanged; }

void VymModel::setChanged()
{
    if (!mapChanged)
        autosaveTimer->start(
            settings.value("/system/autosave/ms/", 30000).toInt());
    mapChanged = true;
    mapDefault = false;
    mapUnsaved = true;
    updateActions();
}

QString VymModel::getObjectName(LinkableMapObj *lmo)
{
    if (!lmo || !lmo->getTreeItem())
        return QString();
    return getObjectName(lmo->getTreeItem());
}

QString VymModel::getObjectName(TreeItem *ti)
{
    QString s;
    if (!ti)
        return QString("Error: NULL has no name!");
    s = ti->getHeadingPlain();
    if (s == "")
        s = "unnamed";

    return QString("%1 (%2)").arg(ti->getTypeName()).arg(s);
}

void VymModel::redo()
{
    // Can we undo at all?
    if (redosAvail < 1)
        return;

    bool saveStateBlockedOrg = saveStateBlocked;
    saveStateBlocked = true;

    redosAvail--;

    if (undosAvail < stepsTotal)
        undosAvail++;
    curStep++;
    if (curStep > stepsTotal)
        curStep = 1;
    QString undoCommand =
        undoSet.value(QString("/history/step-%1/undoCommand").arg(curStep));
    QString undoSelection =
        undoSet.value(QString("/history/step-%1/undoSelection").arg(curStep));
    QString redoCommand =
        undoSet.value(QString("/history/step-%1/redoCommand").arg(curStep));
    QString redoSelection =
        undoSet.value(QString("/history/step-%1/redoSelection").arg(curStep));
    QString comment =
        undoSet.value(QString("/history/step-%1/comment").arg(curStep));
    QString version = undoSet.value("/history/version");

    /* TODO Maybe check for version, if we save the history
    if (!checkVersion(version))
    QMessageBox::warning(0,tr("Warning"),
        tr("Version %1 of saved undo/redo data\ndoes not match current vym
    version %2.").arg(version).arg(vymVersion));
    */

    // Find out current undo directory
    QString bakMapDir(QString(tmpMapDirPath + "/undo-%1").arg(curStep));

    if (debug) {
        qDebug() << "VymModel::redo() begin\n";
        qDebug() << "    undosAvail=" << undosAvail;
        qDebug() << "    redosAvail=" << redosAvail;
        qDebug() << "       curStep=" << curStep;
        qDebug() << "    ---------------------------";
        qDebug() << "    comment=" << comment;
        qDebug() << "    undoSel=" << undoSelection;
        qDebug() << "    redoSel=" << redoSelection;
        qDebug() << "    undoCom:";
        cout << qPrintable(undoCommand);
        qDebug() << "    redoCom=";
        cout << qPrintable(redoCommand);
        qDebug() << "    ---------------------------";
    }

    // select  object before redo
    if (!redoSelection.isEmpty())
        select(redoSelection);

    QString errMsg;
    QString redoScript =
        QString("model = vym.currentMap(); model.%1").arg(redoCommand);
    errMsg = QVariant(execute(redoScript)).toString();
    saveStateBlocked = saveStateBlockedOrg;

    undoSet.setValue("/history/undosAvail", QString::number(undosAvail));
    undoSet.setValue("/history/redosAvail", QString::number(redosAvail));
    undoSet.setValue("/history/curStep", QString::number(curStep));
    undoSet.writeSettings(histPath);

    mainWindow->updateHistory(undoSet);
    updateActions();

    /* TODO remove testing
    qDebug() << "ME::redo() end\n";
    qDebug() << "    undosAvail="<<undosAvail;
    qDebug() << "    redosAvail="<<redosAvail;
    qDebug() << "       curStep="<<curStep;
    qDebug() << "    ---------------------------";
    */
}

bool VymModel::isRedoAvailable()
{
    if (undoSet.numValue("/history/redosAvail", 0) > 0)
        return true;
    else
        return false;
}

QString VymModel::lastRedoSelection()
{
    if (isUndoAvailable())
        return undoSet.value(
            QString("/history/step-%1/redoSelection").arg(curStep));
    else
        return QString();
}

QString VymModel::lastRedoCommand()
{
    if (isUndoAvailable())
        return undoSet.value(
            QString("/history/step-%1/redoCommand").arg(curStep));
    else
        return QString();
}

QVariant VymModel::repeatLastCommand()
{
    QString command = "m = vym.currentMap();";
    if (isUndoAvailable())
        command += "m." +
                   undoSet.value(
                       QString("/history/step-%1/redoCommand").arg(curStep)) +
                   ";";
    else
        return false;
    return execute(command);
}

void VymModel::undo()
{
    // Can we undo at all?
    if (undosAvail < 1)
        return;

    mainWindow->statusMessage(tr("Autosave disabled during undo."));

    bool saveStateBlockedOrg = saveStateBlocked;
    saveStateBlocked = true;

    QString undoCommand =
        undoSet.value(QString("/history/step-%1/undoCommand").arg(curStep));
    QString undoSelection =
        undoSet.value(QString("/history/step-%1/undoSelection").arg(curStep));
    QString redoCommand =
        undoSet.value(QString("/history/step-%1/redoCommand").arg(curStep));
    QString redoSelection =
        undoSet.value(QString("/history/step-%1/redoSelection").arg(curStep));
    QString comment =
        undoSet.value(QString("/history/step-%1/comment").arg(curStep));
    QString version = undoSet.value("/history/version");

    /* TODO Maybe check for version, if we save the history
    if (!checkVersion(version))
    QMessageBox::warning(0,tr("Warning"),
        tr("Version %1 of saved undo/redo data\ndoes not match current vym
    version %2.").arg(version).arg(vymVersion));
    */

    // Find out current undo directory
    QString bakMapDir(QString(tmpMapDirPath + "/undo-%1").arg(curStep));

    // select  object before undo
    if (!undoSelection.isEmpty() && !select(undoSelection)) {
        qWarning("VymModel::undo()  Could not select object for undo");
        return;
    }

    if (debug) {
        qDebug() << "VymModel::undo() begin\n";
        qDebug() << "    undosAvail=" << undosAvail;
        qDebug() << "    redosAvail=" << redosAvail;
        qDebug() << "       curStep=" << curStep;
        cout << "    ---------------------------" << endl;
        qDebug() << "    comment=" << comment;
        qDebug() << "    undoSel=" << undoSelection;
        qDebug() << "    redoSel=" << redoSelection;
        cout << "    undoCom:" << endl;
        cout << qPrintable(undoCommand) << endl;
        cout << "    redoCom:" << endl;
        cout << qPrintable(redoCommand) << endl;
        cout << "    ---------------------------" << endl;
    }

    // select  object before undo   // FIXME-2 double select, see above
    if (!undoSelection.isEmpty())
        select(undoSelection);

    // bool noErr;
    QString errMsg;
    QString undoScript =
        QString("model = vym.currentMap(); model.%1").arg(undoCommand);
    errMsg = QVariant(execute(undoScript)).toString();

    undosAvail--;
    curStep--;
    if (curStep < 1)
        curStep = stepsTotal;

    redosAvail++;

    saveStateBlocked = saveStateBlockedOrg;
    /* testing only
        qDebug() << "VymModel::undo() end\n";
        qDebug() << "    undosAvail="<<undosAvail;
        qDebug() << "    redosAvail="<<redosAvail;
        qDebug() << "       curStep="<<curStep;
        qDebug() << "    ---------------------------";
    */

    undoSet.setValue("/history/undosAvail", QString::number(undosAvail));
    undoSet.setValue("/history/redosAvail", QString::number(redosAvail));
    undoSet.setValue("/history/curStep", QString::number(curStep));
    undoSet.writeSettings(histPath);

    mainWindow->updateHistory(undoSet);
    updateActions();
}

bool VymModel::isUndoAvailable()
{
    if (undoSet.numValue("/history/undosAvail", 0) > 0)
        return true;
    return false;
}

void VymModel::gotoHistoryStep(int i)
{
    // Restore variables
    int undosAvail = undoSet.numValue(QString("/history/undosAvail"));
    int redosAvail = undoSet.numValue(QString("/history/redosAvail"));

    if (i < 0)
        i = undosAvail + redosAvail;

    // Clicking above current step makes us undo things
    if (i < undosAvail) {
        for (int j = 0; j < undosAvail - i; j++)
            undo();
        return;
    }
    // Clicking below current step makes us redo things
    if (i > undosAvail)
        for (int j = undosAvail; j < i; j++) {
            if (debug)
                qDebug() << "VymModel::gotoHistoryStep redo " << j << "/"
                         << undosAvail << " i=" << i;
            redo();
        }

    // And ignore clicking the current row ;-)
}

QString VymModel::getHistoryPath()
{
    QString histName(QString("history-%1").arg(curStep));
    return (tmpMapDirPath + "/" + histName);
}

void VymModel::resetHistory()
{
    curStep = 0;
    redosAvail = 0;
    undosAvail = 0;

    stepsTotal = settings.value("/history/stepsTotal", 100).toInt();
    undoSet.setValue("/history/stepsTotal", QString::number(stepsTotal));
    mainWindow->updateHistory(undoSet);
}

void VymModel::saveState(const SaveMode &savemode, const QString &undoSelection,
                         const QString &undoCom, const QString &redoSelection,
                         const QString &redoCom, const QString &comment,
                         TreeItem *saveSel, QString dataXML)
{
    sendData(redoCom); // FIXME-4 testing

    // Main saveState

    if (saveStateBlocked)
        return;

    if (debug)
        qDebug() << "VM::saveState() for map " << mapName;

    QString undoCommand = undoCom;
    QString redoCommand = redoCom;


    // Increase undo steps, but check for repeated actions
    // like editing a vymNote - then do not increase but replace last command
    /*
    QRegExp re ("parseVymText.*\\(.*vymnote");
    if (curStep > 0 && redoSelection == lastRedoSelection() &&
        lastRedoCommand().contains(re)) {
        undoCommand = undoSet.value(
            QString("/history/step-%1/undoCommand").arg(curStep), undoCommand);
    }
    else {
    */
        if (undosAvail < stepsTotal)
            undosAvail++;

        curStep++;
        if (curStep > stepsTotal)
            curStep = 1;
    //}

    QString histDir = getHistoryPath();
    QString bakMapPath = histDir + "/map.xml";

    // Create histDir if not available
    QDir d(histDir);
    if (!d.exists())
        makeSubDirs(histDir);

    // Save depending on how much needs to be saved
    QList<Link *> tmpLinks;
    if (saveSel)
        dataXML = saveToDir(histDir, mapName + "-", FlagRowMaster::NoFlags, QPointF(),
                            saveSel);

    if (savemode == PartOfMap) {
        undoCommand.replace("PATH", bakMapPath);
        redoCommand.replace("PATH", bakMapPath);
    }

    if (!dataXML.isEmpty())
        // Write XML Data to disk
        saveStringToDisk(bakMapPath, dataXML);

    // We would have to save all actions in a tree, to keep track of
    // possible redos after a action. Possible, but we are too lazy: forget
    // about redos.
    redosAvail = 0;

    // Write the current state to disk
    undoSet.setValue("/history/undosAvail", QString::number(undosAvail));
    undoSet.setValue("/history/redosAvail", QString::number(redosAvail));
    undoSet.setValue("/history/curStep", QString::number(curStep));
    undoSet.setValue(QString("/history/step-%1/undoCommand").arg(curStep),
                     undoCommand);
    undoSet.setValue(QString("/history/step-%1/undoSelection").arg(curStep),
                     undoSelection);
    undoSet.setValue(QString("/history/step-%1/redoCommand").arg(curStep),
                     redoCommand);
    undoSet.setValue(QString("/history/step-%1/redoSelection").arg(curStep),
                     redoSelection);
    undoSet.setValue(QString("/history/step-%1/comment").arg(curStep), comment);
    undoSet.setValue(QString("/history/version"), vymVersion);
    undoSet.writeSettings(histPath);

    if (debug) {
        // qDebug() << "          into="<< histPath;
        qDebug() << "    stepsTotal=" << stepsTotal
                 << ", undosAvail=" << undosAvail
                 << ", redosAvail=" << redosAvail << ", curStep=" << curStep;
        cout << "    ---------------------------" << endl;
        qDebug() << "    comment=" << comment;
        qDebug() << "    undoSel=" << undoSelection;
        qDebug() << "    redoSel=" << redoSelection;
        if (saveSel)
            qDebug() << "    saveSel=" << qPrintable(getSelectString(saveSel));
        cout << "    undoCom:" << endl;
        cout << qPrintable(undoCommand) << endl;
        cout << "    redoCom:" << endl;
        cout << qPrintable(redoCommand) << endl;
        cout << "    ---------------------------" << endl;
    }

    mainWindow->updateHistory(undoSet);

    setChanged();
}

void VymModel::saveStateChangingPart(TreeItem *undoSel, TreeItem *redoSel,
                                     const QString &rc, const QString &comment)
{
    // save the selected part of the map, Undo will replace part of map
    QString undoSelection = "";
    if (undoSel)
        undoSelection = getSelectString(undoSel);
    else
        qWarning("VymModel::saveStateChangingPart  no undoSel given!");
    QString redoSelection = "";
    if (redoSel)
        redoSelection = getSelectString(undoSel);
    else
        qWarning("VymModel::saveStateChangingPart  no redoSel given!");

    saveState(PartOfMap, undoSelection, "addMapReplace (\"PATH\")",
              redoSelection, rc, comment, undoSel);
}

void VymModel::saveStateRemovingPart(TreeItem *redoSel, const QString &comment)
{
    if (!redoSel) {
        qWarning("VymModel::saveStateRemovingPart  no redoSel given!");
        return;
    }
    QString undoSelection;
    QString redoSelection = getSelectString(redoSel);
    if (redoSel->isBranchLikeType()) {
        // save the selected branch of the map, Undo will insert part of map
        if (redoSel->depth() > 0)
            undoSelection = getSelectString(redoSel->parent());
        saveState(PartOfMap, undoSelection,
                  QString("addMapInsert (\"PATH\",%1,%2)")
                      .arg(redoSel->num())
                      .arg(SlideContent),
                  redoSelection, "remove ()", comment, redoSel);
    }
}

void VymModel::saveState(TreeItem *undoSel, const QString &uc,
                         TreeItem *redoSel, const QString &rc,
                         const QString &comment)
{
    // "Normal" savestate: save commands, selections and comment
    // so just save commands for undo and redo
    // and use current selection, if empty parameter passed

    QString redoSelection = "";
    if (redoSel)
        redoSelection = getSelectString(redoSel);
    QString undoSelection = "";
    if (undoSel)
        undoSelection = getSelectString(undoSel);

    saveState(UndoCommand, undoSelection, uc, redoSelection, rc, comment, NULL);
}

void VymModel::saveState(const QString &undoSel, const QString &uc,
                         const QString &redoSel, const QString &rc,
                         const QString &comment)
{
    // "Normal" savestate: save commands, selections and comment
    // so just save commands for undo and redo
    // and use current selection
    saveState(UndoCommand, undoSel, uc, redoSel, rc, comment, NULL);
}

void VymModel::saveState(const QString &uc, const QString &rc,
                         const QString &comment)
{
    // "Normal" savestate applied to model (no selection needed):
    // save commands  and comment
    saveState(UndoCommand, NULL, uc, NULL, rc, comment, NULL);
}

void VymModel::saveStateMinimal(TreeItem *undoSel, const QString &uc,
                                TreeItem *redoSel, const QString &rc,
                                const QString &comment)
{ //  Save a change in string and merge
    //  minor sequential  changes  */
    QString redoSelection = "";
    if (redoSel)
        redoSelection = getSelectString(redoSel);
    QString undoSelection = "";
    if (undoSel)
        undoSelection = getSelectString(undoSel);

    saveState(UndoCommand, undoSelection, uc, redoSelection, rc, comment, NULL);
}

void VymModel::saveStateBeforeLoad(LoadMode lmode, const QString &fname)
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi) {
        if (lmode == ImportAdd)
            saveStateChangingPart(selbi, selbi,
                                  QString("addMapInsert (\"%1\")").arg(fname),
                                  QString("Add map %1 to %2")
                                      .arg(fname)
                                      .arg(getObjectName(selbi)));
        if (lmode == ImportReplace) {
            BranchItem *pi = (BranchItem *)(selbi->parent());
            saveStateChangingPart(pi, pi,
                                  QString("addMapReplace(%1)").arg(fname),
                                  QString("Add map %1 to %2")
                                      .arg(fname)
                                      .arg(getObjectName(selbi)));
        }
    }
}

QGraphicsScene *VymModel::getScene() { return mapEditor->getScene(); }

TreeItem *VymModel::findBySelectString(QString s)
{
    if (s.isEmpty())
        return NULL;

    // Old maps don't have multiple mapcenters and don't save full path
    if (s.left(2) != "mc")
        s = "mc:0," + s;

    QStringList parts = s.split(",");
    QString typ;
    int n;
    TreeItem *ti = rootItem;

    while (!parts.isEmpty()) {
        typ = parts.first().left(2);
        n = parts.first().right(parts.first().length() - 3).toInt();
        parts.removeFirst();
        if (typ == "mc" || typ == "bo")
            ti = ti->getBranchNum(n);
        else if (typ == "fi")
            ti = ti->getImageNum(n);
        else if (typ == "ai")
            ti = ti->getAttributeNum(n);
        else if (typ == "xl")
            ti = ti->getXLinkItemNum(n);
        if (!ti)
            return NULL;
    }
    return ti;
}

TreeItem *VymModel::findID(const uint &id)
{
    BranchItem *cur = NULL;
    BranchItem *prev = NULL;
    nextBranch(cur, prev);
    while (cur) {
        if (id == cur->getID())
            return cur;
        int j = 0;
        while (j < cur->xlinkCount()) {
            XLinkItem *xli = cur->getXLinkItemNum(j);
            if (id == xli->getID())
                return xli;
            j++;
        }
        j = 0;
        while (j < cur->imageCount()) {
            ImageItem *ii = cur->getImageNum(j);
            if (id == ii->getID())
                return ii;
            j++;
        }
        nextBranch(cur, prev);
    }
    return NULL;
}

TreeItem *VymModel::findUuid(const QUuid &id)
{
    BranchItem *cur = NULL;
    BranchItem *prev = NULL;
    nextBranch(cur, prev);
    while (cur) {
        if (id == cur->getUuid())
            return cur;
        int j = 0;
        while (j < cur->xlinkCount()) {
            XLinkItem *xli = cur->getXLinkItemNum(j);
            if (id == xli->getUuid())
                return xli;
            j++;
        }
        j = 0;
        while (j < cur->imageCount()) {
            ImageItem *ii = cur->getImageNum(j);
            if (id == ii->getUuid())
                return ii;
            j++;
        }
        nextBranch(cur, prev);
    }
    return NULL;
}

//////////////////////////////////////////////
// Interface
//////////////////////////////////////////////
void VymModel::setVersion(const QString &s) { version = s; }

QString VymModel::getVersion() { return version; }

void VymModel::setTitle(const QString &s)
{
    saveState(QString("setMapTitle (\"%1\")").arg(title),
              QString("setMapTitle (\"%1\")").arg(s),
              QString("Set title of map to \"%1\"").arg(s));
    title = s;
}

QString VymModel::getTitle() { return title; }

void VymModel::setAuthor(const QString &s)
{
    saveState(QString("setMapAuthor (\"%1\")").arg(author),
              QString("setMapAuthor (\"%1\")").arg(s),
              QString("Set author of map to \"%1\"").arg(s));
    author = s;
}

QString VymModel::getAuthor() { return author; }

void VymModel::setComment(const QString &s)
{
    saveState(QString("setMapComment (\"%1\")").arg(comment),
              QString("setMapComment (\"%1\")").arg(s),
              QString("Set comment of map"));
    comment = s;
}

QString VymModel::getComment() { return comment; }

QString VymModel::getDate()
{
    return QDate::currentDate().toString("yyyy-MM-dd");
}

int VymModel::branchCount()
{
    int c = 0;
    BranchItem *cur = NULL;
    BranchItem *prev = NULL;
    nextBranch(cur, prev);
    while (cur) {
        c++;
        nextBranch(cur, prev);
    }
    return c;
}

int VymModel::centerCount() { return rootItem->branchCount(); }

void VymModel::setSortFilter(const QString &s)
{
    sortFilter = s;
    emit(sortFilterChanged(sortFilter));
}

QString VymModel::getSortFilter() { return sortFilter; }

void VymModel::setHeading(const VymText &vt, BranchItem *bi)
{
    Heading h_old;
    Heading h_new;
    h_new = vt;
    QString s = vt.getTextASCII();
    if (!bi)
        bi = getSelectedBranch();
    if (bi) {
        h_old = bi->getHeading();
        if (h_old == h_new)
            return;
        saveState(bi, "parseVymText (\"" + quoteQuotes(h_old.saveToDir()) + "\")", bi,
                  "parseVymText (\"" + quoteQuotes(h_new.saveToDir()) + "\")",
                  QString("Set heading of %1 to \"%2\"")
                      .arg(getObjectName(bi))
                      .arg(s));
        bi->setHeading(vt);
        emitDataChanged(bi);
        emitUpdateQueries();
        reposition();
    }
}

void VymModel::setHeadingPlainText(const QString &s, BranchItem *bi)
{
    if (!bi)
        bi = getSelectedBranch();
    if (bi) {
        VymText vt = bi->getHeading();
        vt.setPlainText(s);
        if (bi->getHeading() == vt)
            return;
        setHeading(vt, bi);

        // Set URL
        if ((s.startsWith("http://") || s.startsWith("https://")) &&
            bi->getURL().isEmpty())
            setURL(s);
    }
}

Heading VymModel::getHeading()
{
    TreeItem *selti = getSelectedItem();
    if (selti)
        return selti->getHeading();
    qWarning() << "VymModel::getHeading Nothing selected.";
    return Heading();
}

void VymModel::updateNoteText(const VymText &vt)
{
    bool editorStateChanged = false;

    TreeItem *selti = getSelectedItem();
    if (selti) {
        VymNote note_old = selti->getNote();
        VymNote note_new(vt);
        if (note_new.getText() != note_old.getText()) {
            if ((note_new.isEmpty() && !note_old.isEmpty()) ||
                (!note_new.isEmpty() && note_old.isEmpty()))
                editorStateChanged = true;

            VymNote vn;
            vn.copy(vt);

            saveState(selti, "parseVymText (\"" + quoteQuotes(note_old.saveToDir()) + "\")",
                      selti, "parseVymText (\"" + quoteQuotes(note_new.saveToDir()) + "\")",
                      QString("Set note of %1 to \"%2\"")
                          .arg(getObjectName(selti))
                          .arg(note_new.getTextASCII().left(20)));

            selti->setNote(vn);
        }

        // Update also flags after changes in NoteEditor
        emitDataChanged(selti);

        // Only update flag, if state has changed
        if (editorStateChanged)
            reposition();
    }
}

void VymModel::setNote(const VymNote &vn)
{
    TreeItem *selti = getSelectedItem();
    if (selti) {
        VymNote n_old;
        VymNote n_new;
        n_old = selti->getNote();
        n_new = vn;
        saveState(selti, "parseVymText (\"" + quoteQuotes(n_old.saveToDir()) + "\")", selti,
                  "parseVymText (\"" + quoteQuotes(n_new.saveToDir()) + "\")",
                  QString("Set note of %1 to \"%2\"")
                      .arg(getObjectName(selti))
                      .arg(n_new.getTextASCII().left(40)));
        selti->setNote(n_new);
        emitNoteChanged(selti);
        emitDataChanged(selti);
    }
}

VymNote VymModel::getNote()
{
    TreeItem *selti = getSelectedItem();
    if (selti) {
        VymNote n = selti->getNote();
        return n;
    }
    qWarning() << "VymModel::getNote Nothing selected.";
    return VymNote();
}

bool VymModel::hasRichTextNote()
{
    TreeItem *selti = getSelectedItem();
    if (selti) {
        return selti->getNote().isRichText();
    }
    return false;
}

void VymModel::loadNote(const QString &fn)
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi) {
        QString n;
        if (!loadStringFromDisk(fn, n))
            qWarning() << "VymModel::loadNote Couldn't load " << fn;
        else {
            VymNote vn;
            vn.setAutoText(n);
            setNote(vn);
            emitDataChanged(selbi);
            emitUpdateQueries();
            reposition();
        }
    }
    else
        qWarning("VymModel::loadNote no branch selected");
}

void VymModel::saveNote(const QString &fn)
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi) {
        VymNote n = selbi->getNote();
        if (n.isEmpty())
            qWarning() << "VymModel::saveNote  note is empty, won't save to "
                       << fn;
        else {
            if (!saveStringToDisk(fn, n.saveToDir()))
                qWarning() << "VymModel::saveNote Couldn't save " << fn;
        }
    }
    else
        qWarning("VymModel::saveNote no branch selected");
}

void VymModel::findDuplicateURLs() // FIXME-3 needs GUI
{
    // Generate map containing _all_ URLs and branches
    QString u;
    QMap<QString, BranchItem *> map;
    BranchItem *cur = NULL;
    BranchItem *prev = NULL;
    nextBranch(cur, prev);
    while (cur) {
        u = cur->getURL();
        if (!u.isEmpty())
            map.insertMulti(u, cur);
        nextBranch(cur, prev);
    }

    // Extract duplicate URLs
    QMap<QString, BranchItem *>::const_iterator i = map.constBegin();
    QMap<QString, BranchItem *>::const_iterator firstdup =
        map.constEnd(); // invalid
    while (i != map.constEnd()) {
        if (i != map.constBegin() && i.key() == firstdup.key()) {
            if (i - 1 == firstdup) {
                qDebug() << firstdup.key();
                qDebug() << " - " << firstdup.value() << " - "
                         << firstdup.value()->getHeading().getText();
            }
            qDebug() << " - " << i.value() << " - "
                     << i.value()->getHeading().getText();
        }
        else
            firstdup = i;

        ++i;
    }
}

bool VymModel::findAll(FindResultModel *rmodel, QString s,
                       Qt::CaseSensitivity cs, bool searchNotes)
{
    rmodel->clear();
    rmodel->setSearchString(s);
    rmodel->setSearchFlags(0); // FIXME-4 translate cs to
                               // QTextDocument::FindFlag
    bool hit = false;

    BranchItem *cur = NULL;
    BranchItem *prev = NULL;
    nextBranch(cur, prev);

    FindResultItem *lastParent = NULL;
    while (cur) {
        lastParent = NULL;
        if (cur->getHeading().getTextASCII().contains(s, cs)) {
            lastParent = rmodel->addItem(cur);
            hit = true;
        }

        if (searchNotes) {
            QString n = cur->getNoteASCII();
            int i = 0;
            int j = 0;
            while (i >= 0) {
                i = n.indexOf(s, i, cs);
                if (i >= 0) {
                    // If not there yet, add "parent" item
                    if (!lastParent) {
                        lastParent = rmodel->addItem(cur);
                        hit = true;
                        if (!lastParent)
                            qWarning()
                                << "VymModel::findAll still no lastParent?!";
                        /*
                        else
                            lastParent->setSelectable (false);
                        */
                    }

                    // save index of occurence
                    QString e = n.mid(i - 15, 30);
                    n.replace('\n', ' ');
                    rmodel->addSubItem(
                        lastParent,
                        QString(tr("Note", "FindAll in VymModel") +
                                ": \"...%1...\"")
                            .arg(n.mid(i - 8, 80)),
                        cur, j);
                    j++;
                    i++;
                }
            }
        }
        nextBranch(cur, prev);
    }
    return hit;
}

void VymModel::setURL(QString url)
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi->getURL() == url)
        return;
    if (selbi) {
        QString oldurl = selbi->getURL();
        selbi->setURL(url);
        saveState(
            selbi, QString("setURL (\"%1\")").arg(oldurl), selbi,
            QString("setURL (\"%1\")").arg(url),
            QString("set URL of %1 to %2").arg(getObjectName(selbi)).arg(url));
        emitDataChanged(selbi);
        reposition();

        // Check for Confluence
        setHeadingConfluencePageName();
    }
}

QString VymModel::getURL()
{
    TreeItem *selti = getSelectedItem();
    if (selti)
        return selti->getURL();
    else
        return QString();
}

QStringList VymModel::getURLs(bool ignoreScrolled)
{
    QStringList urls;
    BranchItem *selbi = getSelectedBranch();
    BranchItem *cur = NULL;
    BranchItem *prev = NULL;
    nextBranch(cur, prev, true, selbi);
    while (cur) {
        if (!cur->getURL().isEmpty() &&
            !(ignoreScrolled && cur->hasScrolledParent()))
            urls.append(cur->getURL());
        nextBranch(cur, prev, true, selbi);
    }
    return urls;
}

void VymModel::setFrameType(const FrameObj::FrameType &t)
{
    BranchItem *bi = getSelectedBranch();
    if (bi) {
        BranchObj *bo = (BranchObj *)(bi->getLMO());
        if (bo) {
            QString s = bo->getFrameTypeName();
            bo->setFrameType(t);
            saveState(
                bi, QString("setFrameType (\"%1\")").arg(s), bi,
                QString("setFrameType (\"%1\")").arg(bo->getFrameTypeName()),
                QString("set type of frame to %1").arg(s));
            reposition();
            bo->updateLinkGeometry();
        }
    }
}

void VymModel::setFrameType(const QString &s)
{
    BranchItem *bi = getSelectedBranch();
    if (bi) {
        BranchObj *bo = (BranchObj *)(bi->getLMO());
        if (bo) {
            saveState(
                bi,
                QString("setFrameType (\"%1\")").arg(bo->getFrameTypeName()),
                bi, QString("setFrameType (\"%1\")").arg(s),
                QString("set type of frame to %1").arg(s));
            bo->setFrameType(s);
            reposition();
            bo->updateLinkGeometry();
        }
    }
}

void VymModel::toggleFrameIncludeChildren()
{
    BranchItem *bi = getSelectedBranch();
    if (bi) {
        bool b = bi->getFrameIncludeChildren();
        setFrameIncludeChildren(!b);
    }
}

void VymModel::setFrameIncludeChildren(bool b)
{
    BranchItem *bi = getSelectedBranch();
    if (bi) {
        QString u = b ? "false" : "true";
        QString r = !b ? "false" : "true";

        saveState(bi, QString("setFrameIncludeChildren(%1)").arg(u), bi,
                  QString("setFrameIncludeChildren(%1)").arg(r),
                  QString("Include children in %1").arg(getObjectName(bi)));
        bi->setFrameIncludeChildren(b);
        emitDataChanged(bi);
        reposition();
    }
}

void VymModel::setFramePenColor(
    const QColor &c) // FIXME-4 not saved if there is no LMO

{
    BranchItem *bi = getSelectedBranch();
    if (bi) {
        BranchObj *bo = (BranchObj *)(bi->getLMO());
        if (bo) {
            saveState(bi,
                      QString("setFramePenColor (\"%1\")")
                          .arg(bo->getFramePenColor().name()),
                      bi, QString("setFramePenColor (\"%1\")").arg(c.name()),
                      QString("set pen color of frame to %1").arg(c.name()));
            bo->setFramePenColor(c);
        }
    }
}

void VymModel::setFrameBrushColor(
    const QColor &c) // FIXME-4 not saved if there is no LMO
{
    BranchItem *bi = getSelectedBranch();
    if (bi) {
        BranchObj *bo = (BranchObj *)(bi->getLMO());
        if (bo) {
            saveState(bi,
                      QString("setFrameBrushColor (\"%1\")")
                          .arg(bo->getFrameBrushColor().name()),
                      bi, QString("setFrameBrushColor (\"%1\")").arg(c.name()),
                      QString("set brush color of frame to %1").arg(c.name()));
            bo->setFrameBrushColor(c);
            bi->setBackgroundColor(c); // FIXME-4 redundant with above
        }
    }
}

void VymModel::setFramePadding(
    const int &i) // FIXME-4 not saved if there is no LMO
{
    BranchItem *bi = getSelectedBranch();
    if (bi) {
        BranchObj *bo = (BranchObj *)(bi->getLMO());
        if (bo) {
            saveState(
                bi,
                QString("setFramePadding (\"%1\")").arg(bo->getFramePadding()),
                bi, QString("setFramePadding (\"%1\")").arg(i),
                QString("set brush color of frame to %1").arg(i));
            bo->setFramePadding(i);
            reposition();
            bo->updateLinkGeometry();
        }
    }
}

void VymModel::setFrameBorderWidth(
    const int &i) // FIXME-4 not saved if there is no LMO
{
    BranchItem *bi = getSelectedBranch();
    if (bi) {
        BranchObj *bo = (BranchObj *)(bi->getLMO());
        if (bo) {
            saveState(bi,
                      QString("setFrameBorderWidth (\"%1\")")
                          .arg(bo->getFrameBorderWidth()),
                      bi, QString("setFrameBorderWidth (\"%1\")").arg(i),
                      QString("set border width of frame to %1").arg(i));
            bo->setFrameBorderWidth(i);
            reposition();
            bo->updateLinkGeometry();
        }
    }
}

void VymModel::setIncludeImagesVer(bool b)
{
    BranchItem *bi = getSelectedBranch();
    if (bi && b != bi->getIncludeImagesVer()) {
        QString u = b ? "false" : "true";
        QString r = !b ? "false" : "true";

        saveState(
            bi, QString("setIncludeImagesVertically (%1)").arg(u), bi,
            QString("setIncludeImagesVertically (%1)").arg(r),
            QString("Include images vertically in %1").arg(getObjectName(bi)));
        bi->setIncludeImagesVer(b);
        emitDataChanged(bi);
        reposition();
    }
}

void VymModel::setIncludeImagesHor(bool b)
{
    BranchItem *bi = getSelectedBranch();
    if (bi && b != bi->getIncludeImagesHor()) {
        QString u = b ? "false" : "true";
        QString r = !b ? "false" : "true";

        saveState(bi, QString("setIncludeImagesHorizontally (%1)").arg(u), bi,
                  QString("setIncludeImagesHorizontally (%1)").arg(r),
                  QString("Include images horizontally in %1")
                      .arg(getObjectName(bi)));
        bi->setIncludeImagesHor(b);
        emitDataChanged(bi);
        reposition();
    }
}

void VymModel::setChildrenLayout(
    BranchItem::LayoutHint layoutHint) // FIXME-3 no savestate yet
{
    BranchItem *bi = getSelectedBranch();
    if (bi) {
        /*
        QString u= b ? "false" : "true";
        QString r=!b ? "false" : "true";

        saveState(
            bi,
            QString("setIncludeImagesHorizontally (%1)").arg(u),
            bi,
            QString("setIncludeImagesHorizontally (%1)").arg(r),
            QString("Include images horizontally in %1").arg(getObjectName(bi))
        );
        */
        bi->setChildrenLayout(layoutHint);
        emitDataChanged(bi);
        reposition();
    }
}

void VymModel::setHideLinkUnselected(bool b)
{
    TreeItem *ti = getSelectedItem();
    if (ti && (ti->getType() == TreeItem::Image || ti->isBranchLikeType())) {
        QString u = b ? "false" : "true";
        QString r = !b ? "false" : "true";

        saveState(
            ti, QString("setHideLinkUnselected (%1)").arg(u), ti,
            QString("setHideLinkUnselected (%1)").arg(r),
            QString("Hide link of %1 if unselected").arg(getObjectName(ti)));
        ((MapItem *)ti)->setHideLinkUnselected(b);
    }
}

void VymModel::setHideExport(bool b, TreeItem *ti)
{
    if (!ti)
        ti = getSelectedItem();
    if (ti && (ti->getType() == TreeItem::Image || ti->isBranchLikeType()) &&
        ti->hideInExport() != b) {
        ti->setHideInExport(b);
        QString u = b ? "false" : "true";
        QString r = !b ? "false" : "true";

        saveState(ti, QString("setHideExport (%1)").arg(u), ti,
                  QString("setHideExport (%1)").arg(r),
                  QString("Set HideExport flag of %1 to %2")
                      .arg(getObjectName(ti))
                      .arg(r));
        emitDataChanged(ti);
        emitSelectionChanged();
        reposition();
    }
}

void VymModel::toggleHideExport()
{
    QList<TreeItem *> selItems = getSelectedItems();
    if (selItems.count() > 0) {
        foreach (TreeItem *ti, selItems) {
            bool b = !ti->hideInExport();
            setHideExport(b, ti);
        }
    }
}

void VymModel::toggleTask()
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi) {
        saveStateChangingPart(
            selbi, selbi, QString("toggleTask()"),
            QString("Toggle task of %1").arg(getObjectName(selbi)));
        Task *task = selbi->getTask();
        if (!task) {
            task = taskModel->createTask(selbi);
            taskEditor->select(task);
        }
        else
            taskModel->deleteTask(task);

        emitDataChanged(selbi);
        emitSelectionChanged();
        reposition();
    }
}

bool VymModel::cycleTaskStatus(bool reverse)
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi) {
        Task *task = selbi->getTask();
        if (task) {
            saveStateChangingPart(
                selbi, selbi, QString("cycleTask()"),
                QString("Toggle task of %1").arg(getObjectName(selbi)));
            task->cycleStatus(reverse);
            task->setDateModification();

            // make sure task is still visible
            taskEditor->select(task);
            emitDataChanged(selbi);
            reposition();
            return true;
        }
    }
    return false;
}

bool VymModel::setTaskSleep(const QString &s)
{
    bool ok = false;
    BranchItem *selbi = getSelectedBranch();
    if (selbi && !s.isEmpty()) {
        Task *task = selbi->getTask();
        if (task) {
            QDateTime oldSleep = task->getSleep();

            // Parse the string, which could be days, hours or one of several
            // time formats

            if (s == "0") {
                ok = task->setSecsSleep(0);
            }
            else {
                QRegExp re("^\\s*(\\d+)\\s*$");
                re.setMinimal(false);
                int pos = re.indexIn(s);
                if (pos >= 0) {
                    // Found only digit, considered as days
                    ok = task->setDaysSleep(re.cap(1).toInt());
                }
                else {
                    QRegExp re("^\\s*(\\d+)\\s*h\\s*$");
                    pos = re.indexIn(s);
                    if (pos >= 0) {
                        // Found digit followed by "h", considered as hours
                        ok = task->setHoursSleep(re.cap(1).toInt());
                    }
                    else {
                        QRegExp re("^\\s*(\\d+)\\s*w\\s*$");
                        pos = re.indexIn(s);
                        if (pos >= 0) {
                            // Found digit followed by "w", considered as weeks
                            ok = task->setDaysSleep(7 * re.cap(1).toInt());
                        }
                        else {
                            QRegExp re("^\\s*(\\d+)\\s*s\\s*$");
                            pos = re.indexIn(s);
                            if (pos >= 0) {
                                // Found digit followed by "s", considered as
                                // seconds
                                ok = task->setSecsSleep(re.cap(1).toInt());
                            }
                            else {
                                ok = task->setDateSleep(
                                    s); // ISO date YYYY-MM-DDTHH:mm:ss

                                if (!ok) {
                                    QRegExp re("(\\d+)\\.(\\d+)\\.(\\d+)");
                                    re.setMinimal(false);
                                    int pos = re.indexIn(s);
                                    QStringList list = re.capturedTexts();
                                    QDateTime d;
                                    if (pos >= 0) {
                                        d = QDateTime(
                                            QDate(list.at(3).toInt(),
                                                  list.at(2).toInt(),
                                                  list.at(1).toInt()));
                                        // d = QDate(list.at(3).toInt(),
                                        // list.at(2).toInt(),
                                        // list.at(1).toInt()).startOfDay();
                                        ok = task->setDateSleep(
                                            d); // German format,
                                                // e.g. 24.12.2012
                                    }
                                    else {
                                        re.setPattern("(\\d+)\\.(\\d+)\\.");
                                        pos = re.indexIn(s);
                                        list = re.capturedTexts();
                                        if (pos >= 0) {
                                            int month = list.at(2).toInt();
                                            int day = list.at(1).toInt();
                                            int year =
                                                QDate::currentDate().year();
                                            d = QDateTime(
                                                QDate(year, month, day));
                                            // d = QDate(year, month,
                                            // day).startOfDay();
                                            if (QDateTime::currentDateTime()
                                                    .daysTo(d) < 0) {
                                                year++;
                                                d = QDateTime(
                                                    QDate(year, month, day));
                                                // d = QDate(year, month,
                                                // day).startOfDay();
                                            }
                                            ok = task->setDateSleep(
                                                d); // Short German format,
                                                    // e.g. 24.12.
                                        }
                                        else {
                                            re.setPattern("(\\d+)\\:(\\d+)");
                                            pos = re.indexIn(s);
                                            list = re.capturedTexts();
                                            if (pos >= 0) {
                                                int hour = list.at(1).toInt();
                                                int min = list.at(2).toInt();
                                                d = QDateTime(
                                                    QDate::currentDate(),
                                                    QTime(hour, min));
                                                ok = task->setDateSleep(
                                                    d); // Time HH:MM
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            if (ok) {
                QString oldSleepString;
                if (oldSleep.isValid())
                    oldSleepString = oldSleep.toString(Qt::ISODate);
                else
                    oldSleepString =
                        "1970-01-26T00:00:00"; // Some date long ago

                QString newSleepString = task->getSleep().toString(Qt::ISODate);
                task->setDateModification();
                selbi->updateTaskFlag(); // If tasks changes awake mode, then
                                         // flag needs to change
                saveState(
                    selbi, QString("setTaskSleep (\"%1\")").arg(oldSleepString),
                    selbi, QString("setTaskSleep (\"%1\")").arg(newSleepString),
                    QString("setTaskSleep (\"%1\")").arg(newSleepString));
                emitDataChanged(selbi);
                reposition();
            }

        } // Found task
    }     // Found branch
    return ok;
}

void VymModel::setTaskPriorityDelta(const int &n)
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi) {
        Task *task = selbi->getTask();
        if (task) {
            task->setPriorityDelta(n);
            emitDataChanged(selbi);
        }
    }
}

int VymModel::getTaskPriorityDelta()
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi) {
        Task *task = selbi->getTask();
        if (task)
            return task->getPriorityDelta();
    }
    return 0;
}

int VymModel::taskCount() { return taskModel->count(this); }

void VymModel::updateTasksAlarm(bool force)
{
    if (taskModel->updateAwake(force) || force) {
        reposition();
    }
}

BranchItem *VymModel::addTimestamp()
{
    BranchItem *selbi = addNewBranch();
    if (selbi) {
        QDate today = QDate::currentDate();
        QChar c = '0';
        selbi->setHeadingPlainText(QString("%1-%2-%3")
                                       .arg(today.year(), 4, 10, c)
                                       .arg(today.month(), 2, 10, c)
                                       .arg(today.day(), 2, 10, c));
        emitDataChanged(selbi);
        reposition();
        select(selbi);
    }
    return selbi;
}

void VymModel::copy()
{
    if (readonly)
        return;

    QList<TreeItem *> itemList = getSelectedItems();

    clipboardItemCount = itemList.count();

    if (clipboardItemCount > 0) {
        uint i = 1;
        QString fn;
        foreach (TreeItem *ti, itemList) {
            fn = QString("%1/%2-%3.xml")
                     .arg(clipboardDir)
                     .arg(clipboardFile)
                     .arg(i);
            QString content = saveToDir(clipboardDir, clipboardFile,
                                        FlagRowMaster::NoFlags, QPointF(), ti);
            if (!saveStringToDisk(fn, content))
                qWarning() << "ME::saveStringToDisk failed: " << fn;
            else
                i++;
        }
        clipboardItemCount = i - 1;
    }
}

void VymModel::paste()
{
    if (readonly)
        return;

    BranchItem *selbi = getSelectedBranch();
    if (selbi && clipboardItemCount > 0) {
        saveStateChangingPart(selbi, selbi, QString("paste ()"),
                              QString("Paste"));

        bool zippedOrg = zipped;
        uint i = 1;
        QString fn;
        while (i <= clipboardItemCount) {
            fn = QString("%1/%2-%3.xml")
                     .arg(clipboardDir)
                     .arg(clipboardFile)
                     .arg(i);
            if (File::Success != loadMap(fn, ImportAdd, VymMap, SlideContent))
                qWarning() << "Loading clipboard failed: " << fn;

            i++;
        }
        zipped = zippedOrg;
        reposition();
    }
}

void VymModel::cut()
{
    if (readonly)
        return;

    deleteSelection(true);
    return;

    QStringList itemList = getSelectedUUIDs();

    clipboardItemCount = itemList.count();

    if (clipboardItemCount > 0) {
        uint i = 0;
        QString fn;
        TreeItem *ti;
        foreach (QString id, itemList) {
            ti = findUuid(QUuid(id));
            if (ti) {
                fn = QString("%1/%2-%3.xml")
                         .arg(clipboardDir)
                         .arg(clipboardFile)
                         .arg(i);
                QString content = saveToDir(clipboardDir, clipboardFile,
                                            FlagRowMaster::NoFlags, QPointF(), ti);
                if (!saveStringToDisk(fn, content))
                    qWarning() << "ME::saveStringToDisk failed: " << fn;
                else {

                    i++;
                }
            }
        }
        clipboardItemCount = i;
        reposition();
    }
}

bool VymModel::moveUp(BranchItem *bi)
{
    if (readonly)
        return false;

    bool oldState = saveStateBlocked;
    saveStateBlocked = true;
    bool result = false;
    if (bi && bi->canMoveUp())
        result =
            relinkBranch(bi, (BranchItem *)bi->parent(), bi->num() - 1, false);
    saveStateBlocked = oldState;
    return result;
}

void VymModel::moveUp()
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi) {
        QString oldsel = getSelectString(selbi);
        if (moveUp(selbi)) {
            saveState(getSelectString(selbi), "moveDown ()", oldsel,
                      "moveUp ()",
                      QString("Move up %1").arg(getObjectName(selbi)));
            select(selbi);
        }
    }
}

bool VymModel::moveDown(BranchItem *bi)
{
    if (readonly)
        return false;

    bool oldState = saveStateBlocked;
    saveStateBlocked = true;
    bool result = false;
    if (bi && bi->canMoveDown())
        result =
            relinkBranch(bi, (BranchItem *)bi->parent(), bi->num() + 1, false);
    saveStateBlocked = oldState;
    return result;
}

void VymModel::moveDown()
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi) {
        QString oldsel = getSelectString(selbi);
        if (moveDown(selbi)) {
            saveState(getSelectString(selbi), "moveUp ()", oldsel,
                      "moveDown ()",
                      QString("Move down %1").arg(getObjectName(selbi)));
            select(selbi);
        }
    }
}

void VymModel::detach()
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi && selbi->depth() > 0) {
        // if no relPos have been set before, try to use current rel positions
        if (selbi->getLMO())
            for (int i = 0; i < selbi->branchCount(); ++i)
                selbi->getBranchNum(i)->getBranchObj()->setRelPos();

        QString oldsel = getSelectString();
        int n = selbi->num();
        QPointF p;
        BranchObj *bo = selbi->getBranchObj();
        if (bo)
            p = bo->getAbsPos();
        QString parsel = getSelectString(selbi->parent());
        if (relinkBranch(selbi, rootItem, -1, true))
            saveState(getSelectString(selbi),
                      QString("relinkTo (\"%1\",%2,%3,%4)")
                          .arg(parsel)
                          .arg(n)
                          .arg(p.x())
                          .arg(p.y()),
                      oldsel, "detach ()",
                      QString("Detach %1").arg(getObjectName(selbi)));
    }
}

void VymModel::sortChildren(bool inverse)
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi) {
        if (selbi->branchCount() > 1) {
            if (!inverse)
                saveStateChangingPart(
                    selbi, selbi, "sortChildren ()",
                    QString("Sort children of %1").arg(getObjectName(selbi)));
            else
                saveStateChangingPart(selbi, selbi, "sortChildren (false)",
                                      QString("Inverse sort children of %1")
                                          .arg(getObjectName(selbi)));

            selbi->sortChildren(inverse);
            select(selbi);
            reposition();
        }
    }
}

BranchItem *VymModel::createMapCenter()
{
    BranchItem *newbi = addMapCenter(QPointF(0, 0));
    return newbi;
}

BranchItem *VymModel::createBranch(BranchItem *dst)
{
    if (dst)
        return addNewBranchInt(dst, -2);
    else
        return NULL;
}

ImageItem *VymModel::createImage(BranchItem *dst)
{
    if (dst) {
        QModelIndex parix;
        int n;

        QList<QVariant> cData;
        cData << tr("Image", "Default name for new image") << "undef";

        ImageItem *newii = new ImageItem(cData);
        // newii->setHeading (QApplication::translate("Heading of new image in
        // map", "new image"));

        emit(layoutAboutToBeChanged());

        parix = index(dst);
        if (!parix.isValid())
            qDebug() << "VM::createII invalid index\n";
        n = dst->getRowNumAppend(newii);
        beginInsertRows(parix, n, n);
        dst->appendChild(newii);
        endInsertRows();

        emit(layoutChanged());

        // save scroll state. If scrolled, automatically select
        // new branch in order to tmp unscroll parent...
        newii->createMapObj();
        latestAddedItem = newii;
        reposition();
        return newii;
    }
    return NULL;
}

bool VymModel::createLink(Link *link)
{
    BranchItem *begin = link->getBeginBranch();
    BranchItem *end = link->getEndBranch();

    if (!begin || !end) {
        qWarning() << "VM::createXLinkNew part of XLink is NULL";
        return false;
    }

    if (begin == end) {
        if (debug)
            qDebug() << "VymModel::createLink begin==end, aborting";
        return false;
    }

    // check, if link already exists
    foreach (Link *l, xlinks) {
        if ((l->getBeginBranch() == begin && l->getEndBranch() == end) ||
            (l->getBeginBranch() == end && l->getEndBranch() == begin)) {
            qWarning() << "VymModel::createLink link exists already, aborting";
            return false;
        }
    }

    QModelIndex parix;
    int n;

    QList<QVariant> cData;

    cData << "new Link begin"
          << "undef";
    XLinkItem *newli = new XLinkItem(cData);
    newli->setLink(link);
    link->setBeginLinkItem(newli);

    emit(layoutAboutToBeChanged());

    parix = index(begin);
    n = begin->getRowNumAppend(newli);
    beginInsertRows(parix, n, n);
    begin->appendChild(newli);
    endInsertRows();

    cData.clear();
    cData << "new Link end"
          << "undef";
    newli = new XLinkItem(cData);
    newli->setLink(link);
    link->setEndLinkItem(newli);

    parix = index(end);
    n = end->getRowNumAppend(newli);
    beginInsertRows(parix, n, n);
    end->appendChild(newli);
    endInsertRows();

    emit(layoutChanged());

    xlinks.append(link);
    link->activate();

    latestAddedItem = newli;

    if (!link->getMO()) {
        link->createMapObj();
        reposition();
    }
    else
        link->updateLink();

    link->setStyleBegin(defXLinkStyleBegin);
    link->setStyleEnd(defXLinkStyleEnd);
    return true;
}

QColor VymModel::getXLinkColor()
{
    Link *l = getSelectedXLink();
    if (l)
        return l->getPen().color();
    else
        return QColor();
}

int VymModel::getXLinkWidth()
{
    Link *l = getSelectedXLink();
    if (l)
        return l->getPen().width();
    else
        return -1;
}

Qt::PenStyle VymModel::getXLinkStyle()
{
    Link *l = getSelectedXLink();
    if (l)
        return l->getPen().style();
    else
        return Qt::NoPen;
}

QString VymModel::getXLinkStyleBegin()
{
    Link *l = getSelectedXLink();
    if (l)
        return l->getStyleBeginString();
    else
        return QString();
}

QString VymModel::getXLinkStyleEnd()
{
    Link *l = getSelectedXLink();
    if (l)
        return l->getStyleEndString();
    else
        return QString();
}

AttributeItem *VymModel::addAttribute() // FIXME-3 Experimental, savestate missing

{
    BranchItem *selbi = getSelectedBranch();
    if (selbi) {
        QList<QVariant> cData;
        cData << "new attribute"
              << "undef";
        AttributeItem *a = new AttributeItem(cData);
        a->setAttributeType(AttributeItem::String);
        a->setKey("Foo Attrib");
        a->setValue(QString("Att val"));

        if (addAttribute(selbi, a))
            return a;
    }
    return NULL;
}

AttributeItem *VymModel::addAttribute(BranchItem *dst, AttributeItem *ai)
{
    if (dst) {
        emit(layoutAboutToBeChanged());

        QModelIndex parix = index(dst);
        int n = dst->getRowNumAppend(ai);
        beginInsertRows(parix, n, n);
        dst->appendChild(ai);
        endInsertRows();

        emit(layoutChanged());

        return ai;  // FIXME-3 Check if ai is used or deleted - deep copy here?
    }
    return NULL;
}

BranchItem *VymModel::addMapCenter(bool saveStateFlag)
{
    if (!hasContextPos) {
        // E.g. when called via keypresss:
        // Place new MCO in middle of existing ones,
        // Useful for "brainstorming" mode...
        contextPos = QPointF();
        BranchItem *bi;
        BranchObj *bo;
        for (int i = 0; i < rootItem->branchCount(); ++i) {
            bi = rootItem->getBranchNum(i);
            bo = (BranchObj *)bi->getLMO();
            if (bo)
                contextPos += bo->getAbsPos();
        }
        if (rootItem->branchCount() > 1)
            contextPos *= 1 / (qreal)(rootItem->branchCount());
    }

    BranchItem *bi = addMapCenter(contextPos);
    updateActions();
    emitShowSelection();
    if (saveStateFlag)
        saveState(bi, "remove()", NULL,
                  QString("addMapCenter (%1,%2)")
                      .arg(contextPos.x())
                      .arg(contextPos.y()),
                  QString("Adding MapCenter to (%1,%2)")
                      .arg(contextPos.x())
                      .arg(contextPos.y()));
    emitUpdateLayout();
    return bi;
}

BranchItem *VymModel::addMapCenter(QPointF absPos)
// createMapCenter could then probably be merged with createBranch
{

    // Create TreeItem
    QModelIndex parix = index(rootItem);

    QList<QVariant> cData;
    cData << "VM:addMapCenter"
          << "undef";
    BranchItem *newbi = new BranchItem(cData, rootItem);
    newbi->setHeadingPlainText(tr("New map", "New map"));
    int n = rootItem->getRowNumAppend(newbi);

    emit(layoutAboutToBeChanged());
    beginInsertRows(parix, n, n);

    rootItem->appendChild(newbi);

    endInsertRows();
    emit(layoutChanged());

    // Create MapObj
    newbi->setPositionMode(MapItem::Absolute);
    BranchObj *bo = newbi->createMapObj(mapEditor->getScene());
    if (bo)
        bo->move(absPos);

    return newbi;
}

BranchItem *VymModel::addNewBranchInt(BranchItem *dst, int pos)
{
    // Depending on pos:
    // -3	insert in children of parent  above selection
    // -2	add branch to selection
    // -1	insert in children of parent below selection
    // 0..n	insert in children of parent at pos

    // Create TreeItem
    QList<QVariant> cData;
    cData << ""
          << "undef";

    BranchItem *parbi = dst;
    int n;
    BranchItem *newbi = new BranchItem(cData);

    emit(layoutAboutToBeChanged());

    if (pos == -2) {
        n = parbi->getRowNumAppend(newbi);
        beginInsertRows(index(parbi), n, n);
        parbi->appendChild(newbi);
        endInsertRows();
    }
    else if (pos == -1 || pos == -3) {
        // insert below selection
        parbi = (BranchItem *)dst->parent();
        n = dst->childNumber() + (3 + pos) / 2; //-1 |-> 1;-3 |-> 0
        beginInsertRows(index(parbi), n, n);
        parbi->insertBranch(n, newbi);
        endInsertRows();
    }
    else { // pos >= 0
        n = parbi->getRowNumAppend(newbi) - (parbi->branchCount() - pos);
        beginInsertRows(index(parbi), n, n);
        parbi->insertBranch(pos, newbi);
        endInsertRows();
    }
    emit(layoutChanged());

    // Create MapObj and Container
    BranchObj *newbo = newbi->createMapObj(mapEditor->getScene());

    // Add newbi also into Container of parent
    parbi->getBranchObj()->addContainer(newbo); // FIXME-0 not considered yet: position!

    // Set color of heading to that of parent
    newbi->setHeadingColor(parbi->getHeadingColor());

    reposition();
    return newbi;
}

BranchItem *VymModel::addNewBranch(BranchItem *bi, int pos)
{
    BranchItem *newbi = NULL;
    if (!bi)
        bi = getSelectedBranch();

    if (bi) {
        QString redosel = getSelectString(bi);
        newbi = addNewBranchInt(bi, pos);
        QString undosel = getSelectString(newbi);

        if (newbi) {
            saveState(undosel, "remove ()", redosel,
                      QString("addBranch (%1)").arg(pos),
                      QString("Add new branch to %1").arg(getObjectName(bi)));

            latestAddedItem = newbi;
            // In Network mode, the client needs to know where the new branch
            // is, so we have to pass on this information via saveState.
            // TODO: Get rid of this positioning workaround
            /* FIXME-4  network problem:  QString ps=qpointfToString
               (newbo->getAbsPos()); sendData ("selectLatestAdded ()"); sendData
               (QString("move %1").arg(ps)); sendSelection();
               */
        }
    }
    return newbi;
}

BranchItem *VymModel::addNewBranchBefore()
{
    BranchItem *newbi = NULL;
    BranchItem *selbi = getSelectedBranch();
    if (selbi && selbi->getType() == TreeItem::Branch)
    // We accept no MapCenter here, so we _have_ a parent
    {
        // add below selection
        newbi = addNewBranchInt(selbi, -1);

        if (newbi) {
            saveState(
                newbi, "remove ()", newbi, "addBranchBefore ()",
                QString("Add branch before %1").arg(getObjectName(selbi)));

            // newbi->move2RelPos (p);

            // Move selection to new branch
            relinkBranch(selbi, newbi, 0, true);

            // Use color of child instead of parent
            newbi->setHeadingColor(selbi->getHeadingColor());
            emitDataChanged(newbi);
        }
    }
    return newbi;
}

bool VymModel::relinkBranch(BranchItem *branch, BranchItem *dst, int pos,
                            bool updateSelection, QPointF orgPos)
{
    if (branch && dst) {
        // Check if we relink to ourselves
        if (dst->isChildOf(branch))
            return false;

        if (updateSelection)
            unselectAll();

        // Do we need to update frame type?
        bool keepFrame = true;

        // Save old position for savestate
        QString preSelStr = getSelectString(branch);
        QString preNum = QString::number(branch->num(), 10);
        QString preParStr = getSelectString(branch->parent());

        emit(layoutAboutToBeChanged());
        BranchItem *branchpi = (BranchItem *)branch->parent();
        // Remove at current position
        int n = branch->childNum();

        beginRemoveRows(index(branchpi), n, n);
        branchpi->removeChild(n);
        endRemoveRows();

        if (pos < 0 || pos > dst->branchCount())
            pos = dst->branchCount();

        // Append as last branch to dst
        if (dst->branchCount() == 0)
            n = 0;
        else
            n = dst->getFirstBranch()->childNumber();
        beginInsertRows(index(dst), n + pos, n + pos);
        dst->insertBranch(pos, branch);
        endInsertRows();

        // Correct type if necessesary
        if (branch->getType() == TreeItem::MapCenter && branch->depth() > 0) {
            branch->setType(TreeItem::Branch);
            keepFrame = false;
        }

        // reset parObj, fonts, frame, etc in related LMO or other view-objects
        branch->updateStyles(keepFrame);

        emitDataChanged(branch);
        reposition(); // both for moveUp/Down and relinking

        // Savestate
        QString postSelStr = getSelectString(branch);
        QString postNum = QString::number(branch->num(), 10);

        QPointF savePos;
        LinkableMapObj *lmosel = branch->getLMO();
        if (lmosel)
            savePos = lmosel->getAbsPos();

        if (!saveStateBlocked) { // Don't build strings when moving up/down
            QString undoCom =
                "relinkTo (\"" + preParStr + "\"," + preNum + "," +
                QString("%1,%2").arg(orgPos.x()).arg(orgPos.y()) + ")";

            QString redoCom =
                "relinkTo (\"" + getSelectString(dst) + "\"," + postNum + "," +
                QString("%1,%2").arg(savePos.x()).arg(savePos.y()) + ")";

            saveState(postSelStr, undoCom, preSelStr, redoCom,
                      QString("Relink %1 to %2")
                          .arg(getObjectName(branch))
                          .arg(getObjectName(dst)));
        }

        // New parent might be invisible
        branch->updateVisibility();

        if (dst->isScrolled()) {
            if (updateSelection)
                select(dst);
        }
        else if (updateSelection)
            select(branch);
        return true;
    }
    return false;
}

bool VymModel::relinkImage(ImageItem *image, BranchItem *dst)
{
    if (image && dst) {
        emit(layoutAboutToBeChanged());

        BranchItem *pi = (BranchItem *)(image->parent());
        QString oldParString = getSelectString(pi);
        // Remove at current position
        int n = image->childNum();
        beginRemoveRows(index(pi), n, n);
        pi->removeChild(n);
        endRemoveRows();

        // Add at dst
        QModelIndex dstix = index(dst);
        n = dst->getRowNumAppend(image);
        beginInsertRows(dstix, n, n + 1);
        dst->appendChild(image);
        endInsertRows();

        // Set new parent also for lmo
        if (image->getLMO() && dst->getLMO())
            image->getLMO()->setParObj(dst->getLMO());

        emit(layoutChanged());
        saveState(image, QString("relinkTo (\"%1\")").arg(oldParString), image,
                  QString("relinkTo (\"%1\")").arg(getSelectString(dst)),
                  QString("Relink floatimage to %1").arg(getObjectName(dst)));
        return true;
    }
    return false;
}

bool VymModel::relinkTo(const QString &dest, int num, QPointF pos)
{
    TreeItem *selti = getSelectedItem();
    if (!selti)
        return false; // Nothing selected to relink

    TreeItem *dst = findBySelectString(dest);

    if (selti->isBranchLikeType()) {
        BranchItem *selbi = (BranchItem *)selti;
        if (!dst)
            return false; // Could not find destination

        if (dst->getType() == TreeItem::Branch) {
            // Now try to relink to branch
            if (relinkBranch(selbi, (BranchItem *)dst, num, true)) {
                emitSelectionChanged();
                return true;
            }
            else
                return false; // Relinking failed
        }
        else if (dst->getType() == TreeItem::MapCenter) {
            if (relinkBranch(selbi, (BranchItem *)dst, -1, true)) {
                // Get coordinates of mainbranch
                if (selbi->getLMO()) {
                    ((BranchObj *)selbi->getLMO())->move(pos);
                    ((BranchObj *)selbi->getLMO())->setRelPos();
                }
                reposition();
                emitSelectionChanged();
                return true;
            }
        }
        return false; // Relinking failed
    }
    else if (selti->getType() == TreeItem::Image) {
        if (dst->isBranchLikeType())
            if (relinkImage(((ImageItem *)selti), (BranchItem *)dst))
                return true;
    }
    return false; // Relinking failed
}

void VymModel::cleanupItems()
{
    while (!deleteLaterIDs.isEmpty()) {
        TreeItem *ti = findID(deleteLaterIDs.takeFirst());
        if (ti)
            deleteItem(ti);
    }
}

void VymModel::deleteLater(uint id)
{
    if (!deleteLaterIDs.contains(id))
        deleteLaterIDs.append(id);
}

void VymModel::deleteSelection(bool copyToClipboard)
{
    QList<uint> selectedIDs = getSelectedIDs();
    unselectAll();
    QString fn;

    if (copyToClipboard)
        clipboardItemCount = 0;

    foreach (uint id, selectedIDs) {
        TreeItem *ti = findID(id);
        if (ti) {
            if (ti->isBranchLikeType()) { // Delete branch
                BranchItem *selbi = (BranchItem *)ti;
                saveStateRemovingPart(
                    selbi, QString("remove %1").arg(getObjectName(selbi)));

                if (copyToClipboard) {
                    fn = QString("%1/%2-%3.xml")
                             .arg(clipboardDir)
                             .arg(clipboardFile)
                             .arg(clipboardItemCount + 1);
                    QString content =
                        saveToDir(clipboardDir, clipboardFile, FlagRowMaster::NoFlags,
                                  QPointF(), ti);
                    if (!saveStringToDisk(fn, content))
                        qWarning() << "ME::saveStringToDisk failed: " << fn;
                    else
                        clipboardItemCount++;
                }

                BranchItem *pi = (BranchItem *)(deleteItem(selbi));
                if (pi) {
                    if (pi->isScrolled() && pi->branchCount() == 0)
                        pi->unScroll();
                    emitDataChanged(pi);
                    select(pi);
                }
                else
                    emitDataChanged(rootItem);
                ti = NULL;
            }
            else {
                // Delete other item
                TreeItem *pi = ti->parent();
                if (pi) {
                    if (ti->getType() == TreeItem::Image ||
                        ti->getType() == TreeItem::Attribute ||
                        ti->getType() == TreeItem::XLink) {
                        saveStateChangingPart(
                            pi, ti, "remove ()",
                            QString("Remove %1").arg(getObjectName(ti)));

                        if (copyToClipboard) {
                            fn = QString("%1/%2-%3.xml")
                                     .arg(clipboardDir)
                                     .arg(clipboardFile)
                                     .arg(clipboardItemCount + 1);
                            QString content =
                                saveToDir(clipboardDir, clipboardFile,
                                          FlagRowMaster::NoFlags, QPointF(), ti);
                            if (!saveStringToDisk(fn, content))
                                qWarning()
                                    << "ME::saveStringToDisk failed: " << fn;
                            else
                                clipboardItemCount++;
                        }

                        deleteItem(ti);
                        emitDataChanged(pi);
                        select(pi);
                        reposition();
                    }
                    else
                        qWarning(
                            "VymmModel::deleteSelection()  unknown type?!");
                }
            }
        }
    }
}

void VymModel::deleteKeepChildren(bool saveStateFlag)
// deleteKeepChildren FIXME-3+ does not work yet for mapcenters
// deleteKeepChildren FIXME-3+ children of scrolled branch stay invisible...
{
    BranchItem *selbi = getSelectedBranch();
    BranchItem *pi;
    if (selbi) {
        // Don't use this on mapcenter
        if (selbi->depth() < 1)
            return;

        pi = (BranchItem *)(selbi->parent());
        // Check if we have children at all to keep
        if (selbi->branchCount() == 0) {
            deleteSelection();
            return;
        }

        QPointF p;
        if (selbi->getLMO())
            p = selbi->getLMO()->getRelPos();
        if (saveStateFlag)
            saveStateChangingPart(pi, pi, "removeKeepChildren ()",
                                  QString("Remove %1 and keep its children")
                                      .arg(getObjectName(selbi)));

        QString sel = getSelectString(selbi);
        unselectAll();
        bool oldSaveState = saveStateBlocked;
        saveStateBlocked = true;
        int pos = selbi->num();
        BranchItem *bi = selbi->getFirstBranch();
        while (bi) {
            relinkBranch(bi, pi, pos, true);
            bi = selbi->getFirstBranch();
            pos++;
        }
        deleteItem(selbi);
        reposition();
        emitDataChanged(pi);
        select(sel);
        BranchObj *bo = getSelectedBranchObj();
        if (bo) {
            bo->move2RelPos(p);
            reposition();
        }
        saveStateBlocked = oldSaveState;
    }
}

void VymModel::deleteChildren()

{
    BranchItem *selbi = getSelectedBranch();
    if (selbi) {
        saveStateChangingPart(
            selbi, selbi, "removeChildren ()",
            QString("Remove children of branch %1").arg(getObjectName(selbi)));
        emit(layoutAboutToBeChanged());

        QModelIndex ix = index(selbi);
        int n = selbi->branchCount() - 1;
        beginRemoveRows(ix, 0, n);
        removeRows(0, n + 1, ix);
        endRemoveRows();
        if (selbi->isScrolled())
            unscrollBranch(selbi);
        emit(layoutChanged());
        reposition();
    }
}

TreeItem *VymModel::deleteItem(TreeItem *ti)
{
    if (ti) {
        TreeItem *pi = ti->parent();
        // qDebug()<<"VM::deleteItem  start ti="<<ti<<"  "<<ti->getHeading()<<"
        // pi="<<pi<<"="<<pi->getHeading();

        TreeItem::Type t = ti->getType();

        QModelIndex parentIndex = index(pi);

        emit(layoutAboutToBeChanged());

        int n = ti->childNum();
        beginRemoveRows(parentIndex, n, n);
        removeRows(n, 1, parentIndex);
        endRemoveRows();

        // Size of parent branch might change when deleting images
        if (t == TreeItem::Image) {
            BranchObj *bo = (BranchObj *)(((BranchItem *)pi)->getMO());
            if (bo)
                bo->calcBBoxSize();
        }

        reposition();

        emit(layoutChanged());
        emitUpdateQueries();
        if (!cleaningUpLinks)
            cleanupItems();

        // qDebug()<<"VM::deleteItem  end   ti="<<ti;
        if (pi->depth() >= 0)
            return pi;
    }
    return NULL;
}

void VymModel::deleteLink(Link *l)
{
    if (xlinks.removeOne(l))
        delete (l);
}

void VymModel::clearItem(TreeItem *ti)
{
    if (ti) {
        // Clear task (or other data in item itself)
        ti->clear();

        QModelIndex parentIndex = index(ti);
        if (!parentIndex.isValid())
            return;

        int n = ti->childCount();
        if (n == 0)
            return;

        emit(layoutAboutToBeChanged());

        beginRemoveRows(parentIndex, 0, n - 1);
        removeRows(0, n, parentIndex);
        endRemoveRows();

        reposition();

        emit(layoutChanged());
    }
    return;
}

bool VymModel::scrollBranch(BranchItem *bi)
{
    if (bi) {
        if (bi->isScrolled())
            return false;
        if (bi->branchCount() == 0)
            return false;
        if (bi->depth() == 0)
            return false;
        if (bi->toggleScroll()) {
            QString u, r;
            r = "scroll";
            u = "unscroll";
            saveState(bi, QString("%1 ()").arg(u), bi, QString("%1 ()").arg(r),
                      QString("%1 %2").arg(r).arg(getObjectName(bi)));
            emitDataChanged(bi);
            emitSelectionChanged();
            reposition();
            mapEditor->getScene()
                ->update(); // Needed for _quick_ update,  even in 1.13.x
            return true;
        }
    }
    return false;
}

bool VymModel::unscrollBranch(BranchItem *bi)
{
    if (bi) {
        if (!bi->isScrolled())
            return false;
        if (bi->toggleScroll()) {
            QString u, r;
            u = "scroll";
            r = "unscroll";
            saveState(bi, QString("%1 ()").arg(u), bi, QString("%1 ()").arg(r),
                      QString("%1 %2").arg(r).arg(getObjectName(bi)));
            emitDataChanged(bi);
            emitSelectionChanged();
            reposition();
            mapEditor->getScene()
                ->update(); // Needed for _quick_ update,  even in 1.13.x
            return true;
        }
    }
    return false;
}

void VymModel::toggleScroll()
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi) {
        if (selbi->isScrolled())
            unscrollBranch(selbi);
        else
            scrollBranch(selbi);
        // Note: saveState & reposition are called in above functions
    }
}

void VymModel::unscrollChildren()
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi) {
        saveStateChangingPart(
            selbi, selbi, QString("unscrollChildren ()"),
            QString("unscroll all children of %1").arg(getObjectName(selbi)));
        BranchItem *prev = NULL;
        BranchItem *cur = NULL;
        nextBranch(cur, prev, true, selbi);
        while (cur) {
            if (cur->isScrolled()) {
                cur->toggleScroll();
                emitDataChanged(cur);
            }
            nextBranch(cur, prev, true, selbi);
        }
        updateActions();
        reposition();
        // Would this help??? emitSelectionChanged();
    }
}

void VymModel::setScaleFactor(qreal f)
{
    ImageItem *selii = getSelectedImage();
    if (selii) {
        qreal f_old = selii->getScaleFactor();
        selii->setScaleFactor(f);
        saveState(selii, QString("setScaleFactor(%1)").arg(f_old), selii,
                  QString("setScaleFactor(%1)").arg(f),
                  QString("Scale %1").arg(getObjectName(selii)));
        reposition();
    }
}

void VymModel::growSelectionSize() // FIXME-3 Also for heading in BranchItem?
{
    ImageItem *selii = getSelectedImage();
    if (selii) {
        qreal f = 0.05;
        qreal sx = selii->getScaleFactor();
        setScaleFactor(sx + f);
    }
}

void VymModel::shrinkSelectionSize()
{
    ImageItem *selii = getSelectedImage();
    if (selii) {
        qreal f = 0.05;
        qreal sx = selii->getScaleFactor();
        setScaleFactor(sx - f);
    }
}

void VymModel::resetSelectionSize()
{
    ImageItem *selii = getSelectedImage();
    if (selii)
        setScaleFactor(1);
}

void VymModel::emitExpandAll() { emit(expandAll()); }

void VymModel::emitExpandOneLevel() { emit(expandOneLevel()); }

void VymModel::emitCollapseOneLevel() { emit(collapseOneLevel()); }

void VymModel::emitCollapseUnselected() { emit(collapseUnselected()); }

void VymModel::toggleTarget()
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi) {
        selbi->toggleTarget();
        reposition();
        saveState(selbi, "toggleTarget()", selbi, "toggleTarget()",
                  "Toggle target");
    }
}

ItemList VymModel::getLinkedMaps()
{
    ItemList targets;

    // rmodel->setSearchString (s);

    BranchItem *cur = NULL;
    BranchItem *prev = NULL;
    nextBranch(cur, prev);

    QString s;

    while (cur) {
        if (cur->hasActiveSystemFlag("system-target") &&
            !cur->getVymLink().isEmpty()) {
            s = cur->getHeading().getTextASCII();
            s.replace(QRegularExpression("\n+"), " ");
            s.replace(QRegularExpression("\\s+"), " ");
            s.replace(QRegularExpression("^\\s+"), "");

            QStringList sl;
            sl << s;
            sl << cur->getVymLink();

            targets[cur->getID()] = sl;
        }
        nextBranch(cur, prev);
    }
    return targets;
}

ItemList VymModel::getTargets()
{
    ItemList targets;

    // rmodel->setSearchString (s);

    BranchItem *cur = NULL;
    BranchItem *prev = NULL;
    nextBranch(cur, prev);

    QString s;

    while (cur) {
        if (cur->hasActiveSystemFlag("system-target")) {
            s = cur->getHeading().getTextASCII();
            s.replace(QRegularExpression("\n+"), " ");
            s.replace(QRegularExpression("\\s+"), " ");
            s.replace(QRegularExpression("^\\s+"), "");

            QStringList sl;
            sl << s;

            targets[cur->getID()] = sl;
        }
        nextBranch(cur, prev);
    }
    return targets;
}

void VymModel::toggleFlagByUid(
    const QUuid &uid,
    bool useGroups) 
    // FIXME-2  saveState not correct when toggling flags in groups
    // (previous flags not saved!)
{
    BranchItem *bi = getSelectedBranch();

    if (bi) {
        Flag *f = bi->toggleFlagByUid(uid, useGroups);

        if (f) {
            QString u = "toggleFlagByUid";
            QString name = f->getName();
            saveState(bi, QString("%1 (\"%2\")").arg(u).arg(uid.toString()), bi,
                      QString("%1 (\"%2\")").arg(u).arg(uid.toString()),
                      QString("Toggling flag \"%1\" of %2")
                          .arg(name)
                          .arg(getObjectName(bi)));
            emitDataChanged(bi);
            reposition();
        }
        else
            qWarning() << "VymModel::toggleFlag failed for flag with uid "
                       << uid;
    }
}

void VymModel::toggleFlagByName(const QString &name, bool useGroups)
{
    // Toggling by name only used from vymmodelwrapper for scripting  // FIXME-4
    // maybe rework?
    BranchItem *bi = getSelectedBranch();

    if (bi) {
        Flag *f = standardFlagsMaster->findFlagByName(name);
        if (!f) {
            f = userFlagsMaster->findFlagByName(name);
            if (!f) {
                qWarning() << "VymModel::toggleFlag failed for flag named "
                           << name;
                return;
            }
        }

        QUuid uid = f->getUuid();

        f = bi->toggleFlagByUid(uid, useGroups);

        if (f) {
            QString u = "toggleFlag";
            QString name = f->getName();
            saveState(bi, QString("%1 (\"%2\")").arg(u).arg(name), bi,
                      QString("%1 (\"%2\")").arg(u).arg(name),
                      QString("Toggling flag \"%1\" of %2")
                          .arg(name)
                          .arg(getObjectName(bi)));
            emitDataChanged(bi);
            reposition();
        }
        else
            qWarning() << "VymModel::toggleFlag failed for flag named " << name
                       << " with uid " << uid;
    }
}

void VymModel::clearFlags()
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi) {
        selbi->deactivateAllStandardFlags();
        reposition();
        emitDataChanged(selbi);
        setChanged();
    }
}

void VymModel::colorBranch(QColor c)
{
    QList<BranchItem *> selbis = getSelectedBranches();
    foreach (BranchItem *selbi, selbis) {
        saveState(selbi,
                  QString("colorBranch (\"%1\")")
                      .arg(selbi->getHeadingColor().name()),
                  selbi, QString("colorBranch (\"%1\")").arg(c.name()),
                  QString("Set color of %1 to %2")
                      .arg(getObjectName(selbi))
                      .arg(c.name()));
        selbi->setHeadingColor(c); // color branch
        emitDataChanged(selbi);
        taskEditor->showSelection();
    }
    mapEditor->getScene()->update();
}

void VymModel::colorSubtree(QColor c, BranchItem *b)
{
    QList<BranchItem *> selbis;
    if (b)
        selbis.append(b);
    else
        selbis = getSelectedBranches();

    foreach (BranchItem *bi, selbis) {
        saveStateChangingPart(bi, bi,
                              QString("colorSubtree (\"%1\")").arg(c.name()),
                              QString("Set color of %1 and children to %2")
                                  .arg(getObjectName(bi))
                                  .arg(c.name()));
        BranchItem *prev = NULL;
        BranchItem *cur = NULL;
        nextBranch(cur, prev, true, bi);
        while (cur) {
            cur->setHeadingColor(c); // color links, color children
            emitDataChanged(cur);
            nextBranch(cur, prev, true, bi);
        }
    }
    taskEditor->showSelection();
    mapEditor->getScene()->update();
}

QColor VymModel::getCurrentHeadingColor()
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi)
        return selbi->getHeadingColor();

    QMessageBox::warning(
        0, "Warning",
        "Can't get color of heading,\nthere's no branch selected");
    return Qt::black;
}

void VymModel::note2URLs()
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi) {
        saveStateChangingPart(
            selbi, selbi, QString("note2URLs()"),
            QString("Extract URLs from note of %1").arg(getObjectName(selbi)));

        QString n = selbi->getNoteASCII();
        if (n.isEmpty())
            return;
        QRegExp re("(http.*)(\\s|\"|')");
        re.setMinimal(true);

        BranchItem *bi;
        int pos = 0;
        while ((pos = re.indexIn(n, pos)) != -1) {
            bi = createBranch(selbi);
            bi->setHeadingPlainText(re.cap(1));
            bi->setURL(re.cap(1));
            emitDataChanged(bi);
            pos += re.matchedLength();
        }
    }
}

void VymModel::editHeading2URL()
{
    TreeItem *selti = getSelectedItem();
    if (selti)
        setURL(selti->getHeadingPlain());
}

void VymModel::getJiraData(bool subtree) // FIXME-2 update error message, check
                                         // if jiraClientAvail is set correctly
{
    if (!jiraClientAvailable) {
        WarningDialog dia;
        dia.setText(
            QObject::tr("JIRA client not found."));
        dia.setWindowTitle(
            tr("Warning") + ": " + tr("JIRA client not found"));
        dia.setShowAgainName("/JiraClient/missing");
        dia.exec();
        return;
    }

    BranchItem *selbi = getSelectedBranch();
    if (selbi) {
        QString url;
        BranchItem *prev = NULL;
        BranchItem *cur = NULL;
        nextBranch(cur, prev, true, selbi);
        while (cur) {
            url = cur->getURL();

            if (url.isEmpty()) {
                QString heading = cur->getHeadingPlain();
                if (heading.left(4) == "http") {
                    // Set URL from heading
                    cur->setURL(heading);
                    url = heading;
                }
                else {
                    // Heading could contain a JIRA ID, call jigger using that
                    // Afterwards jiraagent could update URL, if successfully
                    // retrieved data But only do that, if heading looks like a
                    // JIRA ID and without URL yet also only do if there are no
                    // children and no whitespaces (created in log info later)
                    if (cur->branchCount() == 0) {
                        if (heading.contains(QRegExp("\\w[-|\\s](\\d+)"))) {
                            new JiraAgent(cur, heading);
                            mainWindow->statusMessage(
                                tr("Contacting Jira...", "VymModel"));
                        }
                    }
                }
            }

            if (!url.isEmpty()) {
                new JiraAgent(cur, url);
                mainWindow->statusMessage(tr("Contacting Jira...", "VymModel"));
            }

            if (subtree)
                nextBranch(cur, prev, true, selbi);
            else
                cur = NULL;
        }
    }
}

void VymModel::setHeadingConfluencePageName()
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi) {
        QString url = selbi->getURL();
        if (!url.isEmpty()) {
            if (confluenceAgentAvailable) {
                if (!url.contains(
                        settings.value("/confluence/url", "").toString()))
                    return;

                if (confluencePassword.isEmpty()) {
                    // Get password and abort, if dialog canceled
                    if (!mainWindow->settingsConfluence())
                        return;
                }

                ConfluenceAgent *ca_setHeading = new ConfluenceAgent(selbi);
                ca_setHeading->setPageURL(url);
                ca_setHeading->setJobType(ConfluenceAgent::CopyPagenameToHeading);
                ca_setHeading->startJob();
            }
        }
    }
}

void VymModel::setVymLink(const QString &s) // FIXME-4 fail, if s does not exist
{
    BranchItem *bi = getSelectedBranch();
    if (bi) {
        saveState(
            bi, "setVymLink (\"" + bi->getVymLink() + "\")", bi,
            "setVymLink (\"" + s + "\")",
            QString("Set vymlink of %1 to %2").arg(getObjectName(bi)).arg(s));
        bi->setVymLink(s);
        emitDataChanged(bi);
        reposition();
    }
}

void VymModel::deleteVymLink()
{
    BranchItem *bi = getSelectedBranch();
    if (bi) {
        saveState(bi, "setVymLink (\"" + bi->getVymLink() + "\")", bi,
                  "setVymLink (\"\")",
                  QString("Unset vymlink of %1").arg(getObjectName(bi)));
        bi->setVymLink("");
        emitDataChanged(bi);
        reposition();
        updateActions();
    }
}

QString VymModel::getVymLink()
{
    BranchItem *bi = getSelectedBranch();
    if (bi)
        return bi->getVymLink();
    else
        return "";
}

QStringList VymModel::getVymLinks()
{
    QStringList links;
    BranchItem *selbi = getSelectedBranch();
    BranchItem *cur = NULL;
    BranchItem *prev = NULL;
    nextBranch(cur, prev, true, selbi);
    while (cur) {
        if (!cur->getVymLink().isEmpty())
            links.append(cur->getVymLink());
        nextBranch(cur, prev, true, selbi);
    }
    return links;
}

void VymModel::followXLink(int i)
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi) {
        selbi = selbi->getXLinkItemNum(i)->getPartnerBranch();
        if (selbi)
            select(selbi);
    }
}

void VymModel::editXLink()
{
    Link *l = getSelectedXLink();
    if (l) {
        EditXLinkDialog dia;
        dia.setLink(l);
        if (dia.exec() == QDialog::Accepted) {
            if (dia.useSettingsGlobal()) {
                setMapDefXLinkPen(l->getPen());
                setMapDefXLinkStyleBegin(l->getStyleBeginString());
                setMapDefXLinkStyleEnd(l->getStyleEndString());
            }
        }
    }
}

void VymModel::setXLinkColor(const QString &new_col)
{
    Link *l = getSelectedXLink();
    if (l) {
        QPen pen = l->getPen();
        QColor new_color = QColor(new_col);
        QColor old_color = pen.color();
        if (new_color == old_color)
            return;
        pen.setColor(new_color);
        l->setPen(pen);
        saveState(l->getBeginLinkItem(),
                  QString("setXLinkColor(\"%1\")").arg(old_color.name()),
                  l->getBeginLinkItem(),
                  QString("setXLinkColor(\"%1\")").arg(new_color.name()),
                  QString("set color of xlink to %1").arg(new_color.name()));
    }
}

void VymModel::setXLinkStyle(const QString &new_style)
{
    Link *l = getSelectedXLink();
    if (l) {
        QPen pen = l->getPen();
        QString old_style = penStyleToString(pen.style());
        if (new_style == old_style)
            return;
        bool ok;
        pen.setStyle(penStyle(new_style, ok));
        l->setPen(pen);
        saveState(l->getBeginLinkItem(),
                  QString("setXLinkStyle(\"%1\")").arg(old_style),
                  l->getBeginLinkItem(),
                  QString("setXLinkStyle(\"%1\")").arg(new_style),
                  QString("set style of xlink to %1").arg(new_style));
    }
}

void VymModel::setXLinkStyleBegin(const QString &new_style)
{
    Link *l = getSelectedXLink();
    if (l) {
        QString old_style = l->getStyleBeginString();
        if (new_style == old_style)
            return;
        l->setStyleBegin(new_style);
        saveState(l->getBeginLinkItem(),
                  QString("setXLinkStyleBegin(\"%1\")").arg(old_style),
                  l->getBeginLinkItem(),
                  QString("setXLinkStyleBegin(\"%1\")").arg(new_style),
                  "set style of xlink begin");
    }
}

void VymModel::setXLinkStyleEnd(const QString &new_style)
{
    Link *l = getSelectedXLink();
    if (l) {
        QString old_style = l->getStyleEndString();
        if (new_style == old_style)
            return;
        l->setStyleEnd(new_style);
        saveState(l->getBeginLinkItem(),
                  QString("setXLinkStyleEnd(\"%1\")").arg(old_style),
                  l->getBeginLinkItem(),
                  QString("setXLinkStyleEnd(\"%1\")").arg(new_style),
                  "set style of xlink end");
    }
}

void VymModel::setXLinkWidth(int new_width)
{
    Link *l = getSelectedXLink();
    if (l) {
        QPen pen = l->getPen();
        int old_width = pen.width();
        if (new_width == old_width)
            return;
        pen.setWidth(new_width);
        l->setPen(pen);
        saveState(
            l->getBeginLinkItem(), QString("setXLinkWidth(%1)").arg(old_width),
            l->getBeginLinkItem(), QString("setXLinkWidth(%1)").arg(new_width),
            "set width of xlink");
    }
}

//////////////////////////////////////////////
// Scripting
//////////////////////////////////////////////

QVariant VymModel::execute(
    const QString &script) // FIXME-3 still required???
                           // Called from these places:
                           //
                           // scripts/vym-ruby.rb  (and adaptormodel) used for
                           // testing Main::callMacro Main::checkReleaseNotes
                           // VymModel::undo
                           // VymModel::redo
                           // VymModel::exportLast
                           // VymModel::updateSlideSelection
{
    // qDebug()<<"VM::execute called: "<<script;
    return mainWindow->runScript(script);
}

void VymModel::setExportMode(bool b)
{
    // should be called before and after exports
    // depending on the settings
    if (b && settings.value("/export/useHideExport", "true") == "true")
        setHideTmpMode(TreeItem::HideExport);
    else
        setHideTmpMode(TreeItem::HideNone);
}

QPointF VymModel::exportImage(QString fname, bool askName, QString format)
{
    QPointF offset; // set later, when getting image from MapEditor

    if (fname == "") {
        if (!askName) {
            qWarning("VymModel::exportImage called without filename (and "
                     "askName==false)");
            return offset;
        }

        fname = lastImageDir.absolutePath() + "/" + getMapName() + ".png";
        format = "PNG";
    }

    ExportBase ex;
    ex.setName("Image");
    ex.setModel(this);
    ex.setFilePath(fname);
    ex.setWindowTitle(tr("Export map as image"));
    ex.addFilter(
        "PNG (*.png);;All (* *.*)"); //  imageIO.getFilters().join(";;")
    ex.setLastCommand(
        settings.localValue(filePath, "/export/last/command", "").toString());

    if (askName) {
        if (!ex.execDialog())
            return offset;
        fname = ex.getFilePath();
        lastImageDir = QDir(fname);
    }

    setExportMode(true);

    QImage img(mapEditor->getImage(offset));
    if (!img.save(fname, format.toLocal8Bit()))
        QMessageBox::critical(
            0, tr("Critical Error"),
            tr("Couldn't save QImage %1 in format %2").arg(fname).arg(format));
    setExportMode(false);

    ex.completeExport();

    return offset;
}

void VymModel::exportPDF(QString fname, bool askName)
{
    if (fname == "") {
        if (!askName) {
            qWarning("VymModel::exportPDF called without filename (and "
                     "askName==false)");
            return;
        }

        fname = lastExportDir.absolutePath() + "/" + getMapName() + ".pdf";
    }

    ExportBase ex;
    ex.setName("PDF");
    ex.setModel(this);
    ex.setFilePath(fname);
    ex.setWindowTitle(tr("Export map as PDF"));
    ex.addFilter("PDF (*.pdf);;All (* *.*)");
    ex.setLastCommand(
        settings.localValue(filePath, "/export/last/command", "").toString());

    if (askName) {
        if (!ex.execDialog())
            return;
        fname = ex.getFilePath();
    }

    setExportMode(true);

    // To PDF
    QPrinter pdfPrinter(QPrinter::HighResolution);
    pdfPrinter.setOutputFormat(QPrinter::PdfFormat);
    pdfPrinter.setOutputFileName(fname);
    pdfPrinter.setPageSize(QPageSize(QPageSize::A3));

    QRectF bbox = mapEditor->getTotalBBox();
    if (bbox.width() > bbox.height())
        // recommend landscape
        pdfPrinter.setPageOrientation(QPageLayout::Landscape);
    else
        // recommend portrait
        pdfPrinter.setPageOrientation(QPageLayout::Portrait);

    QPainter *pdfPainter = new QPainter(&pdfPrinter);
    getScene()->render(pdfPainter);
    pdfPainter->end();
    delete pdfPainter;

    setExportMode(false);

    ex.completeExport();
}

QPointF VymModel::exportSVG(QString fname, bool askName)
{
    QPointF offset; // FIXME-3 not needed?

    if (fname == "") {
        if (!askName) {
            qWarning("VymModel::exportSVG called without filename (and "
                     "askName==false)");
            return offset;
        }

        fname = lastImageDir.absolutePath() + "/" + getMapName() + ".png";
    }

    ExportBase ex;
    ex.setName("SVG");
    ex.setModel(this);
    ex.setFilePath(fname);
    ex.setWindowTitle(tr("Export map as SVG"));
    ex.addFilter("SVG (*.svg);;All (* *.*)");

    if (askName) {
        if (!ex.execDialog())
            return offset;
        fname = ex.getFilePath();
        lastImageDir = QDir(fname);
    }

    setExportMode(true);

    QSvgGenerator generator;
    generator.setFileName(fname);
    QSize sceneSize = getScene()->sceneRect().size().toSize();
    generator.setSize(sceneSize);
    generator.setViewBox(QRect(0, 0, sceneSize.width(), sceneSize.height()));
    QPainter *svgPainter = new QPainter(&generator);
    getScene()->render(svgPainter);
    svgPainter->end();
    delete svgPainter;

    setExportMode(false);
    ex.completeExport();

    return offset;
}

void VymModel::exportXML(QString fpath, QString dpath, bool useDialog)
{
    ExportBase ex;
    ex.setName("XML");
    ex.setModel(this);
    ex.setWindowTitle(tr("Export map as XML"));
    ex.addFilter("XML (*.xml);;All (* *.*)");
    ex.setLastCommand(
        settings.localValue(filePath, "/export/last/command", "").toString());

    if (useDialog) {
        QFileDialog fd;
        fd.setWindowTitle(vymName + " - " + tr("Export XML to directory"));
        QStringList filters;
        filters << "XML data (*.xml)";
        fd.setNameFilters(filters);
        fd.setOption(QFileDialog::DontConfirmOverwrite, true);
        fd.setAcceptMode(QFileDialog::AcceptSave);
        fd.selectFile(mapName + ".xml");

        QString fn;
        if (fd.exec() != QDialog::Accepted || fd.selectedFiles().isEmpty())
            return;

        fpath = fd.selectedFiles().first();
        dpath = fpath.left(fpath.lastIndexOf("/"));

        if (!confirmDirectoryOverwrite(QDir(dpath)))
            return;
    }
    ex.setFilePath(fpath);

    QString mname = basename(fpath);

    // Hide stuff during export, if settings want this
    setExportMode(true);

    // Create subdirectories
    makeSubDirs(dpath);

    // write image and calculate offset (Remember old mapSaved setting while
    // exporting image)
    bool mchanged = mapChanged;
    bool munsaved = mapUnsaved;

    QPointF offset =
        exportImage(dpath + "/images/" + mname + ".png", false, "PNG");

    mapChanged = mchanged;
    mapUnsaved = munsaved;

    // write to directory   //FIXME-3 check totalBBox here...
    QString saveFile =
        saveToDir(dpath, mname + "-", FlagRowMaster::NoFlags, offset, NULL);
    QFile file;

    file.setFileName(fpath);
    if (!file.open(QIODevice::WriteOnly)) {
        // This should neverever happen
        QMessageBox::critical(0, tr("Critical Export Error"),
                              QString("VymModel::exportXML couldn't open %1")
                                  .arg(file.fileName()));
        return;
    }

    // Write it finally, and write in UTF8, no matter what
    QTextStream ts(&file);
    ts.setCodec("UTF-8");
    ts << saveFile;
    file.close();

    setExportMode(false);

    ex.completeExport(QStringList(dpath));
}

void VymModel::exportAO(QString fname, bool askName)
{
    ExportAO ex;
    ex.setModel(this);
    ex.setLastCommand(
        settings.localValue(filePath, "/export/last/command", "").toString());

    if (fname == "")
        ex.setFilePath(mapName + ".txt");
    else
        ex.setFilePath(fname);

    if (askName) {
        ex.setDirPath(lastExportDir.absolutePath());
        ex.execDialog();
    }
    if (!ex.canceled()) {
        setExportMode(true);
        ex.doExport();
        setExportMode(false);
    }
}

void VymModel::exportASCII(const QString &fname, bool listTasks, bool askName)
{
    ExportASCII ex;
    ex.setModel(this);
    ex.setListTasks(listTasks);
    ex.setLastCommand(
        settings.localValue(filePath, "/export/last/command", "").toString());

    if (fname == "")
        ex.setFilePath(mapName + ".txt");
    else
        ex.setFilePath(fname);

    if (askName) {
        ex.setDirPath(lastExportDir.absolutePath());
        ex.execDialog();
    }

    if (!ex.canceled()) {
        setExportMode(true);
        ex.doExport();
        setExportMode(false);
    }
}

void VymModel::exportCSV(const QString &fname, bool askName)
{
    ExportCSV ex;
    ex.setModel(this);
    ex.setLastCommand(
        settings.localValue(filePath, "/export/last/command", "").toString());

    if (fname == "")
        ex.setFilePath(mapName + ".csv");
    else
        ex.setFilePath(fname);

    if (askName) {
        ex.addFilter("CSV (*.csv);;All (* *.*)");
        ex.setDirPath(lastExportDir.absolutePath());
        ex.setWindowTitle(vymName + " -" + tr("Export as csv") + " " +
                          tr("(still experimental)"));
        ex.execDialog();
    }

    if (!ex.canceled()) {
        setExportMode(true);
        ex.doExport();
        setExportMode(false);
    }
}

void VymModel::exportFirefoxBookmarks(const QString &fname, bool askName)
{
    ExportFirefox ex;
    ex.setModel(this);
    ex.setLastCommand(
        settings.localValue(filePath, "/export/last/command", "").toString());

    if (fname == "")
        ex.setFilePath(mapName + ".csv");
    else
        ex.setFilePath(fname);

    if (askName) {
        ex.addFilter("JSON (*.json);;All (* *.*)");
        ex.setDirPath(lastExportDir.absolutePath());
        ex.setWindowTitle(vymName + " -" + tr("Export as csv") + " " +
                          tr("(still experimental)"));
        ex.execDialog();
    }

    if (!ex.canceled()) {
        setExportMode(true);
        ex.doExport();
        setExportMode(false);
    }
}

void VymModel::exportHTML(const QString &fpath, const QString &dpath,
                          bool useDialog)
{
    ExportHTML ex(this);
    ex.setLastCommand(
        settings.localValue(filePath, "/export/last/command", "").toString());

    if (!dpath.isEmpty())
        ex.setDirPath(dpath);
    if (!fpath.isEmpty())
        ex.setFilePath(fpath);

    ex.doExport(useDialog);
}

void VymModel::exportConfluence(bool createPage, const QString &pageURL,
                                const QString &pageTitle, bool useDialog)
{
    ExportConfluence ex(this);
    ex.setCreateNewPage(createPage);
    ex.setURL(pageURL);
    ex.setPageTitle(pageTitle);
    ex.setLastCommand(
        settings.localValue(filePath, "/export/last/command", "").toString());

    ex.doExport(useDialog);
}

void VymModel::exportImpress(const QString &fn, const QString &cf)
{
    ExportOO ex;
    ex.setFilePath(fn);
    ex.setModel(this);
    ex.setLastCommand(
        settings.localValue(filePath, "/export/last/command", "").toString());

    if (ex.setConfigFile(cf)) {
        QString lastCommand =
            settings.localValue(filePath, "/export/last/command", "")
                .toString();

        setExportMode(true);
        ex.exportPresentation();
        setExportMode(false);

        QString command =
            settings.localValue(filePath, "/export/last/command", "")
                .toString();
        if (lastCommand != command)
            setChanged();
    }
}

bool VymModel::exportLastAvailable(QString &description, QString &command,
                                   QString &dest)
{
    command =
        settings.localValue(filePath, "/export/last/command", "").toString();
    description = settings.localValue(filePath, "/export/last/description", "")
                      .toString();
    dest = settings.localValue(filePath, "/export/last/destination", "")
               .toString();
    if (!command.isEmpty() && command.contains("exportMap"))
        return true;
    else
        return false;
}

void VymModel::exportLast()
{
    QString desc, command,
        dest; // FIXME-3 better integrate configFile into command
    if (exportLastAvailable(desc, command, dest)) {
        execute(command);
    }
}

void VymModel::exportLaTeX(const QString &fname, bool askName)
{
    ExportLaTeX ex;
    ex.setModel(this);
    ex.setLastCommand(
        settings.localValue(filePath, "/export/last/command", "").toString());

    if (fname == "")
        ex.setFilePath(mapName + ".tex");
    else
        ex.setFilePath(fname);

    if (askName)
        ex.execDialog();
    if (!ex.canceled()) {
        setExportMode(true);
        ex.doExport();
        setExportMode(false);
    }
}

void VymModel::exportOrgMode(const QString &fname, bool askName)
{
    ExportOrgMode ex;
    ex.setModel(this);
    ex.setLastCommand(
        settings.localValue(filePath, "/export/last/command", "").toString());

    if (fname == "")
        ex.setFilePath(mapName + ".org");
    else
        ex.setFilePath(fname);

    if (askName) {
        ex.setDirPath(lastExportDir.absolutePath());
        ex.execDialog();
    }

    if (!ex.canceled()) {
        setExportMode(true);
        ex.doExport();
        setExportMode(false);
    }
}

void VymModel::exportMarkdown(const QString &fname, bool askName)
{
    ExportMarkdown ex;
    ex.setModel(this);
    ex.setLastCommand(
        settings.localValue(filePath, "/export/last/command", "").toString());

    if (fname == "")
        ex.setFilePath(mapName + ".org");
    else
        ex.setFilePath(fname);

    if (askName) {
        ex.setDirPath(lastExportDir.absolutePath());
        ex.execDialog();
    }

    if (!ex.canceled()) {
        setExportMode(true);
        ex.doExport();
        setExportMode(false);
    }
}
//////////////////////////////////////////////
// View related
//////////////////////////////////////////////

void VymModel::registerEditor(QWidget *me) { mapEditor = (MapEditor *)me; }

void VymModel::unregisterEditor(QWidget *) { mapEditor = NULL; }

void VymModel::setMapZoomFactor(const double &d)
{
    zoomFactor = d;
    mapEditor->setZoomFactorTarget(d);
}

void VymModel::setMapRotationAngle(const double &d)
{
    rotationAngle = d;
    mapEditor->setAngleTarget(d);
}

void VymModel::setMapAnimDuration(const int &d) { animDuration = d; }

void VymModel::setMapAnimCurve(const QEasingCurve &c) { animCurve = c; }

bool VymModel::centerOnID(const QString &id)
{
    TreeItem *ti = findUuid(QUuid(id));
    if (ti) {
        LinkableMapObj *lmo = ((MapItem *)ti)->getLMO();
        if (zoomFactor > 0 && lmo) {
            mapEditor->setViewCenterTarget(lmo->getBBox().center(), zoomFactor,
                                           rotationAngle, animDuration,
                                           animCurve);
            return true;
        }
    }
    return false;
}

void VymModel::setContextPos(QPointF p)
{
    contextPos = p;
    hasContextPos = true;
}

void VymModel::unsetContextPos()
{
    contextPos = QPointF();
    hasContextPos = false;
}

void VymModel::reposition() // FIXME-4 VM should have no need to reposition, but
                            // the views...
{
    if (repositionBlocked)
        return;

    BranchObj *bo;
    for (int i = 0; i < rootItem->branchCount(); i++) {
        bo = rootItem->getBranchObjNum(i);
        if (bo)
            bo->reposition(); //  for positioning heading
        else
            qDebug() << "VM::reposition bo=0";
    }
    mapEditor->getTotalBBox();

    // required to *reposition* the selection box. size is already correct:
    emitSelectionChanged();

    // Reposition also containers   // FIXME-0 testing
    BranchItem *bi;
    for (int i = 0; i < rootItem->branchCount(); i++) {
        bi = rootItem->getBranchNum(i);
        bi->getBranchObj()->repositionContainers();
    }
}

bool VymModel::setMapLinkStyle(const QString &s)
{
    QString snow;
    switch (linkstyle) {
    case LinkableMapObj::Line:
        snow = "StyleLine";
        break;
    case LinkableMapObj::Parabel:
        snow = "StyleParabel";
        break;
    case LinkableMapObj::PolyLine:
        snow = "StylePolyLine";
        break;
    case LinkableMapObj::PolyParabel:
        snow = "StylePolyParabel";
        break;
    default:
        return false;
        break;
    }

    saveState(QString("setMapLinkStyle (\"%1\")").arg(s),
              QString("setMapLinkStyle (\"%1\")").arg(snow),
              QString("Set map link style (\"%1\")").arg(s));

    if (s == "StyleLine")
        linkstyle = LinkableMapObj::Line;
    else if (s == "StyleParabel")
        linkstyle = LinkableMapObj::Parabel;
    else if (s == "StylePolyLine")
        linkstyle = LinkableMapObj::PolyLine;
    else if (s == "StylePolyParabel")
        linkstyle = LinkableMapObj::PolyParabel;
    else
        linkstyle = LinkableMapObj::UndefinedStyle;

    BranchItem *cur = NULL;
    BranchItem *prev = NULL;
    BranchObj *bo;
    nextBranch(cur, prev);
    while (cur) {
        bo = (BranchObj *)(cur->getLMO());
        bo->setLinkStyle(bo->getDefLinkStyle(
            cur->parent())); // FIXME-4 better emit dataCHanged and leave the
                             // changes to View
        nextBranch(cur, prev);
    }
    reposition();
    return true;
}

LinkableMapObj::Style VymModel::getMapLinkStyle() { return linkstyle; }

uint VymModel::getModelID() { return modelID; }

void VymModel::setView(VymView *vv) { vymView = vv; }

void VymModel::setMapDefLinkColor(QColor col)
{
    if (!col.isValid())
        return;
    saveState(
        QString("setMapDefLinkColor (\"%1\")").arg(getMapDefLinkColor().name()),
        QString("setMapDefLinkColor (\"%1\")").arg(col.name()),
        QString("Set map link color to %1").arg(col.name()));

    defLinkColor = col;
    BranchItem *cur = NULL;
    BranchItem *prev = NULL;
    BranchObj *bo;
    nextBranch(cur, prev);
    while (cur) {
        bo = (BranchObj *)(cur->getLMO());
        bo->setLinkColor();

        for (int i = 0; i < cur->imageCount(); ++i)
            cur->getImageNum(i)->getLMO()->setLinkColor();

        nextBranch(cur, prev);
    }
    updateActions();
}

void VymModel::setMapLinkColorHintInt()
{
    // called from setMapLinkColorHint(lch) or at end of parse
    BranchItem *cur = NULL;
    BranchItem *prev = NULL;
    BranchObj *bo;
    nextBranch(cur, prev);
    while (cur) {
        bo = (BranchObj *)(cur->getLMO());
        bo->setLinkColor();

        for (int i = 0; i < cur->imageCount(); ++i)
            cur->getImageNum(i)->getLMO()->setLinkColor();

        nextBranch(cur, prev);
    }
}

void VymModel::setMapLinkColorHint(LinkableMapObj::ColorHint lch)
{
    linkcolorhint = lch;
    setMapLinkColorHintInt();
}

void VymModel::toggleMapLinkColorHint()
{
    if (linkcolorhint == LinkableMapObj::HeadingColor)
        linkcolorhint = LinkableMapObj::DefaultColor;
    else
        linkcolorhint = LinkableMapObj::HeadingColor;
    BranchItem *cur = NULL;
    BranchItem *prev = NULL;
    BranchObj *bo;
    nextBranch(cur, prev);
    while (cur) {
        bo = (BranchObj *)(cur->getLMO());
        bo->setLinkColor();

        for (int i = 0; i < cur->imageCount(); ++i)
            cur->getImageNum(i)->getLMO()->setLinkColor();

        nextBranch(cur, prev);
    }
}

void VymModel::
    selectMapBackgroundImage() // FIXME-3 for using background image:
                               // view.setCacheMode(QGraphicsView::CacheBackground);
                               // Also this belongs into ME
{
    QStringList filters;
    filters << tr("Images") +
                   " (*.png *.bmp *.xbm *.jpg *.png *.xpm *.gif *.pnm)";
    QFileDialog fd;
    fd.setFileMode(QFileDialog::ExistingFile);
    fd.setWindowTitle(vymName + " - " + tr("Load background image"));
    fd.setDirectory(lastImageDir);
    fd.setAcceptMode(QFileDialog::AcceptOpen);

    if (fd.exec() == QDialog::Accepted && !fd.selectedFiles().isEmpty()) {
        // TODO selectMapBackgroundImg in QT4 use:  lastImageDir=fd.directory();
        lastImageDir = QDir(fd.directory().path());
        setMapBackgroundImage(fd.selectedFiles().first());
    }
}

void VymModel::setMapBackgroundImage(
    const QString &fn) // FIXME-3 missing savestate, move to ME
{
    /*
    QColor oldcol=mapEditor->getScene()->backgroundBrush().color();
    saveState(
    selection,
    QString ("setMapBackgroundImage (%1)").arg(oldcol.name()),
    selection,
    QString ("setMapBackgroundImage (%1)").arg(col.name()),
    QString("Set background color of map to %1").arg(col.name()));
    */
    QBrush brush;
    brush.setTextureImage(QImage(fn));
    mapEditor->getScene()->setBackgroundBrush(brush);
}

void VymModel::selectMapBackgroundColor()
{
    QColor col = QColorDialog::getColor(
        mapEditor->getScene()->backgroundBrush().color(), NULL);
    if (!col.isValid())
        return;
    setMapBackgroundColor(col);
}

void VymModel::setMapBackgroundColor(QColor col) // FIXME-4 move to ME
{
    QColor oldcol = mapEditor->getScene()->backgroundBrush().color();
    saveState(QString("setMapBackgroundColor (\"%1\")").arg(oldcol.name()),
              QString("setMapBackgroundColor (\"%1\")").arg(col.name()),
              QString("Set background color of map to %1").arg(col.name()));
    mapEditor->getScene()->setBackgroundBrush(col);
}

QColor VymModel::getMapBackgroundColor() // FIXME-4 move to ME
{
    return mapEditor->getScene()->backgroundBrush().color();
}

QFont VymModel::getMapDefaultFont() { return defaultFont; }

void VymModel::setMapDefaultFont(const QFont &f) { defaultFont = f; }

LinkableMapObj::ColorHint VymModel::getMapLinkColorHint() // FIXME-4 move to ME
{
    return linkcolorhint;
}

QColor VymModel::getMapDefLinkColor() // FIXME-4 move to ME
{
    return defLinkColor;
}

void VymModel::setMapDefXLinkPen(const QPen &p) // FIXME-4 move to ME
{
    defXLinkPen = p;
}

QPen VymModel::getMapDefXLinkPen() // FIXME-4 move to ME
{
    return defXLinkPen;
}

void VymModel::setMapDefXLinkStyleBegin(const QString &s)
{
    defXLinkStyleBegin = s;
}

QString VymModel::getMapDefXLinkStyleBegin() { return defXLinkStyleBegin; }

void VymModel::setMapDefXLinkStyleEnd(const QString &s)
{
    defXLinkStyleEnd = s;
}

QString VymModel::getMapDefXLinkStyleEnd() { return defXLinkStyleEnd; }

void VymModel::move(const double &x, const double &y)
{
    MapItem *seli = (MapItem *)getSelectedItem();
    if (seli &&
        (seli->isBranchLikeType() || seli->getType() == TreeItem::Image)) {
        LinkableMapObj *lmo = seli->getLMO();
        if (lmo) {
            QPointF ap(lmo->getAbsPos());
            QPointF to(x, y);
            if (ap != to) {
                QString ps = qpointFToString(ap);
                QString s = getSelectString(seli);
                saveState(
                    s, "move " + ps, s, "move " + qpointFToString(to),
                    QString("Move %1 to %2").arg(getObjectName(seli)).arg(ps));
                lmo->move(x, y);
                reposition();
                emitSelectionChanged();
            }
        }
    }
}

void VymModel::moveRel(const double &x, const double &y)
{
    MapItem *seli = (MapItem *)getSelectedItem();
    if (seli &&
        (seli->isBranchLikeType() || seli->getType() == TreeItem::Image)) {
        LinkableMapObj *lmo = seli->getLMO();
        if (lmo) {
            QPointF rp(lmo->getRelPos());
            QPointF to(x, y);
            if (rp != to) {
                QString ps = qpointFToString(lmo->getRelPos());
                QString s = getSelectString(seli);
                saveState(s, "moveRel " + ps, s,
                          "moveRel " + qpointFToString(to),
                          QString("Move %1 to relative position %2")
                              .arg(getObjectName(seli))
                              .arg(ps));
                ((OrnamentedObj *)lmo)->move2RelPos(x, y);
                reposition();
                lmo->updateLinkGeometry();
                emitSelectionChanged();
            }
        }
    }
}

void VymModel::animate()
{
    animationTimer->stop();
    BranchObj *bo;
    int i = 0;
    while (i < animObjList.size()) {
        bo = (BranchObj *)animObjList.at(i);
        if (!bo->animate()) {
            if (i >= 0) {
                animObjList.removeAt(i);
                i--;
            }
        }
        bo->reposition();
        i++;
    }
    emitSelectionChanged();

    if (!animObjList.isEmpty())
        animationTimer->start(animationInterval);
}

void VymModel::startAnimation(BranchObj *bo, const QPointF &v)
{
    if (!bo)
        return;

    if (bo->getUseRelPos())
        startAnimation(bo, bo->getRelPos(), bo->getRelPos() + v);
    else
        startAnimation(bo, bo->getAbsPos(), bo->getAbsPos() + v);
}

void VymModel::startAnimation(BranchObj *bo, const QPointF &start,
                              const QPointF &dest)
{
    if (start == dest)
        return;
    if (bo && bo->getTreeItem()->depth() >= 0) {
        AnimPoint ap;
        ap.setStart(start);
        ap.setDest(dest);
        ap.setTicks(animationTicks);
        ap.setAnimated(true);
        bo->setAnimation(ap);
        if (!animObjList.contains(bo))
            animObjList.append(bo);
        animationTimer->setSingleShot(true);
        animationTimer->start(animationInterval);
    }
}

void VymModel::stopAnimation(MapObj *mo)
{
    int i = animObjList.indexOf(mo);
    if (i >= 0)
        animObjList.removeAt(i);
}

void VymModel::stopAllAnimation()
{
    BranchObj *bo;
    int i = 0;
    while (i < animObjList.size()) {
        bo = (BranchObj *)animObjList.at(i);
        bo->stopAnimation();
        bo->requestReposition();
        i++;
    }
    reposition();
}

void VymModel::sendSelection()
{
    if (netstate != Server)
        return;
    sendData(QString("select (\"%1\")").arg(getSelectString()));
}

void VymModel::newServer()
{
    port = 54321;
    sendCounter = 0;
    tcpServer = new QTcpServer(this);
    if (!tcpServer->listen(QHostAddress::Any, port)) {
        QMessageBox::critical(NULL, "vym server",
                              QString("Unable to start the server: %1.")
                                  .arg(tcpServer->errorString()));
        // FIXME-3 needed? we are no widget any longer... close();
        return;
    }
    connect(tcpServer, SIGNAL(newConnection()), this, SLOT(newClient()));
    netstate = Server;
    qDebug() << "Server is running on port " << tcpServer->serverPort();
}

void VymModel::connectToServer()
{
    port = 54321;
    server = "salam.suse.de";
    server = "localhost";
    clientSocket = new QTcpSocket(this);
    clientSocket->abort();
    clientSocket->connectToHost(server, port);
    connect(clientSocket, SIGNAL(readyRead()), this, SLOT(readData()));
    connect(clientSocket, SIGNAL(error(QAbstractSocket::SocketError)), this,
            SLOT(displayNetworkError(QAbstractSocket::SocketError)));
    netstate = Client;
    qDebug() << "connected to " << qPrintable(server) << " port " << port;
}

void VymModel::newClient()
{
    QTcpSocket *newClient = tcpServer->nextPendingConnection();
    connect(newClient, SIGNAL(disconnected()), newClient, SLOT(deleteLater()));

    qDebug() << "ME::newClient  at "
             << qPrintable(newClient->peerAddress().toString());

    clientList.append(newClient);
}

void VymModel::sendData(const QString &s)
{
    if (clientList.size() == 0)
        return;

    // Create bytearray to send
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_0);

    // Reserve some space for blocksize
    out << (quint16)0;

    // Write sendCounter
    out << sendCounter++;

    // Write data
    out << s;

    // Go back and write blocksize so far
    out.device()->seek(0);
    quint16 bs = (quint16)(block.size() - 2 * sizeof(quint16));
    out << bs;

    if (debug)
        qDebug() << "ME::sendData  bs=" << bs << "  counter=" << sendCounter
                 << "  s=" << qPrintable(s);

    for (int i = 0; i < clientList.size(); ++i) {
        // qDebug() << "Sending \""<<qPrintable (s)<<"\" to "<<qPrintable
        // (clientList.at(i)->peerAddress().toString());
        clientList.at(i)->write(block);
    }
}

void VymModel::readData()
{
    while (clientSocket->bytesAvailable() >= (int)sizeof(quint16)) {
        if (debug)
            qDebug() << "readData  bytesAvail="
                     << clientSocket->bytesAvailable();
        quint16 recCounter;
        quint16 blockSize;

        QDataStream in(clientSocket);
        in.setVersion(QDataStream::Qt_4_0);

        in >> blockSize;
        in >> recCounter;

        QString t;
        in >> t;
        if (debug)
            qDebug() << "VymModel::readData  command=" << qPrintable(t);
        // bool noErr;
        // QString errMsg;
        // parseAtom (t,noErr,errMsg);    //FIXME-4 needs rework using scripts
    }
    return;
}

void VymModel::displayNetworkError(QAbstractSocket::SocketError socketError)
{
    switch (socketError) {
    case QAbstractSocket::RemoteHostClosedError:
        break;
    case QAbstractSocket::HostNotFoundError:
        QMessageBox::information(NULL, vymName + " Network client",
                                 "The host was not found. Please check the "
                                 "host name and port settings.");
        break;
    case QAbstractSocket::ConnectionRefusedError:
        QMessageBox::information(NULL, vymName + " Network client",
                                 "The connection was refused by the peer. "
                                 "Make sure the fortune server is running, "
                                 "and check that the host name and port "
                                 "settings are correct.");
        break;
    default:
        QMessageBox::information(NULL, vymName + " Network client",
                                 QString("The following error occurred: %1.")
                                     .arg(clientSocket->errorString()));
    }
}

void VymModel::downloadImage(const QUrl &url, BranchItem *bi)
{
    if (!bi)
        bi = getSelectedBranch();
    if (!bi) {
        qWarning("VM::download bi==NULL");
        return;
    }

    // FIXME-3 download img to tmpfile and delete after running script in
    // mainWindow
    QString script;
    script += QString("m = vym.currentMap();m.selectID(\"%1\");")
                  .arg(bi->getUuid().toString());
    script += QString("m.loadImage(\"$TMPFILE\");");

    DownloadAgent *agent = new DownloadAgent(url);
    agent->setFinishedAction(this, script);
    connect(agent, SIGNAL(downloadFinished()), mainWindow,
            SLOT(downloadFinished()));
    QTimer::singleShot(0, agent, SLOT(execute()));
}

void VymModel::selectMapSelectionColor()
{
    QColor col = QColorDialog::getColor(defLinkColor, NULL);
    setSelectionColor(col);
}

void VymModel::setSelectionColorInt(QColor col)
{
    if (!col.isValid())
        return;
    saveState(QString("setSelectionColor (\"%1\")")
                  .arg(mapEditor->getSelectionColor().name()),
              QString("setSelectionColor (\"%1\")").arg(col.name()),
              QString("Set color of selection box to %1").arg(col.name()));

    mapEditor->setSelectionColor(col);
}

void VymModel::emitSelectionChanged(const QItemSelection &newsel)
{
    emit(selectionChanged(newsel,
                          newsel)); // needed e.g. to update geometry in editor
    sendSelection();
}

void VymModel::emitSelectionChanged()
{
    QItemSelection newsel = selModel->selection();
    emitSelectionChanged(newsel);
}

void VymModel::setSelectionColor(QColor col)
{
    if (!col.isValid())
        return;
    setSelectionColorInt(col);
}

QColor VymModel::getSelectionColor() { return mapEditor->getSelectionColor(); }

bool VymModel::initIterator(const QString &iname, bool deepLevelsFirst)
{
    Q_UNUSED(deepLevelsFirst);

    // Remove existing iterators first
    selIterCur.remove(iname);
    selIterPrev.remove(iname);
    selIterStart.remove(iname);
    selIterActive.remove(iname);

    QList<BranchItem *> selbis;
    selbis = getSelectedBranches();
    if (selbis.count() == 1) {
        BranchItem *prev = NULL;
        BranchItem *cur = NULL;
        nextBranch(cur, prev, false, selbis.first());
        if (cur) {
            selIterCur.insert(iname, cur->getUuid());
            selIterPrev.insert(iname, prev->getUuid());
            selIterStart.insert(iname, selbis.first()->getUuid());
            selIterActive.insert(iname, false);
            // qDebug() << "Created iterator " << iname;
            return true;
        }
    }
    return false;
}

bool VymModel::nextIterator(const QString &iname)
{
    if (selIterCur.keys().indexOf(iname) < 0) {
        qWarning()
            << QString("VM::nextIterator couldn't find %1 in hash of iterators")
                   .arg(iname);
        return false;
    }

    BranchItem *cur = (BranchItem *)(findUuid(selIterCur.value(iname)));
    if (!cur) {
        qWarning() << "VM::nextIterator couldn't find cur" << selIterCur;
        return false;
    }

    qDebug() << "  " << iname << "selecting " << cur->getHeadingPlain();
    select(cur);

    if (!selIterActive.value(iname)) {
        // Select for the first time
        select(cur);
        selIterActive[iname] = true;
        return true;
    }

    BranchItem *prev = (BranchItem *)(findUuid(selIterPrev.value(iname)));
    BranchItem *start = (BranchItem *)(findUuid(selIterStart.value(iname)));
    if (!prev)
        qWarning() << "VM::nextIterator couldn't find prev"
                   << selIterPrev.value(iname);
    if (!start)
        qWarning() << "VM::nextIterator couldn't find start "
                   << selIterStart.value(iname);

    if (cur && prev && start) {
        nextBranch(cur, prev, false, start);
        if (cur) {
            selIterCur[iname] = cur->getUuid();
            selIterPrev[iname] = prev->getUuid();
            select(cur);
            return true;
        }
        else
            return false;
    }
    return false;
}

void VymModel::setHideTmpMode(TreeItem::HideTmpMode mode)
{
    hidemode = mode;
    for (int i = 0; i < rootItem->branchCount(); i++)
        rootItem->getBranchNum(i)->setHideTmp(mode);
    reposition();
    if (mode == TreeItem::HideExport)
        unselectAll();
    else
        reselect();

    qApp->processEvents();
}

//////////////////////////////////////////////
// Selection related
//////////////////////////////////////////////

void VymModel::updateSelection(QItemSelection newsel, QItemSelection dsel)
{
    QModelIndex ix;
    MapItem *mi;
    BranchItem *bi;
    bool do_reposition = false;
    foreach (ix, dsel.indexes()) {
        mi = static_cast<MapItem *>(ix.internalPointer());
        if (mi->isBranchLikeType())
            do_reposition =
                do_reposition || ((BranchItem *)mi)->resetTmpUnscroll();
        if (mi->getType() == TreeItem::XLink) {
            Link *li = ((XLinkItem *)mi)->getLink();
            XLinkObj *xlo = li->getXLinkObj();
            if (xlo)
                xlo->setSelection(XLinkObj::Unselected);

            do_reposition =
                do_reposition || li->getBeginBranch()->resetTmpUnscroll();
            do_reposition =
                do_reposition || li->getEndBranch()->resetTmpUnscroll();
        }
    }

    foreach (ix, newsel.indexes()) {
        mi = static_cast<MapItem *>(ix.internalPointer());
        if (mi->isBranchLikeType()) {
            bi = (BranchItem *)mi;
            if (bi->hasScrolledParent()) {
                bi->tmpUnscroll();
                do_reposition = true;
            }
        }
        if (mi->getType() == TreeItem::XLink) {
            ((XLinkItem *)mi)->setSelection();

            // begin/end branches need to be tmp unscrolled
            Link *li = ((XLinkItem *)mi)->getLink();
            bi = li->getBeginBranch();
            if (bi->hasScrolledParent()) {
                bi->tmpUnscroll();
                do_reposition = true;
            }
            bi = li->getEndBranch();
            if (bi->hasScrolledParent()) {
                bi->tmpUnscroll();
                do_reposition = true;
            }
        }
    }
    if (do_reposition)
        reposition();
}

void VymModel::setSelectionModel(QItemSelectionModel *sm) { selModel = sm; }

QItemSelectionModel *VymModel::getSelectionModel() { return selModel; }

void VymModel::setSelectionBlocked(bool b) { selectionBlocked = b; }

bool VymModel::isSelectionBlocked() { return selectionBlocked; }

bool VymModel::select(const QString &s)
{
    if (s.isEmpty())
        return false;
    TreeItem *ti = findBySelectString(s);
    if (ti)
        return select(index(ti));
    return false;
}

bool VymModel::selectID(const QString &s)
{
    if (s.isEmpty())
        return false;
    TreeItem *ti = findUuid(QUuid(s));
    if (ti)
        return select(index(ti));
    return false;
}

bool VymModel::select(LinkableMapObj *lmo)
{
    QItemSelection oldsel = selModel->selection();

    if (lmo)
        return select(lmo->getTreeItem());
    else
        return false;
}

bool VymModel::selectToggle(TreeItem *ti)
{
    if (ti) {
        selModel->select(index(ti), QItemSelectionModel::Toggle);
        // appendSelectionToHistory();	// FIXME-4 selection history not implemented yet
        // for multiselections
        lastToggledUuid = ti->getUuid();
        return true;
    }
    return false;
}

bool VymModel::select(TreeItem *ti)
{
    if (ti)
        return select(index(ti));
    else
        return false;
}

bool VymModel::select(const QModelIndex &index)
{
    if (index.isValid()) {
        TreeItem *ti = getItem(index);
        if (ti->isBranchLikeType()) {
            if (((BranchItem *)ti)->tmpUnscroll())
                reposition();
        }
        selModel->select(index, QItemSelectionModel::ClearAndSelect);
        appendSelectionToHistory();
        return true;
    }
    return false;
}

void VymModel::unselectAll() { unselect(selModel->selection()); }

void VymModel::unselect(QItemSelection desel)
{
    if (!desel.isEmpty()) {
        lastSelectString = getSelectString();
        selModel->clearSelection();
    }
}

bool VymModel::reselect()
{
    bool b = select(lastSelectString);
    return b;
}

bool VymModel::canSelectPrevious()
{
    if (currentSelection > 0)
        return true;
    else
        return false;
}

bool VymModel::selectPrevious()
{
    keepSelectionHistory = true;
    bool result = false;
    while (currentSelection > 0) {
        currentSelection--;
        TreeItem *ti = findID(selectionHistory.at(currentSelection));
        if (ti) {
            result = select(ti);
            break;
        }
        else
            selectionHistory.removeAt(currentSelection);
    }
    keepSelectionHistory = false;
    return result;
}

bool VymModel::canSelectNext()
{
    if (currentSelection < selectionHistory.count() - 1)
        return true;
    else
        return false;
}

bool VymModel::selectNext()
{
    keepSelectionHistory = true;
    bool result = false;
    while (currentSelection < selectionHistory.count() - 1) {
        currentSelection++;
        TreeItem *ti = findID(selectionHistory.at(currentSelection));
        if (ti) {
            result = select(ti);
            break;
        }
        else
            selectionHistory.removeAt(currentSelection);
    }
    keepSelectionHistory = false;
    return result;
}

void VymModel::resetSelectionHistory()
{
    selectionHistory.clear();
    currentSelection = -1;
    keepSelectionHistory = false;
    appendSelectionToHistory();
}

void VymModel::appendSelectionToHistory() // FIXME-4 history unable to cope with multiple
                                          // selections
{
    uint id = 0;
    TreeItem *ti = getSelectedItem();
    if (ti && !keepSelectionHistory) {
        if (ti->isBranchLikeType())
            ((BranchItem *)ti)->setLastSelectedBranch();
        id = ti->getID();
        selectionHistory.append(id);
        currentSelection = selectionHistory.count() - 1;
        updateActions();
    }
}

void VymModel::emitShowSelection()
{
    if (!repositionBlocked)
        emit(showSelection());
}

TreeItem* VymModel::lastToggledItem()
{
    return findUuid(lastToggledUuid);
}

void VymModel::emitNoteChanged(TreeItem *ti)
{
    QModelIndex ix = index(ti);
    emit(noteChanged(ix));
    mainWindow->updateNoteEditor(ti);
}

void VymModel::emitDataChanged(TreeItem *ti)
{
    QModelIndex ix = index(ti);
    emit(dataChanged(ix, ix));
    emitSelectionChanged();
    if (!repositionBlocked) {
        // Update taskmodel and recalc priorities there
        if (ti->isBranchLikeType() && ((BranchItem *)ti)->getTask()) {
            taskModel->emitDataChanged(((BranchItem *)ti)->getTask());
            taskModel->recalcPriorities();
        }
    }
}

void VymModel::emitUpdateQueries()
{
    // Used to tell MainWindow to update query results
    if (repositionBlocked)
        return;
    emit(updateQueries(this));
}
void VymModel::emitUpdateLayout()
{
    if (settings.value("/mainwindow/autoLayout/use", "true") == "true")
        emit(updateLayout());
}

bool VymModel::selectFirstBranch()
{
    TreeItem *ti = getSelectedBranch();
    if (ti) {
        TreeItem *par = ti->parent();
        if (par) {
            TreeItem *ti2 = par->getFirstBranch();
            if (ti2)
                return select(ti2);
        }
    }
    return false;
}

bool VymModel::selectFirstChildBranch()
{
    TreeItem *ti = getSelectedBranch();
    if (ti) {
        BranchItem *bi = ti->getFirstBranch();
        if (bi)
            return select(bi);
    }
    return false;
}

bool VymModel::selectLastBranch()
{
    TreeItem *ti = getSelectedBranch();
    if (ti) {
        TreeItem *par = ti->parent();
        if (par) {
            TreeItem *ti2 = par->getLastBranch();
            if (ti2)
                return select(ti2);
        }
    }
    return false;
}

bool VymModel::selectLastChildBranch()
{
    TreeItem *ti = getSelectedBranch();
    if (ti) {
        BranchItem *bi = ti->getLastBranch();
        if (bi)
            return select(bi);
    }
    return false;
}

bool VymModel::selectLastSelectedBranch()
{
    BranchItem *bi = getSelectedBranch();
    if (bi) {
        bi = bi->getLastSelectedBranch();
        if (bi)
            return select(bi);
    }
    return false;
}

bool VymModel::selectLastImage()
{
    TreeItem *ti = getSelectedBranch();
    if (ti) {
        TreeItem *par = ti->parent();
        if (par) {
            TreeItem *ti2 = par->getLastImage();
            if (ti2)
                return select(ti2);
        }
    }
    return false;
}

bool VymModel::selectLatestAdded() { return select(latestAddedItem); }

bool VymModel::selectParent()
{
    TreeItem *ti = getSelectedItem();
    TreeItem *par;
    if (ti) {
        par = ti->parent();
        if (par)
            return select(par);
    }
    return false;
}

TreeItem::Type VymModel::selectionType()
{
    TreeItem *ti = getSelectedItem();
    if (ti)
        return ti->getType();
    else
        return TreeItem::Undefined;
}

LinkableMapObj *VymModel::getSelectedLMO()
{
    QModelIndexList list = selModel->selectedIndexes();
    if (list.count() == 1) {
        TreeItem *ti = getItem(list.first());
        TreeItem::Type type = ti->getType();
        if (type == TreeItem::Branch || type == TreeItem::MapCenter ||
            type == TreeItem::Image)
            return ((MapItem *)ti)->getLMO();
    }
    return NULL;
}

BranchObj *VymModel::getSelectedBranchObj() // convenience function
{
    TreeItem *ti = getSelectedBranch();
    if (ti)
        return (BranchObj *)(((MapItem *)ti)->getLMO());
    else
        return NULL;
}

BranchItem *VymModel::getSelectedBranch()
{
    TreeItem *ti = getSelectedItem();
    if (ti) {
        TreeItem::Type type = ti->getType();
        if (type == TreeItem::Branch || type == TreeItem::MapCenter)
            return (BranchItem *)ti;
    }
    return NULL;
}

QList<BranchItem *> VymModel::getSelectedBranches()
{
    QList<BranchItem *> bis;
    foreach (TreeItem *ti, getSelectedItems()) {
        TreeItem::Type type = ti->getType();
        if (type == TreeItem::Branch || type == TreeItem::MapCenter)
            bis.append((BranchItem *)ti);
    }
    return bis;
}

ImageItem *VymModel::getSelectedImage()
{
    TreeItem *ti = getSelectedItem();
    if (ti && ti->getType() == TreeItem::Image)
        return (ImageItem *)ti;
    else
        return NULL;
}

Task *VymModel::getSelectedTask()
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi)
        return selbi->getTask();
    else
        return NULL;
}

Link *VymModel::getSelectedXLink()
{
    XLinkItem *xli = getSelectedXLinkItem();
    if (xli)
        return xli->getLink();
    return NULL;
}

XLinkItem *VymModel::getSelectedXLinkItem()
{
    TreeItem *ti = getSelectedItem();
    if (ti && ti->getType() == TreeItem::XLink)
        return (XLinkItem *)ti;
    else
        return NULL;
}

AttributeItem *VymModel::getSelectedAttribute()
{
    TreeItem *ti = getSelectedItem();
    if (ti && ti->getType() == TreeItem::Attribute)
        return (AttributeItem *)ti;
    else
        return NULL;
}

TreeItem *VymModel::getSelectedItem()
{
    if (!selModel)
        return NULL;
    QModelIndexList list = selModel->selectedIndexes();
    if (list.count() == 1)
        return getItem(list.first());
    else
        return NULL;
}

QList<TreeItem *> VymModel::getSelectedItems()
{
    QList<TreeItem *> l;
    if (!selModel)
        return l;
    QModelIndexList list = selModel->selectedIndexes();
    foreach (QModelIndex ix, list)
        l.append(getItem(ix));
    return l;
}

QModelIndex VymModel::getSelectedIndex()
{
    QModelIndexList list = selModel->selectedIndexes();
    if (list.count() == 1)
        return list.first();
    else
        return QModelIndex();
}

QList<uint> VymModel::getSelectedIDs()
{
    QList<uint> uids;
    foreach (TreeItem *ti, getSelectedItems())
        uids.append(ti->getID());
    return uids;
}

QStringList VymModel::getSelectedUUIDs()
{
    QStringList uids;
    foreach (TreeItem *ti, getSelectedItems())
        uids.append(ti->getUuid().toString());
    return uids;
}

bool VymModel::isSelected(TreeItem *ti)
{
    return getSelectedItems().contains(ti);
}

QString VymModel::getSelectString()
{
    return getSelectString(getSelectedItem());
}

QString VymModel::getSelectString(
    LinkableMapObj *lmo) // only for convenience. Used in MapEditor
{
    if (!lmo)
        return QString();
    return getSelectString(lmo->getTreeItem());
}

QString VymModel::getSelectString(TreeItem *ti)
{
    QString s;
    if (!ti || ti->depth() < 0)
        return s;
    switch (ti->getType()) {
    case TreeItem::MapCenter:
        s = "mc:";
        break;
    case TreeItem::Branch:
        s = "bo:";
        break;
    case TreeItem::Image:
        s = "fi:";
        break;
    case TreeItem::Attribute:
        s = "ai:";
        break;
    case TreeItem::XLink:
        s = "xl:";
        break;
    default:
        s = "unknown type in VymModel::getSelectString()";
        break;
    }
    s = s + QString("%1").arg(ti->num());
    if (ti->depth() > 0)
        // call myself recursively
        s = getSelectString(ti->parent()) + "," + s;
    return s;
}

QString VymModel::getSelectString(BranchItem *bi)
{
    return getSelectString((TreeItem *)bi);
}

QString VymModel::getSelectString(const uint &i)
{
    return getSelectString(findID(i));
}

void VymModel::setLatestAddedItem(TreeItem *ti) { latestAddedItem = ti; }

TreeItem *VymModel::getLatestAddedItem() { return latestAddedItem; }

SlideModel *VymModel::getSlideModel() { return slideModel; }

int VymModel::slideCount() { return slideModel->count(); }

SlideItem *VymModel::addSlide()
{
    SlideItem *si = slideModel->getSelectedItem();
    if (si)
        si = slideModel->addSlide(NULL, si->childNumber() + 1);
    else
        si = slideModel->addSlide();

    TreeItem *seli = getSelectedItem();

    if (si && seli) {
        QString inScript;
        if (!loadStringFromDisk(vymBaseDir.path() +
                                    "/macros/slideeditor-snapshot.vys",
                                inScript)) {
            qWarning() << "VymModel::addSlide couldn't load template for "
                          "taking snapshot";
            return NULL;
        }

        inScript.replace(
            "CURRENT_ZOOM",
            QString().setNum(getMapEditor()->getZoomFactorTarget()));
        inScript.replace("CURRENT_ANGLE",
                         QString().setNum(getMapEditor()->getAngleTarget()));
        inScript.replace("CURRENT_ID",
                         "\"" + seli->getUuid().toString() + "\"");

        si->setInScript(inScript);
        slideModel->setData(slideModel->index(si), seli->getHeadingPlain());
    }
    QString s = "<vymmap>" + si->saveToDir() + "</vymmap>";
    int pos = si->childNumber();
    saveState(PartOfMap, getSelectString(),
              QString("removeSlide (%1)").arg(pos), getSelectString(),
              QString("addMapInsert (\"PATH\",%1)").arg(pos), "Add slide", NULL,
              s);
    return si;
}

void VymModel::deleteSlide(SlideItem *si)
{
    if (si) {
        QString s = "<vymmap>" + si->saveToDir() + "</vymmap>";
        int pos = si->childNumber();
        saveState(PartOfMap, getSelectString(),
                  QString("addMapInsert (\"PATH\",%1)").arg(pos),
                  getSelectString(), QString("removeSlide (%1)").arg(pos),
                  "Remove slide", NULL, s);
        slideModel->deleteSlide(si);
    }
}

void VymModel::deleteSlide(int n) { deleteSlide(slideModel->getSlide(n)); }

void VymModel::relinkSlide(SlideItem *si, int pos)
{
    if (si && pos >= 0)
        slideModel->relinkSlide(si, si->parent(), pos);
}

bool VymModel::moveSlideDown(int n)
{
    SlideItem *si = NULL;
    if (n < 0) // default if called without parameters
    {
        si = slideModel->getSelectedItem();
        if (si)
            n = si->childNumber();
        else
            return false;
    }
    else
        si = slideModel->getSlide(n);
    if (si && n >= 0 && n < slideModel->count() - 1) {
        blockSlideSelection = true;
        slideModel->relinkSlide(si, si->parent(), n + 1);
        blockSlideSelection = false;
        saveState(getSelectString(), QString("moveSlideUp (%1)").arg(n + 1),
                  getSelectString(), QString("moveSlideDown (%1)").arg(n),
                  QString("Move slide %1 down").arg(n));
        return true;
    }
    else
        return false;
}

bool VymModel::moveSlideUp(int n)
{
    SlideItem *si = NULL;
    if (n < 0) // default if called without parameters
    {
        si = slideModel->getSelectedItem();
        if (si)
            n = si->childNumber();
        else
            return false;
    }
    else
        si = slideModel->getSlide(n);
    if (si && n > 0 && n < slideModel->count()) {
        blockSlideSelection = true;
        slideModel->relinkSlide(si, si->parent(), n - 1);
        blockSlideSelection = false;
        saveState(getSelectString(), QString("moveSlideDown (%1)").arg(n - 1),
                  getSelectString(), QString("moveSlideUp (%1)").arg(n),
                  QString("Move slide %1 up").arg(n));
        return true;
    }
    else
        return false;
}

void VymModel::updateSlideSelection(QItemSelection newsel, QItemSelection)
{
    if (blockSlideSelection)
        return;
    QModelIndex ix;
    foreach (ix, newsel.indexes()) {
        SlideItem *si = static_cast<SlideItem *>(ix.internalPointer());
        QString inScript = si->getInScript();

        // show inScript in ScriptEditor
        scriptEditor->setSlideScript(modelID, si->getID(), inScript);

        // Execute inScript
        execute(inScript);
    }
}
