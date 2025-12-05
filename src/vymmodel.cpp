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
#include "branchpropeditor.h"
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
#include "export-impress-filedialog.h"
#include "export-latex.h"
#include "export-markdown.h"
#include "export-orgmode.h"
#include "export-taskjuggler.h"
#include "file.h"
#include "findresultmodel.h"
#include "heading-container.h"
#include "image-container.h"
#include "jira-agent.h"
#include "linkobj.h"
#include "lockedfiledialog.h"
#include "mainwindow.h"
#include "mapdesign.h"
#include "mapeditor.h"
#include "misc.h"
#include "noteeditor.h"
#include "options.h"
#include "scripteditor.h"
#include "slideitem.h"
#include "slidemodel.h"
#include "taskeditor.h"
#include "taskmodel.h"
#include "treeitem.h"
#include "vymmodelwrapper.h"
#include "vymview.h"
#include "warningdialog.h"
#include "xlink.h"
#include "xlinkitem.h"
#include "xlinkobj.h"
#include "xml-freeplane.h"
#include "xml-ithoughts.h"
#include "xml-vym.h"
#include "xmlobj.h"
#include "zip-agent.h"

extern bool debug;
extern bool testmode;
extern bool restoreMode;
extern QStringList ignoredLockedFiles;

extern BranchPropertyEditor *branchPropertyEditor;
extern Main *mainWindow;

extern QDir tmpVymDir;

extern bool useActionLog;
extern QString actionLogPath;

extern NoteEditor *noteEditor;
extern TaskEditor *taskEditor;
extern ScriptEditor *scriptEditor;
extern FlagRowMaster *standardFlagsMaster;
extern FlagRowMaster *userFlagsMaster;

extern Options options;

extern QDir clipboardDir;
extern QString clipboardFileName;

extern ImageIO imageIO;

extern TaskModel *taskModel;

extern QString vymName;
extern QString vymVersion;
extern QDir vymBaseDir;

extern QDir lastImageDir;
extern QDir lastExportDir;

extern Settings settings;
extern QTextStream vout;

extern bool usingDarkTheme;
extern QColor vymBlueColor;

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
    // qDebug() << "Destr VymModel begin this=" << this << "  " << mapName << "zipAgent=" << zipAgent;

    mapEditor = nullptr;
    repositionBlocked = true;

    autosaveTimer->stop();
    fileChangedTimer->stop();
    taskAlarmTimer->stop();

    delete(autosaveTimer);
    delete(fileChangedTimer);
    delete(taskAlarmTimer);

    filePath.clear();

    vymLock.releaseLock();

    // Delete rootItem already now, while VymModel is still around
    // ImageItems can ask VymModel for a path in their destructor then.
    // Delete whole tree, XLinkItems will be queude for deletion in xlinksTrash
    delete rootItem;

    emptyXLinksTrash();

    delete (wrapper);
    delete mapDesignInt;

    // VymModel can be destroyed now, zipProcess for saving is handled in MainWindow

    //qDebug() << "Destr VymModel end this=" << this;
    //logInfo("VymModel destroyed", __func__);
}

void VymModel::clear()
{
    while (rootItem->childCount() > 0) {
        // qDebug()<<"VM::clear  ri="<<rootItem<<"
        // ri->count()="<<rootItem->childCount();
        deleteItem(rootItem->childItemByRow(0));
    }
    reposition();
}

void VymModel::init()
{
    // No MapEditor yet
    mapEditor = nullptr;

    // No ZipAgent yet and not saving
    zipAgent = nullptr;
    isSavingInt = false;
    isLoadingInt = false;

    // Use default author
    authorInt = settings
            .value("/user/name",
                    tr("unknown user", "default name for map author in settings")).toString();
    // MapDesign
    mapDesignInt = new MapDesign;

    // States and IDs
    idLast++;
    modelIdInt = idLast;
    mapChanged = false;
    mapDefault = true;
    mapUnsaved = false;
    buildingUndoScript = false;

    // Selection history
    selModel = nullptr;
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
    dataChangedBlocked = false;

    autosaveTimer = new QTimer(this);
    connect(autosaveTimer, SIGNAL(timeout()), this, SLOT(autosave()));

    fileChangedTimer = new QTimer(this);
    connect(fileChangedTimer, SIGNAL(timeout()), this, SLOT(fileChanged()));
    fileChangedTimer->start(3000);

    taskAlarmTimer = new QTimer(this);
    connect(taskAlarmTimer, SIGNAL(timeout()), this, SLOT(updateTasksAlarm()));
    taskAlarmTimer->start(3000);

    hasContextPos = false;

    hideMode = TreeItem::HideNone;

    // Animation in MapEditor
    zoomFactor = 1;
    mapRotationInt = 0;
    animDuration = 2000;
    animCurve = QEasingCurve::OutQuint;

    // Initialize presentation slides
    slideModel = new SlideModel(this);
    blockSlideSelection = false;

    // Network
    netstate = Offline;

#if defined(VYM_DBUS)
    // Announce myself on DBUS
    new AdaptorModel(this); // Created and not deleted as documented in Qt
    if (!QDBusConnection::sessionBus().registerObject(
            QString("/vymmodel_%1").arg(modelIdInt), this))
        qWarning("VymModel: Couldn't register DBUS object!");
#endif
}

void VymModel::makeTmpDirectories()
{
    // Create unique temporary directories
    tmpMapDirPath = tmpVymDir.path() + QString("/model-%1").arg(modelIdInt);
    histPath = tmpMapDirPath + "/history";
    QDir d;
    if (!d.mkdir(tmpMapDirPath))
        qWarning() << "Couldn't create tmpMapDir=" << tmpMapDirPath;

    QString s = tmpMapDirPath + "/zipDir";
    if (!d.mkpath(s))
        qWarning() << "Couldn't create zipDirInt=" << s;

    zipDirInt.setPath(s);
}

QString VymModel::tmpDirPath() { return tmpMapDirPath; }

QString VymModel::zipDirPath() { return zipDirInt.path(); }

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
                            bool writeMapAttr,
                            bool writeMapDesign,
                            bool writeCompleteTree,
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
    QString header =
        "<?xml version=\"1.0\" encoding=\"utf-8\"?><!DOCTYPE vymmap>\n";

    QString mapAttr = xml.attribute("version", vymVersion);
    // Current map version after load still might be original one, change it now.
    mapVersionInt = vymVersion;

    if (writeMapAttr) {
        mapAttr += xml.attribute("date", toS(QDate::currentDate())) + "\n";

        if (!authorInt.isEmpty())
            mapAttr += xml.attribute("author", authorInt) + "\n";
        if (!titleInt.isEmpty())
            mapAttr += xml.attribute("title", titleInt) + "\n";
        if (!commentInt.isEmpty())
            mapAttr += xml.attribute("comment", commentInt) + "\n";

        mapAttr += xml.attribute("branchCount", QString().number(branchCount()));
        if (mapEditor) {
        mapAttr += xml.attribute("mapZoomFactor",
                     QString().setNum(mapEditor->zoomFactorTarget()));
        mapAttr += xml.attribute("mapRotation",
                     QString().setNum(mapEditor->rotationTarget()));
        }
    }
    header += xml.beginElement("vymmap", mapAttr);

    QString design;

    if (writeMapDesign)
        design = mapDesignInt->saveToDir(tmpdir, prefix);

    xml.incIndent();

    // Find the used flags while traversing the tree
    resetUsedFlags();

    // Temporary list of links
    QList<XLink *> tmpXLinks;

    QString tree;
    // Build xml recursivly
    if (writeCompleteTree) {
        // Save all mapcenters as complete map, if saveSel not set
        tree += saveTreeToDir(tmpdir, prefix, offset, tmpXLinks);

        // Save local settings
        tree += settings.getDataXML(destPath);

        // Save selection
        if (getSelectedItems().count() > 0 && !saveSel)
            tree += xml.valueElement("select", getSelectString());
    } else
    {
        if (saveSel) {
            switch (saveSel->getType()) {
                case TreeItem::Branch:
                case TreeItem::MapCenter:
                    // Save Subtree
                    tree += ((BranchItem *)saveSel)
                                ->saveToDir(tmpdir, prefix, offset, tmpXLinks, exportBoundingBoxes);
                    break;
                case TreeItem::Image:
                    tree += ((ImageItem *)saveSel)->saveToDir(tmpdir);
                    break;
                case TreeItem::XLinkItemType:
                    tree += ((XLinkItem *)saveSel)->getXLink()->saveToDir();
                    break;
                default:
                    // other types shouldn't be saved directly...
                    break;
            }
        }
    }

    QString flags;

    // Write images and definitions of used user flags
    standardFlagsMaster->saveDataToDir(tmpdir + "/flags/standard/", FlagRowMaster::UsedFlags);
    if (flagMode != FlagRowMaster::NoFlags) {
        // First find out, which flags are used
        // Definitions
        flags += userFlagsMaster->saveDef(flagMode);

        userFlagsMaster->saveDataToDir(tmpdir + "/flags/user/", flagMode);
    }

    QString footer;
    // Save XLinks
    for (int i = 0; i < tmpXLinks.count(); ++i)
        footer += tmpXLinks.at(i)->saveToDir();

    if (writeCompleteTree)
        // Save slides
        footer += slideModel->saveToDir();

    xml.decIndent();
    footer += xml.endElement("vymmap");

    return header + design + flags + tree + footer;
}

QString VymModel::saveTreeToDir(const QString &tmpdir, const QString &prefix,
                                const QPointF &offset, QList<XLink *> &tmpXLinks)
{
    QString s;
    for (int i = 0; i < rootItem->branchCount(); i++)
        s += rootItem->getBranchNum(i)->saveToDir(
                tmpdir,
                prefix,
                offset,
                tmpXLinks,
                exportBoundingBoxes);
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
        fileDir = filePath.left(filePath.lastIndexOf("/"));

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

bool VymModel::loadMap(QString fname, const File::LoadMode &lmode,
                                  const File::FileType &ftype,
                                  const int &contentFilter,
                                  BranchItem *insertBranch,
                                  int insertPos)
{
    bool noError = true;

    // Get updated zoomFactor, before applying one read from file in the end
    if (mapEditor) {
        zoomFactor = mapEditor->zoomFactorTarget();
        mapRotationInt = mapEditor->rotationTarget();
    }

    BaseReader *reader;
    fileType = ftype;
    switch (fileType) {
        case File::VymMap:
            reader = new VymReader(this);
            // For imports we might want to ignore slides
            reader->setContentFilter(contentFilter);// FIXME-4 Maybe ignore slides hardcoded, when parsing and remove contentFilter?
            break;
        case File::FreemindMap:
            reader = new FreeplaneReader(this);
            break;
        case File::IThoughtsMap:
            reader = new IThoughtsReader(this);
            break;
        default:
            QMessageBox::critical(0, tr("Critical Parse Error"),
                                  "Unknown FileType in VymModel::load()");
            return false;
    }

    if (lmode == File::NewMap) {
        // Reset timestamp to check for later updates of file
        fileChangedTime = QFileInfo(destPath).lastModified();

        selModel->clearSelection();
    }

    bool zipped_org = zipped;

    // Create temporary directory for unzip
    bool ok;
    QString tmpUnzipDir = makeTmpDir(ok, tmpDirPath(), "unzip");
    if (!ok) {
        QMessageBox::critical(
            0, tr("Critical Load Error"),
            tr("Couldn't create temporary directory before load\n"));
        return false;
    }

    // No returns now befor end of function
    isLoadingInt = true;

    QString xmlfile;
    if (fname.right(4) == ".xml" || fname.right(3) == ".mm") {
        xmlfile = fname;
        zipped = false;

        if (lmode == File::NewMap || lmode == File::DefaultMap)
            zipped_org = false;
    } else {
        // Try to unzip file
        QFile file(fname);
        if (file.size() > 2000000)
            // Inform user that unzipping might take a while.
            // Detailed estimation about number of branches required for progress bar 
            // is within the zipfile and cannot be used yet.
            mainWindow->statusMessage(tr("Uncompressing %1").arg(fname));

        ZipAgent zipAgent(tmpUnzipDir, fname);
        zipAgent.setBackgroundProcess(false);
        zipAgent.startUnzip();
        if (zipAgent.exitStatus() != QProcess::NormalExit)
            noError = false;

        if (file.size() > 2000000)
            // Inform user that unzipping might take a while.
            // Detailed estimation about number of branches required for progress bar 
            // is within the zipfile and cannot be used yet.
            mainWindow->statusMessage(tr("Loading %1").arg(fname), 0);
    }

    if (zipped) {
        QStringList filters;
        filters << "*.xml";
        QStringList xmlFileList = QDir(tmpUnzipDir).entryList(filters);
        if (xmlFileList.count() == 1) {
            // Only one xml file in zip archive, take this one
            xmlfile = tmpUnzipDir + "/" + xmlFileList.first();
        } else {
            // Multiple xml files - which one to choose?
            // Sometimes (at least on Windows) a zipped (!) $MAPNAME.xml also ends up in archive,
            // which prevented subsequent loading
            QString warning = QString("Found multiple .xml files in %1: %2").arg(fname, xmlFileList.join(", "));
            logWarning(warning, __func__);
            QMessageBox::warning (0, "Multiple xml files found", warning + "\n\nWill try to keep only map.xml");

            // mainWindow->fileLoadFromTmp (xmlFileList);
            // returnCode = 1;	// Silently forget this attempt to load
            if (fileType == File::VymMap) {
                // Default map name
                QString xfile = "map.xml";
                if (xmlFileList.contains(xfile))
                    xmlfile = tmpUnzipDir + "/" + xfile;
                else {
                    // Try $MAPNAME.xml
                    xfile = basename(fname);
                    xfile = tmpUnzipDir + "/" + xfile.left(xfile.lastIndexOf(".", -1, Qt::CaseSensitive)) + ".xml";
                    QFile mfile(xfile);
                    if (!mfile.exists()) {
                        // $MAPNAME.xml does not exist, well, ...
                        QMessageBox::critical(
                            0, tr("Critical Load Error"), "Multiple maps found, but no map.xml or " + xfile);
                        noError = false;
                    }
                }
            } else if (fileType == File::IThoughtsMap) {
                if (!xmlFileList.contains("mapdata.xml")) {
                    QMessageBox::critical(
                        0, tr("Critical Load Error"),
                        tr("Couldn't find %1 in map file.\n").arg("mapdata.xml"));
                    noError = false;
                }
                else
                    xmlfile = tmpUnzipDir + "/mapdata.xml";
            } else {
                QMessageBox::critical(
                    0, tr("Critical Load Error"), "Multiple maps found in zip archive and no clue about file type");
                noError = false;
            }
        }
        if (xmlFileList.isEmpty()) {
            QMessageBox::critical(
                0, tr("Critical Load Error"),
                tr("Couldn't find a map (*.xml) in .vym archive.\n"));
            noError = false;
        }
    } // zipped

    QFile file(xmlfile);

    // I am paranoid: file should exist anyway
    // according to check in mainwindow.
    if (!file.exists()) {
        QMessageBox::critical(
            0, tr("Critical Parse Error"),
            tr(QString("Couldn't open map \"%1\"").arg(file.fileName()).toUtf8()));
        noError = false;
    }
    else {
        bool saveStateBlockedOrg = saveStateBlocked;
        repositionBlocked = true;
        saveStateBlocked = true;
        if (mapEditor)
            mapEditor->setViewportUpdateMode(QGraphicsView::NoViewportUpdate);

        // We need to set the tmpDir in order  to load files with rel. path
        QString tmpdir;
        if (zipped)
            tmpdir = tmpUnzipDir;
        else
            tmpdir = fname.left(fname.lastIndexOf("/", -1));

        reader->setTmpDir(tmpdir);

        reader->setLoadMode(lmode);

        if (lmode == File::ImportReplace || lmode == File::ImportAdd) 
            reader->setInsertBranch(insertBranch);

        if (lmode == File::ImportAdd)
            reader->setInsertPos(insertPos);

        bool parsedWell = false;

        // Open file
        if (!file.open(QFile::ReadOnly | QFile::Text)) {
            QMessageBox::critical(nullptr, "VymModel::loadMap",
                    QString("Cannot read file %1:\n%2.")
                    .arg(QDir::toNativeSeparators(fileName),
                        file.errorString()));
            noError = false;
        } else {
            // Here we actually parse the XML file
            parsedWell = reader->read(&file);

            file.close();
        }

        // Aftermath
        repositionBlocked = false;
        saveStateBlocked = saveStateBlockedOrg;
        if (mapEditor)
            mapEditor->setViewportUpdateMode(QGraphicsView::MinimalViewportUpdate);

        if (noError) {
            if (parsedWell) {
                reposition();

                if (lmode == File::NewMap)  // no lockfile for default map!
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

                // Log
                logInfo(QString("Map loaded successfully from \"%1\"").arg(fname), __func__);
            } else {
                QMessageBox::critical(0, tr("Critical Parse Error"),
                                        reader->errorString());
                // returnCode=1;
                // Still return "success": the map maybe at least
                // partially read by the parser

                setReadOnly(true);
            }
        } // noError so far
    }

    // If required, fix positions when importing from old versions
    if (fileType == File::VymMap && versionLowerOrEqual(mapVersionInt, "2.9.500")) {
        foreach (BranchItem *center, rootItem->getBranches()) {
            foreach (BranchItem *mainBranch, center->getBranches()) {
                BranchContainer *bc = mainBranch->getBranchContainer();
                QRectF rb = bc->ornamentsSceneRect();
                QPointF offset;
                offset.setX(rb.width() / 2);
                offset.setY(rb.height() / 2);
                bc->setPos(bc->x() + offset.x(), bc->y() + offset.y());
                logInfo("Adjusting legacy position of " + mainBranch->headingPlain() + "  offset: " + toS(offset), __func__);
            }
        }
        reposition();
    }

    // Cleanup
    removeDir(QDir(tmpUnzipDir));
    delete reader;

    // Restore original zip state
    zipped = zipped_org;

    updateActions();

    if (lmode != File::NewMap)
        emitUpdateQueries();

    if (mapEditor) {
        mapEditor->setZoomFactorTarget(zoomFactor);
        mapEditor->setRotationTarget(mapRotationInt);
    }

    qApp->processEvents(); // Update view (scene()->update() is not enough)

    isLoadingInt = false;
    return noError;
}

bool VymModel::saveMap(const File::SaveMode &savemode)
{
    // Block closing the map while saving, esp. while zipping
    isSavingInt = true;

    QString mapFileName;
    QString saveFilePath;

    bool noError = true;

    if (zipped)
        // save as .xml
        mapFileName = mapName + ".xml";
    else
        // use name given by user, could be anything
        mapFileName = fileName;

    // Look, if we should zip the data:
    if (!zipped)
    {
        QMessageBox mb(QMessageBox::Warning, vymName,
           tr("The map %1\ndid not use the compressed "
              "vym file format.\nWriting it uncompressed will also "
              "write images \n"
              "and flags and thus may overwrite files into the "
              "given directory\n\nDo you want to write the map")
               .arg(filePath));
        QPushButton *compressedButton = mb.addButton(
            tr("compressed (vym default)"),
            QMessageBox::AcceptRole);
        QPushButton *uncompressedButton = mb.addButton(
            tr("uncompressed, potentially overwrite existing data"),
            QMessageBox::NoRole);
        mb.addButton(
            tr("Cancel"),
            QMessageBox::RejectRole);
        mb.exec();
        if (mb.clickedButton() == compressedButton)
            // save compressed (default file format)
            zipped = true;
        else if (mb.clickedButton() == uncompressedButton)
            zipped = false; // FIXME-5 Filename suffix still could be .xml instead of .vym
        else  {
            // do nothing
            isSavingInt = false;
            return false; 
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
        if (!zipDirInt.exists()) {
            QMessageBox::critical(
                0, tr("Critical Save Error"),
                tr("Couldn't access zipDir %1\n").arg(zipDirInt.path()));
            isSavingInt = false;
            return false;
        }

        saveFilePath = filePath;
        setFilePath(zipDirInt.path() + "/" + mapName + ".xml", saveFilePath);
    } // zipped

    // Notification, that we start to save
    mainWindow->statusMessage(tr("Saving  %1...").arg(saveFilePath));

    // Create mapName and fileDir
    makeSubDirs(fileDir);

    // Real saving (not exporting) needs to save also temporary hidden parts
    TreeItem::HideTmpMode hideModeOrg = hideMode;
    setHideTmpMode(TreeItem::HideNone);

    QString mapStringData;
    if (savemode == File::CompleteMap || selModel->selection().isEmpty()) {
        // Save complete map    // FIXME-3 prefix still needed? all treeItems from all models have unique number in filename already...
        if (zipped)
            // Use defined name for map within zipfile to avoid problems
            // with zip library and umlauts (see #98)
            mapStringData =
                saveToDir(fileDir, "", FlagRowMaster::UsedFlags, QPointF());
        else
            mapStringData = saveToDir(fileDir, mapName + "-", FlagRowMaster::UsedFlags, QPointF());
        mapChanged = false;
        mapUnsaved = false;
        autosaveTimer->stop();
    }
    else {
        // Save part of map
        if (selectionType() == TreeItem::Image)
            saveImage();
        else
            mapStringData = saveToDir(fileDir, mapName + "-", FlagRowMaster::UsedFlags,
                                 QPointF(), false, false, false, getSelectedBranch());
        // FIXME-3 take care of multiselections when saving parts
    }

    // Restore original mode
    setHideTmpMode(hideModeOrg);

    QString saveFileName;
    if (zipped)
        // Use defined map name "map.xml", if zipped. Introduce in 2.6.6
        saveFileName = fileDir + "/map.xml";
    else
        // Use regular mapName, when saved as XML
        saveFileName = fileDir + "/" + mapFileName;

    if (!saveStringToDisk(saveFileName, mapStringData)) {
        qWarning("ME::saveStringToDisk failed!");
        noError = false;
    }

    if (!noError)
        mainWindow->statusMessage(tr("Couldn't save ").arg(saveFilePath));
    else {
        if (useActionLog) {
            QString log = QString("Wrote unzipped map \"%1\" into zipDirInt = %2")
                .arg(mapFileName, zipDirInt.path());
            logInfo(log, __func__);
        }

        if (zipped) {
            // zip
            mainWindow->statusMessage(tr("Compressing %1").arg(destPath), 0);

            zipAgent = new ZipAgent(zipDirInt, destPath);
            connect(zipAgent, SIGNAL(zipFinished()), this, SLOT(zipFinished()));
            QString log = QString("Starting zipAgent to compress \"%1\" in zipDirInt = %2")
                .arg(mapFileName, zipDirInt.path());
            logInfo(log, __func__);
            zipAgent->startZip();
        } else
            mainWindow->statusMessage(tr("Saved %1").arg(saveFilePath));

        logInfo("Finishing saving map " + destPath, __func__);  // FIXME-3 debugging
        // Restore original filepath outside of tmp zip dir
        setFilePath(saveFilePath);
    }

    updateActions();

    if (!zipped)
        isSavingInt = false;

    return noError;
}

bool VymModel::isSaving()
{
    return isSavingInt;
}

bool VymModel::isLoading()
{
    return isLoadingInt;
}

bool VymModel::isBusy()
{
    return isLoadingInt || isSavingInt;
}

void VymModel::zipFinished()
{
    // Cleanup
    QString path = "unknown";
    QString name = "unknown";
    if (zipAgent) {
        path = zipAgent->zipDir().path();
        name = zipAgent->zipName();

        QString log = QString("Finished zipping %1 to %2").arg(path, name);
        logInfo(log, __func__);

        zipAgent->deleteLater();
        zipAgent = nullptr;
    } else
        logWarning("zipAgent == nullptr", __func__);

    isSavingInt = false;

    mainWindow->statusMessage(tr("Saved %1").arg(filePath));

    fileChangedTime = QFileInfo(destPath).lastModified();

    updateActions();

}

ImageItem* VymModel::loadImage(BranchItem *parentBranch, const QStringList &imagePaths)
{
    if (!parentBranch)
        parentBranch = getSelectedBranch();

    if (parentBranch) {
        if (!imagePaths.isEmpty()) {
	    ImageItem *ii = nullptr;

            lastImageDir.setPath(
                imagePaths.first().left(imagePaths.first().lastIndexOf("/")));

            QString s;
            for (int j = 0; j < imagePaths.count(); j++) {
                s = imagePaths.at(j);

                ii = createImage(parentBranch);

                QString bv = setBranchVar(parentBranch);
                QString uc = setImageVar(ii) + "map.removeImage(i);";
                QString rc = bv + "b.loadBranchInsert(\"REDO_PATH\", 0);";
                QString comment = QString("Load image %1").arg(s);

                logAction(rc, comment, __func__);



                if (ii && ii->load(s)) {
                    
                    ImageContainer *ic = ii->getImageContainer();
                    QPointF pos_new = parentBranch->getBranchContainer()->getPositionHintNewChild(ic);
                    ic->setPos(pos_new);

                    saveState( uc, rc, comment, nullptr, ii);
                }
                else {
                    logWarning("Failed: " + comment, __func__);
                    deleteItem(ii);
                    return nullptr;
                }
            }

	    reposition();
	    return ii;	// When pasting we need the last added image for scaling
        }
    }
    return nullptr;
}

ImageItem* VymModel::loadImage(BranchItem *parentBranch, const QString &imagePath)
{
    QStringList imagePaths;
    imagePaths << imagePath;
    return loadImage(parentBranch, imagePaths);
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
                nullptr, vymName + " - " + tr("Save image"), lastImageDir.path(),
                filter, nullptr, QFileDialog::DontConfirmOverwrite);

        if (!fn.isEmpty()) {
            lastImageDir.setPath(fn.left(fn.lastIndexOf("/")));
            if (QFile(fn).exists()) {
                QMessageBox mb(
                   QMessageBox::Warning,
                   vymName,
                   tr("The file %1 exists already.\n"
                      "Do you want to overwrite it?")
                       .arg(fn));
                mb.addButton(
                    tr("Overwrite"),
                    QMessageBox::AcceptRole);
                mb.addButton(
                    tr("Cancel"),
                    QMessageBox::RejectRole);
                mb.exec();
                if (mb.result() != QMessageBox::AcceptRole)
                    return;
            }
            if (!ii->saveImage(fn))
                QMessageBox::critical(0, tr("Critical Error"),
                                      tr("Couldn't save %1").arg(fn));
            else
                mainWindow->statusMessage(tr("Saved %1").arg(fn));
        }
    }
}

void VymModel::importDirInt(QDir d, BranchItem *dst)
{
    bool oldSaveState = saveStateBlocked;
    saveStateBlocked = true;
    BranchItem *bi = dst;
    if (bi) {
        int beginDepth = bi->depth();

        d.setFilter(QDir::AllEntries | QDir::Hidden);
        QFileInfoList list = d.entryInfoList();
        QFileInfo fi;

        QColor dirColor;
        QColor fileColor;
        if (usingDarkTheme) {
            dirColor = QColor(0, 170, 255); // #00aaff
            fileColor = QColor(255, 255, 255);
        } else {
            dirColor = QColor(0, 0, 255);
            fileColor = QColor(0, 0, 0);
        }

        // Traverse directories
        for (int i = 0; i < list.size(); ++i) {
            fi = list.at(i);
            if (fi.isDir() && fi.fileName() != "." && fi.fileName() != "..") {
                bi = addNewBranchInt(dst, -2);
                bi->setHeadingPlainText(fi.fileName());
                bi->setHeadingColor(dirColor);
                if (debug)
                    qDebug() << "Added subdir: " << fi.fileName();
                if (!d.cd(fi.fileName()))
                    QMessageBox::critical(
                        0, tr("Critical Import Error"),
                        tr("Cannot find the directory %1").arg(fi.fileName()));
                else {
                    // Recursively add subdirs
                    importDirInt(d, bi);
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
                bi->setHeadingColor(fileColor);
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

void VymModel::importDir(const QString &dirPath, BranchItem *bi)
{
    BranchItem *selbi = getSelectedBranch(bi);
    if (selbi) {
        QString bv = setBranchVar(selbi);
        QString uc = bv + QString("map.loadMapReplace(\"UNDO_PATH\", b);");
        QString rc = bv + QString("b.importDir(\"%1\");").arg(dirPath);
        QString comment = QString("Import directory structure from \"%1\" to branch \"%2\"").arg(dirPath, selbi->headingText());

        logAction(rc, comment, __func__);

        saveState(uc, rc, comment, selbi);

        QDir d(dirPath);
        importDirInt(d, selbi);
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
        fd.setFileMode(QFileDialog::Directory);
        fd.setNameFilters(filters);
        fd.setWindowTitle(vymName + " - " +
                          tr("Choose directory structure to import"));
        fd.setAcceptMode(QFileDialog::AcceptOpen);

        if (fd.exec() == QDialog::Accepted && !fd.selectedFiles().isEmpty()) {
            importDir(fd.selectedFiles().constFirst());
            reposition();
        }
    }
}

bool VymModel::addMapInsert(QString fpath, int insertPos, BranchItem *bi)
{
    BranchItem *selbi = getSelectedBranch(bi);
    if (!selbi) {
        logWarning("Failed: No branch provided");
        return false;
    }

    // Only saveState if a branch is inserted
    // Other data like XLink is only used for undo/redo operations and currently
    // does not need a saveState
    QString bv = setBranchVar(selbi);
    QString uc = bv + QString("map.loadBranchReplace(\"UNDO_PATH\", b);");
    QString rc = bv + QString("b.loadBranchInsert(\"%1\", %2);").arg(fpath).arg(insertPos);
    QString comment = QString("Add map %1 to \"%2\"").arg(fpath, selbi->headingText());

    logAction(rc, comment, __func__);

    saveState(uc, rc, comment, selbi);

    if (loadMap(fpath, File::ImportAdd, File::VymMap, 0x0000, selbi, insertPos))
        return true;

    logWarning("Failed: Loading from " + fpath, __func__);
    return false;
}

bool VymModel::addMapReplace(QString fpath, BranchItem *bi)
{
    BranchItem *selbi = getSelectedBranch(bi);
    if (!selbi) {
        logWarning("Failed: No branch provided");
        return false;
    }

    QString bv = setBranchVar(selbi);
    QString pbv = setBranchVar(selbi->parentBranch(), "pb");
    QString uc = pbv + QString("map.loadBranchReplace(\"UNDO_PATH\", pb);");
    QString rc = bv + QString("map.loadBranchReplace(\"REDO_PATH\", b);");
    QString comment = QString("Replace \"%1\" with \"%2\"").arg(selbi->headingText(), fpath);

    logAction(rc, comment, __func__);

    saveState(uc, rc, comment, selbi->parentBranch(), selbi);

    if (loadMap(fpath, File::ImportReplace, File::VymMap, 0x0000, selbi))
        return true;

    logWarning("Failed: " + comment, __func__);
    return false;
}

bool VymModel::removeVymLock()
{
    if (vymLock.removeLockForced()) {
        mainWindow->statusMessage(tr("Removed lockfile for %1").arg(mapName));
        setReadOnly(false);
        return true;
    } else
        return false;
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
        if (vymLock.getState() == VymLock::LockedByOther) {
            if (restoreMode) {
                // While restoring maps, existing lockfiles will be ignored for
                // loading, but listed in a warning dialog
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
                        .arg(a, h);
                dia.setText(s);
                dia.setWindowTitle(
                    tr("Warning: Map already opended", "VymModel"));
                if (dia.execDialog() == LockedFileDialog::DeleteLockfile) {
                    if (!removeVymLock()) {
                        // Could not remove existing lockfile, give up
                        QMessageBox::warning(
                            0, tr("Warning"),
                            tr("Couldn't remove lockfile for %1").arg(mapName));
                        return false;
                    }
                    if (!tryVymLock()) {
                        // Was able to remove existing lockfile, but not able to 
                        // create new one.
                        qWarning() << "VymModel::tryVymLock could not create new lockfile after removing old";
                        return false;
                    }
                }
            }
        }
        else if (vymLock.getState() == VymLock::NotWritable) {
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
// map is renamed before fileSaveAs() or from VymModelWrapper::saveSelection()
// Usually renamed back to original name again. Purpose here is to adapt the lockfile 
// new name of map.
// Internally the paths in ImageItems pointing to zipDirInt do not need to be adapted.
{
    QString oldPath = filePath;
    if (vymLock.getState() == VymLock::LockedByMyself || vymLock.getState() == VymLock::Undefined) {
        // vymModel owns the lockfile, try to create new lock
        VymLock newLock;
        newLock = vymLock;
        newLock.setMapPath(newPath);    // Resets state for newLock to "Undefined"
        if (!newLock.tryLock()) {
            logWarning(QString("Failed to create lock for %1").arg(newPath), __func__);
            return false;
        }

        // Change lockfiles now
        if (!vymLock.releaseLock())
            logWarning(QString("Failed to release lock for %1").arg(oldPath), __func__);
        vymLock = newLock;
        setFilePath(newPath);
        if (readonly)
            setReadOnly(false);
        return true;
    }
    logWarning("Failed to rename map.", __func__);
    return false;
}

void VymModel::setReadOnly(bool b)
{
    readonly = b;
    mainWindow->updateTabName(this);
}

bool VymModel::isReadOnly() { return readonly; }

void VymModel::autosave()
{
    // Check if autosave is disabled due to testmode or current zip process
    if (testmode)
        return;

    // Check if autosave is disabled globally
    if (!mainWindow->useAutosave()) {
        // qWarning() << QString("VymModel::autosave disabled globally!  Current map: %1").arg(filePath);
        return;
    }

    if (zipAgent || isSavingInt) {
        //qDebug() << "VymModel::autosave blocked by zipAgent or ongoing save";
        return;
    }

    // Disable autosave, while we have gone back in history
    int redosAvail = undoSet.numValue(QString("/history/redosAvail"));
    if (redosAvail > 0)
        return;

    // Also disable autosave for new map without filename
    if (filePath.isEmpty()) {
        /*
        if (debug)
            qWarning() << "VymModel::autosave rejected due to missing filePath\n";
        */
        return;
    }

    if (mapUnsaved && mapChanged && !testmode) {
        if (QFileInfo(filePath).lastModified() <= fileChangedTime) {
            logInfo("Autosave starting", __func__);

            // Call save via MainWindow to check for filename and readonly status
            mainWindow->fileSave(this);
        } else if (debug)
            qDebug() << "  VM::autosave  rejected, file on disk is newer than "
                        "last save.\n";
    }
    logInfo("Autosave finished", __func__); // FIXME-3 remove after debugging
}

void VymModel::fileChanged()
{
    // Check if file on disk has changed meanwhile
    if (!filePath.isEmpty()) {
        if (readonly && vymLock.getState() != VymLock::LockedByMyself) {
            // unset readonly if lockfile is gone
            // but only, if map was LockedByOther before
            if (vymLock.tryLock())
                setReadOnly(false);
        }
        else {
            // FIXME-5 We could check, if somebody else removed/replaced lockfile
            // (A unique vym ID would be needed)

            if (isSavingInt)
                return;

            QDateTime tmod = QFileInfo(filePath).lastModified();
            if (tmod > fileChangedTime) {
                // FIXME-5 VM switch to current mapeditor and finish
                // lineedits...
                QMessageBox mb(
                    QMessageBox::Question,
                    vymName,
                    tr("The file of the map  on disk has changed:\n\n"
                       "   %1\n\nDo you want to reload that map with the new "
                       "file?")
                        .arg(filePath));

                mb.addButton(
                    tr("Reload"),
                    QMessageBox::AcceptRole);
                mb.addButton(
                    tr("Ignore"),
                    QMessageBox::RejectRole);
                mb.exec();
                if (mb.result() == QMessageBox::AcceptRole) {
                    // Reload map
                    mainWindow->initProgressCounter(1);
                    loadMap(filePath);
                    mainWindow->removeProgressCounter();
                } else
                    // allow autosave to overwrite newer file!
                    fileChangedTime = tmod;
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

QString VymModel::getObjectName(TreeItem *ti)   // FIXME-3 compare with headingText - still needed?
{
    QString s;
    if (!ti)
        return QString("Error: nullptr has no name!");
    s = ti->headingPlain();
    if (s == "")
        s = "unnamed";

    return QString("%1 \"%2\"").arg(ti->getTypeName(), s);
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
    QString redoCommand =
        undoSet.value(QString("/history/step-%1/redoCommand").arg(curStep));
    QString comment =
        undoSet.value(QString("/history/step-%1/comment").arg(curStep));

    /* TODO Maybe check for version, if we save the history
    if (!checkVersion(mapVersionInt))
    QMessageBox::warning(0,tr("Warning"),
        tr("Version %1 of saved undo/redo data\ndoes not match current vym
    version %2.").arg(mapVersionInt).arg(vymVersion));
    */

    if (debug) {
        qDebug() << "VymModel::redo() begin\n";
        qDebug() << "    undosAvail=" << undosAvail;
        qDebug() << "    redosAvail=" << redosAvail;
        qDebug() << "       curStep=" << curStep;
        qDebug() << "    ---------------------------";
        qDebug() << "    comment=" << comment;
        qDebug() << "    undoCom:";
        cout << qPrintable(undoCommand) << endl;
        qDebug() << "    redoCom=";
        cout << qPrintable(redoCommand) << endl;
        qDebug() << "    ---------------------------";
    }

    QString errMsg;
    QString redoScript =
        QString("map = vym.currentMap();%1").arg(redoCommand);

    errMsg = mainWindow->runScript(redoScript).toString();

    saveStateBlocked = saveStateBlockedOrg;

    undoSet.setValue("/history/undosAvail", QString::number(undosAvail));
    undoSet.setValue("/history/redosAvail", QString::number(redosAvail));
    undoSet.setValue("/history/curStep", QString::number(curStep));
    undoSet.writeSettings(histPath);

    mainWindow->updateHistory(undoSet);

    updateActions();

    /* TODO remove testing
    qDebug() << "ME::redo() end\n";
    qDebug() << "    undosAvail=" << undosAvail;
    qDebug() << "    redosAvail=" << redosAvail;
    qDebug() << "       curStep=" << curStep;
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
    if (isRedoAvailable())
        return undoSet.value(
            QString("/history/step-%1/redoCommand").arg(curStep + 1));
    else
        return QString();
}

QString VymModel::lastRedoComment()
{
    if (isRedoAvailable())
        return undoSet.value(QString("/history/step-%1/comment").arg(curStep + 1));
    else
        return QString();
}

QString VymModel::lastUndoCommand()
{
    if (isUndoAvailable())
        return undoSet.value(
            QString("/history/step-%1/undoCommand").arg(curStep));
    else
        return QString();
}

QString VymModel::lastUndoComment()
{
    if (isUndoAvailable())
        return undoSet.value(QString("/history/step-%1/comment").arg(curStep));
    else
        return QString();
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
    QString redoCommand =
        undoSet.value(QString("/history/step-%1/redoCommand").arg(curStep));
    QString comment =
        undoSet.value(QString("/history/step-%1/comment").arg(curStep));

    /* TODO Maybe check for version, if we save the history
    if (!checkVersion(mapVersionInt))
    QMessageBox::warning(0,tr("Warning"),
        tr("Version %1 of saved undo/redo data\ndoes not match current vym
    version %2.").arg(mapVersionInt).arg(vymVersion));
    */

    if (debug) {
        qDebug() << "VymModel::undo() begin\n";
        qDebug() << "    undosAvail=" << undosAvail;
        qDebug() << "    redosAvail=" << redosAvail;
        qDebug() << "       curStep=" << curStep;
        cout << "    ---------------------------" << endl;
        qDebug() << "    comment=" << comment;
        cout << "    undoCom:" << endl;
        cout << qPrintable(undoCommand) << endl;
        cout << "    redoCom:" << endl;
        cout << qPrintable(redoCommand) << endl;
        cout << "    ---------------------------" << endl;
    }

    // bool noErr;
    QString errMsg;
    QString undoScript = QString("map = vym.currentMap();%1").arg(undoCommand);

    errMsg = mainWindow->runScript(undoScript).toString();

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
    QString histName(QString("history-%1").arg(dataStep));
    return (tmpMapDirPath + "/" + histName);
}

void VymModel::resetHistory()
{
    curStep = 0;
    redosAvail = 0;
    undosAvail = 0;
    dataStep = 0;

    stepsTotal = settings.value("/history/stepsTotal", 100).toInt();
    undoSet.setValue("/history/stepsTotal", QString::number(stepsTotal));
    mainWindow->updateHistory(undoSet);
}

QString VymModel::setAttributeVar(AttributeItem* ai, QString varName)
{
    // Default varName: "a"
    QString r;
    if (!ai)
        qWarning() << "VM::setAttributeVar ai == nullptr";
    else

        r = QString("%1 = map.findAttributeById(\"%2\");").arg(varName, ai->getUuid().toString());

    return r;
}

QString VymModel::setBranchVar(BranchItem* bi, QString varName)
{
    // Default varName: "b"
    QString r;
    if (!bi)
        qWarning() << "VM::setBranchVar bi == nullptr";
    else
        r = QString("%1 = map.findBranchById(\"%2\");").arg(varName, bi->getUuid().toString());

    return r;
}

QString VymModel::setImageVar(ImageItem* ii, QString varName)
{
    // Default varName: "i"
    QString r;
    if (!ii)
        qWarning() << "VM::setImageVar ii == nullptr";
    else
        r = QString("%1 = map.findImageById(\"%2\");").arg(varName, ii->getUuid().toString());

    return r;
}

QString VymModel::setXLinkVar(XLink* xl, QString varName)
{
    QString r;
    if (!xl)
        qWarning() << "VM::setXLinkVar xl == nullptr";
    else
        r = QString("%1 = map.findXLinkById(\"%2\");").arg(varName, xl->getUuid().toString());

    return r;
}

QString VymModel::saveState(
         QString undoCommand,
         QString redoCommand,
         const QString &comment,
         TreeItem *saveUndoItem,
         TreeItem *saveRedoItem,
         bool createHistoryDir)
{
    // Main saveState

    // sendData(redoCom); // FIXME-5 testing network

    if (saveStateBlocked)
        return QString();

    /*
    if (debug) {
        qDebug() << "VM::saveState() for map " << mapName;
        qDebug() << "  comment: " << comment;
        qDebug() << "  Script:   " << buildingUndoScript;
        qDebug() << "  undoCom: " << undoCommand;
        qDebug() << "  redoCom: " << redoCommand;
    }
    */

    if (buildingUndoScript)
        logInfo("// Building script: " + redoCommand + " " +  comment, __func__);    // FIXME-3 Use logDebug instead? Remove logging completely from saveState?
    else
        logInfo("saveState: " + comment + " " + redoCommand, __func__);

    QString historyPath = getHistoryPath();

    // Create historyPath if not available and required
    if (saveUndoItem || saveRedoItem || createHistoryDir) {
        dataStep++;
        QDir d(historyPath);
        if (!d.exists())
            makeSubDirs(historyPath);   // FIXME-3 Only create subDirs on demand, e.g. when saving imageItem
    }

    // Save depending on how much needs to be saved
    //
    // FIXME-5 saveState: userFlags are not written, but still in memory. Could
    //         lead to problem, if one day removed from userFlags toolbar AND memory
    if (saveUndoItem) {
        QString dataXML = saveToDir(historyPath, mapName + "-", FlagRowMaster::NoFlags, QPointF(),
                            false, false, false, saveUndoItem);

        QString xmlUndoPath = historyPath + "/undo.xml";
        undoCommand.replace("UNDO_PATH", xmlUndoPath);
        saveStringToDisk(xmlUndoPath, dataXML);
    }
    if (saveRedoItem) {
        QString dataXML = saveToDir(historyPath, mapName + "-", FlagRowMaster::NoFlags, QPointF(),
                            false, false, false, saveRedoItem);

        QString xmlRedoPath = historyPath + "/redo.xml";
        redoCommand.replace("REDO_PATH", xmlRedoPath);
        saveStringToDisk(xmlRedoPath, dataXML);
    }

    undoCommand.replace("HISTORY_PATH", historyPath);
    redoCommand.replace("HISTORY_PATH", historyPath);

    if (debug) {
        qDebug() << "  undoCommand: " << undoCommand;
        qDebug() << "  redoCommand: " << redoCommand;
        qDebug() << "      Comment: " << comment;
    }

    if (buildingUndoScript)
    {
        // Build string with all commands
        undoScript = undoCommand + undoScript;
        redoScript = redoScript + redoCommand;

        if (debug) {
            qDebug() << "VM::saveState  building scripts:";
            qDebug() << "  undoScript = " << undoScript;
            qDebug() << "  redoScript = " << redoScript;
        }
        return historyPath + "/";
    }

    if (undosAvail < stepsTotal)
        undosAvail++;

    curStep++;
    if (curStep > stepsTotal)
        curStep = 1;

    // We would have to save all actions in a tree, to keep track of
    // possible redos after an action. Possible, but we are too lazy: forget
    // about redos
    redosAvail = 0;

    // Write the current state to disk
    undoSet.setValue("/history/undosAvail", QString::number(undosAvail));
    undoSet.setValue("/history/redosAvail", QString::number(redosAvail));
    undoSet.setValue("/history/curStep", QString::number(curStep));
    undoSet.setValue(QString("/history/step-%1/undoCommand").arg(curStep),
                     undoCommand);
    undoSet.setValue(QString("/history/step-%1/redoCommand").arg(curStep),
                     redoCommand);
    undoSet.setValue(QString("/history/step-%1/comment").arg(curStep), comment);
    undoSet.writeSettings(histPath);

    /*
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
        cout << "    undoCom:" <<  qPrintable(undoCommand) << "\n";
        cout << "    redoCom:" <<  qPrintable(redoCommand) << "\n";
        cout << "    ---------------------------\n";
    }
    */

    mainWindow->updateHistory(undoSet);

    setChanged();

    return historyPath + "/";
}

QString VymModel::saveStateBranch(
        BranchItem *bi,
        const QString &uc,
        const QString &rc,
        const QString &comment)
{
    QString prefix = setBranchVar(bi) + "b.";

    QString repeatAction = QString("m = vym.currentMap();");
    repeatAction += " branches = m.selectedBranches(); for (b of branches) {b." + rc + "}";
    mainWindow->setRepeatAction(repeatAction);

    return saveState(prefix + uc, prefix + rc, comment);
}

void VymModel::saveStateBeginScript(const QString &comment)
{
    if (buildingUndoScript)
        logWarning("Nested saveState scripts found", __func__);  // FIXME-3 e.g. for setFrameAutoDesign...
    else {
        logDebug("Starting to build saveStateScript: '" + comment + "'", __func__);

        buildingUndoScript = true;
        undoScriptComment = comment;
        undoScript.clear();
        redoScript.clear();
    }
}

void VymModel::saveStateEndScript()
{
    if (debug)
        std::cout << "VM::saveStateEndScript" << endl 
            << "  buildingScript=" << buildingUndoScript << endl
            << "      undoScript=" << undoScript.toStdString() << endl;

    if (buildingUndoScript) {
        buildingUndoScript = false;

        logDebug("Finished building saveStateScript: '" + undoScriptComment + "'", __func__);

        // Drop whole Script, if empty
        if (undoScript.isEmpty() && redoScript.isEmpty()) return;

        saveState(
                QString("{%1}").arg(undoScript),
                QString("{%1}").arg(redoScript),
                undoScriptComment, nullptr);
    }
}

void VymModel::saveStateCancelScript()
{
    if (debug)
        std::cout << "VM::saveStateCancelScript" << endl 
            << "  buildingScript=" << buildingUndoScript << endl
            << "      undoScript=" << undoScript.toStdString() << endl;

    if (buildingUndoScript) {
        buildingUndoScript = false;

        logDebug("Canceling building saveStateScript: '" + undoScriptComment + "'", __func__);

        // Drop whole Script, if empty
        if (undoScript.isEmpty() && redoScript.isEmpty()) return;

        // Prepare undo of actions so far
        saveState(
                QString("{%1}").arg(undoScript),
                QString("{%1}").arg(redoScript),
                undoScriptComment, nullptr);

        // Undo
        undo();
    }
}

QGraphicsScene *VymModel::getScene() { return mapEditor->getScene(); }

TreeItem *VymModel::findBySelectString(QString s)
{
    if (s.isEmpty())
        return nullptr;

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
            return nullptr;
    }
    return ti;
}

TreeItem *VymModel::findID(const uint &id)
{
    BranchItem *cur = nullptr;
    BranchItem *prev = nullptr;
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
        j = 0;
        while (j < cur->attributeCount()) {
            AttributeItem *ai = cur->getAttributeNum(j);
            if (id == ai->getID())
                return ai;
            j++;
        }
        nextBranch(cur, prev);
    }
    return nullptr;
}

TreeItem *VymModel::findUuid(const QUuid &id)
{
    BranchItem *cur = nullptr;
    BranchItem *prev = nullptr;
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

    // Special case: XLinks (not XLinkItems!) are no TreeItem, if Uuid matches,
    // return one of the XLinkItems. Used in VymModelWrapper::findXLinkById
    foreach (auto xl, xlinks)
        if (xl->getUuid() == id)
            return xl->beginXLinkItem();

    // For restoring MapCenters we might want to add to rootItem
    if (rootItem->getUuid() == id)
        return rootItem;

    return nullptr;
}

BranchItem* VymModel::findBranchByAttribute(const QString &key, const QString &value)
{
    BranchItem *cur = nullptr;
    BranchItem *prev = nullptr;
    nextBranch(cur, prev);
    while (cur) {
        AttributeItem *ai = cur->getAttributeByKey(key); 
        if (ai && ai->value().toString() == value)
            return cur;
        nextBranch(cur, prev);
    }

    return nullptr;
}

void VymModel::test()
{
    // Print item structure
    foreach (TreeItem *ti, getSelectedItems()) {
        if (ti->hasTypeBranch()) {
            BranchContainer *bc = ((BranchItem*)ti)->getBranchContainer();
            bc->printStructure();
        }
        if (ti->hasTypeImage())
            ((ImageItem*)ti)->parentBranch()->getBranchContainer()->printStructure();
    }
    return;

    // Do animation step. All BranchContainers
    QList <BranchContainer*> bc_list;

    qDebug() << "Calculating forces...";
    BranchItem *cur = nullptr;
    BranchItem *prev = nullptr;
    nextBranch(cur, prev);
    while (cur) {
        //qDebug() << "Adding branch: " << cur->headingText();
        BranchContainer *bc = cur->getBranchContainer();
        bc->v_anim = QPointF(0,0);
        bc_list << bc;
        nextBranch(cur, prev);
    }

    foreach (BranchContainer *bc, bc_list) {
        HeadingContainer *hc = bc->getHeadingContainer();
        HeadingContainer *ohc;

        // Forces pushing apart
        /*
        */
        foreach (BranchContainer *obc, bc_list) {
            if (bc != obc) {
                
                ohc = obc->getHeadingContainer();

                QPointF vec = hc->mapFromItem(ohc, ohc->pos());
                qreal dx = vec.x();
                qreal dy = vec.y();
                double l = 2.0 * (dx * dx + dy * dy);

                if (l > 25) {
                    bc->v_anim += QPointF(- (dx *150) / l, - (dy * 150) / l);
                    qDebug() << "Push "<< hc->info() << " <- " << ohc->info() << " vec=" << toS(vec) << " l=" << l;
                }
            }
        }

        // Forces pulling together
        BranchItem *bi = bc->getBranchItem();
        /*
        double weight = (bi->branchCount() + 1) * 10;

        for (int i = 0; i < bi->branchCount(); i++) {
            BranchItem *obi = bi->getBranchNum(i);
            BranchContainer *obc = obi->getBranchContainer();
            ohc = obc->getHeadingContainer();

            // Parent pulled by child
            QPointF vec = hc->mapFromItem(ohc, ohc->pos());
            bc->v_anim += QPointF( vec.x() / weight, vec.y() / weight);
            qDebug() << "  Child Pull  from " << obi->headingText() << " to " << bi->headingText() << toS(vec);

            // Child pulled by parent
            vec = ohc->mapFromItem(hc, ohc->pos());
            obc->v_anim += QPointF( vec.x() / weight, vec.y() / weight);
            qDebug() << "  Parent Pull from " << bi->headingText() << " to " << obi->headingText() << toS(vec);
        }
        */

        // Move MapCenters towards center
        if (bi->depth() == 0) {
            QPointF vec = hc->mapToScene(QPointF(0,0));
            qreal dx = vec.x();
            qreal dy = vec.y();
            double l = sqrt( dx * dx + dy * dy);
            if (l > 5) {
                bc->v_anim += QPointF(- (dx ) / l, - (dy ) / l);
                qDebug() << "Moving to center: " << bc->info() << "l=" << l;
            }
        }

        // Ignore too small vector
        if (qAbs(bc->v_anim.x()) < 0.1 && qAbs(bc->v_anim.y()) < 0.1)
            bc->v_anim = QPointF(0, 0);
    }

    foreach (BranchContainer *bc, bc_list) {
        // Show vector
        bc->v.setLine(0, 0, bc->v_anim.x() * 10, bc->v_anim.y() * 10);

        // Now actually move items
        bc->setPos( bc->pos() + bc->v_anim);
    }

    reposition();
    return;


    //mapEditor->testFunction1();
    //return;

    // Read bookmarks
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(nullptr, "QXmlStream Bookmarks",
                QString("Cannot read file %1:\n%2.")
                .arg(QDir::toNativeSeparators(fileName),
                    file.errorString()));
        return;
    }

    VymReader reader(this);
    if (!reader.read(&file)) {
        QMessageBox::warning(nullptr, QString("QXmlStream Bookmarks"),
                QString("Parse error in file %1:\n\n%2")
                .arg(QDir::toNativeSeparators(fileName),
                    reader.errorString()));
    } else {
        mainWindow->statusMessage("File loaded");
        reposition();
    }

}

//////////////////////////////////////////////
// Interface
//////////////////////////////////////////////
    
void VymModel::setMapTitle(const QString &s)
{
    if (titleInt != s) {
        QString uc = QString("map.setTitle (\"%1\");").arg(titleInt);
        QString rc = QString("map.setTitle (\"%1\");").arg(s);
        QString comment = QString("Set title of map to \"%1\"").arg(s);

        logAction(rc, comment, __func__);

        saveState(uc, rc, comment);
        titleInt = s;
    }
}

QString VymModel::mapTitle() { return titleInt; }

void VymModel::setMapAuthor(const QString &s)
{
    if (authorInt != s) {
        QString uc = QString("map.setAuthor (\"%1\");").arg(authorInt);
        QString rc = QString("map.setAuthor (\"%1\");").arg(s);
        QString comment = QString("Set author of map to \"%1\"").arg(s);

        logAction(rc, comment, __func__);

        saveState(uc, rc, comment);

        authorInt = s;
    }
}

QString VymModel::mapAuthor() { return authorInt; }

void VymModel::setMapComment(const QString &s)
{
    if (commentInt != s) {
        QString uc = QString("map.setComment (\"%1\");").arg(commentInt);
        QString rc = QString("map.setComment (\"%1\");").arg(s);
        QString c = QString("Set comment of map to \"%1\"").arg(s);

        logAction(rc, c, __func__);

        saveState(uc, rc, c);

        commentInt = s;
    }
}

QString VymModel::mapComment() { return commentInt; }

void VymModel::setMapVersion(const QString &s)
{
    // Version stored in file
    mapVersionInt = s;
}

QString VymModel::mapVersion()
{
    return mapVersionInt;
}

int VymModel::branchCount()
{
    int c = 0;
    BranchItem *cur = nullptr;
    BranchItem *prev = nullptr;
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
    emit sortFilterChanged(sortFilter);
}

QString VymModel::getSortFilter() { return sortFilter; }

void VymModel::setHeading(const VymText &vt, TreeItem *ti)
{
    Heading h_old;
    Heading h_new;
    h_new = vt;
    QString s = vt.getTextASCII();

    TreeItem *selti = getSelectedItem(ti);

    if (selti && selti->hasTypeBranchOrImage()) {
        h_old = selti->heading();
        if (h_old == h_new)
            return;

        QString tiv;    // ti variable in script
        if (selti->hasTypeBranch())
            tiv = setBranchVar((BranchItem*)selti) + "b.";
        else
            tiv = setImageVar((ImageItem*)selti) + "i.";

        QString uc, rc;
        if (h_old.isRichText())
            uc = QString("%1setHeadingRichText(\"%2\");").arg(tiv, quoteQuotes(h_old.getText()));
        else
            uc = QString("%1setHeadingText(\"%2\");").arg(tiv, quoteQuotes(h_old.getText()));
        if (h_new.isRichText())
            rc = QString("%1setHeadingRichText(\"%2\");").arg(tiv, quoteQuotes(h_new.getText()));
        else
            rc = QString("%1setHeadingText(\"%2\");").arg(tiv, quoteQuotes(h_new.getText()));

        QString comment = QString("Set heading of %1 to \"%2\"").arg(getObjectName(selti), s);

        logAction(rc, comment, __func__);

        saveState( uc, rc, comment);

        // After adding branches or MapCenters interactively we might want to end an undo script
        saveStateEndScript();

        selti->setHeading(vt);
        emitDataChanged(selti);
        emitUpdateQueries();
        mainWindow->updateHeadingEditor(selti);    // Update HeadingEditor with new heading (if required)
        reposition();
    } else
        qWarning() << "VM::setHeading has no branch or image selected!";
}

void VymModel::setHeadingPlainText(const QString &s, TreeItem *ti)
{
    TreeItem *selti = getSelectedItem(ti);
    if (selti && selti->hasTypeBranchOrImage()) {
        VymText vt = selti->heading();
        vt.setPlainText(s);
        if (selti->heading() == vt)
            return;
        setHeading(vt, selti);

        // Set URL
        if (selti->hasTypeBranch()) {
            BranchItem *bi = (BranchItem*)selti;

            if ((s.startsWith("http://") || s.startsWith("https://")) && !bi->hasUrl())
                    setUrl(s, true, bi);
        }
    }
}

QString VymModel::headingText(TreeItem *ti)
{
    // Mainly used for debugging, also works with nullptr
    if (ti)
        return ti->headingPlain();
    else
        return QString("No treeItem available");
}

void VymModel::updateNoteText(const VymText &vt)
{
    VymNote note_new(vt);
    setNote(note_new, nullptr, true);
}

void VymModel::setNote(const VymNote &note_new, BranchItem *bi, bool senderIsNoteEditor)
{
    BranchItem *selbi = getSelectedBranch(bi);
    //qDebug() << "VM::setNote  selbi=" << selbi->headingText() << " n=" << note_new.getText();
    if (selbi) {
        VymNote note_old;
        note_old = selbi->getNote();
        if (note_old == note_new)
            return;

        bool editorStateChanged = false;
        if (note_new.getText() != note_old.getText()) {
            if ((note_new.isEmpty() && !note_old.isEmpty()) ||
                (!note_new.isEmpty() && note_old.isEmpty()))
                editorStateChanged = true;
        }

        //qDebug() << "VM::setNote  selbi=" << selbi->headingText() << " n=" << note_new.getText();

        // branch variable in script
        QString bv = setBranchVar(selbi) + "b.";

        QString uc, rc;
        if (note_old.isRichText())
            uc = QString("%1setNoteRichText(\"%2\");").arg(bv, quoteQuotes(note_old.getText()));
        else
            uc = QString("%1setNoteText(\"%2\");").arg(bv, quoteQuotes(note_old.getText()));
        if (note_new.isRichText())
            rc = QString("%1setNoteRichText(\"%2\");").arg(bv, quoteQuotes(note_new.getText()));
        else
            rc = QString("%1setNoteText(\"%2\");").arg(bv, quoteQuotes(note_new.getText()));

        QString comment = QString("Set note of %1 to \"%2\"").arg(getObjectName(selbi), note_new.getTextASCII().left(40));

        logAction(rc, comment, __func__);

        saveState(uc, rc, comment);

        selbi->setNote(note_new);
        if (!senderIsNoteEditor)
            emitNoteChanged(selbi);

        emitDataChanged(selbi);

        // Only update flag, if state has changed
        if (editorStateChanged)
            reposition();

    }
}

bool VymModel::loadNote(const QString &fn, BranchItem *bi)
{
    BranchItem *selbi = getSelectedBranch(bi);
    if (selbi) {
        QString n;
        if (!loadStringFromDisk(fn, n))
            logWarning("Failed: Couldn't load '" + fn + "'", __func__);
        else {
            VymNote vn;
            vn.setAutoText(n);
            setNote(vn, selbi);
            emitDataChanged(selbi);
            emitUpdateQueries();
            reposition();
            return true;
        }
    }
    else
        qWarning("VymModel::loadNote no branch selected");
    return false;
}

bool VymModel::saveNote(const QString &fn)
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi) {
        VymNote n = selbi->getNote();
        if (n.isEmpty())
            qWarning() << "VymModel::saveNote  note is empty, won't save to "
                       << fn;
        else {
            if (!saveStringToDisk(fn, n.saveToDir()))
                logWarning("Failed: Couldn't save not to '" + fn + "'", __func__);
            else
                return true;
        }
    }
    else
        qWarning("VymModel::saveNote no branch selected");
    return false;
}

void VymModel::findDuplicateURLs() // FIXME-3 Feature needs GUI for viewing
{
    // Generate multimap containing _all_ URLs and branches
    QMultiMap<QString, BranchItem *> multimap;
    QStringList urls;
    BranchItem *cur = nullptr;
    BranchItem *prev = nullptr;
    nextBranch(cur, prev);
    while (cur) {
        QString u = cur->url();
        if (!u.isEmpty()) {
            multimap.insert(u, cur);
            if (!urls.contains(u))
                urls << u;
        }
        nextBranch(cur, prev);
    }

    // Extract duplicate URLs
    foreach (auto u, urls) {
        if (multimap.values(u).size() > -1) {
            qDebug() << "URL: " << u;
            foreach(auto *bi, multimap.values(u))
                qDebug() << " - " << bi->headingPlain();
        }
    }
}

bool VymModel::findAll(FindResultModel *rmodel, QString s,
                       Qt::CaseSensitivity cs, bool searchNotes)
{
    rmodel->clear();
    rmodel->setSearchString(s);
    rmodel->setSearchFlags(QTextDocument::FindFlags()); // FIXME-4 translate cs to
                               // QTextDocument::FindFlag
    bool hit = false;

    BranchItem *cur = nullptr;
    BranchItem *prev = nullptr;
    nextBranch(cur, prev);

    FindResultItem *lastParent = nullptr;
    while (cur) {
        lastParent = nullptr;
        if (cur->heading().getTextASCII().contains(s, cs)) {
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

void VymModel::setUrl(QString url, bool updateFromCloud, BranchItem *bi)
{
    if (!bi) bi = getSelectedBranch();
    if (bi->url() == url)
        return;

    if (bi) {
        QString oldurl = bi->url();
        bi->setUrl(url);

        QString uc = QString("setUrl(\"%1\");").arg(oldurl);
        QString rc = QString("setUrl(\"%1\");").arg(url);

        QString comment = QString("set URL of %1 to %2").arg(getObjectName(bi), url);

        logAction(rc, comment, __func__);

        saveStateBranch(bi, uc, rc, comment);

        if (!url.isEmpty()) {
            if (updateFromCloud) {    // FIXME-3 use oembed.com also for Youtube and other cloud providers
                // Check for Jira
                JiraAgent agent;
                if (agent.setTicket(url)) {
                    logInfo("Preparing to get data from Jira for URL: " + url, __func__);
                    setAttribute(bi, "Jira.key", agent.key());

                    // Initially set heading with ticket Id
                    setHeading(agent.key(), bi);

                    // Then try to update heading from Jira
                    getJiraData(false, bi);
                }

                // Check for Confluence
                if (bi->urlType() != TreeItem::JiraUrl)
                    setConfluencePageDetails(false);
            }
        } else {
            // url == ""
            AttributeItem *ai = getAttributeByKey("Jira.issueUrl", bi);
            if (ai)
                deleteItem(ai);
        }
        updateJiraFlag(bi); // Implicitly calls setUrlType()


        emitDataChanged(bi);
        reposition();
    }
}

QString VymModel::getUrl()
{
    TreeItem *selti = getSelectedItem();
    if (selti)
        return selti->url();
    else
        return QString();
}

QStringList VymModel::getUrls(bool ignoreScrolled)
{
    QStringList urls;
    BranchItem *selbi = getSelectedBranch();
    BranchItem *cur = nullptr;
    BranchItem *prev = nullptr;
    nextBranch(cur, prev, true, selbi);
    while (cur) {
        if (cur->hasUrl() &&
            !(ignoreScrolled && cur->hasScrolledParent()))
            urls.append(cur->url());
        nextBranch(cur, prev, true, selbi);
    }
    return urls;
}

void VymModel::setJiraQuery(const QString &query_new, BranchItem *bi)
{
    QList <BranchItem*> selbis = getSelectedBranches(bi);

    foreach (BranchItem *bi, selbis)
        if (query_new.isEmpty())
            deleteAttribute(bi, "Jira.query");
        else
            setAttribute(bi, "Jira.query", query_new);
}

void VymModel::setFrameAutoDesign(const bool &useInnerFrame, const bool &b, BranchItem *bi)
{
    QList<BranchItem *> selbis = getSelectedBranches(bi);


    BranchContainer *bc;
    foreach (BranchItem *selbi, selbis) {
        QString uif = toS(useInnerFrame);
        QString b_undo = toS(!b);
        QString b_redo = toS(b);
        QString uc = QString("setFrameAutoDesign (%1, %2);").arg(uif, b_undo);
        QString rc = QString("setFrameAutoDesign (%1, %2);").arg(uif, b_redo);

        QString comment = QString("Set automatic design of frame to '%1'").arg(toS(b));

        logAction(rc, comment, __func__);

        saveStateBeginScript(comment);  // setFrameAD, calls setFrame* functions

        bc = selbi->getBranchContainer();
        bc->setFrameAutoDesign(useInnerFrame, b);
        if (b) {
            setFrameType(useInnerFrame, mapDesignInt->frameType(useInnerFrame, selbi->depth()), selbi);
            setFramePenColor(useInnerFrame, mapDesignInt->framePenColor(useInnerFrame, selbi->depth()), selbi);
            setFramePenWidth(useInnerFrame, mapDesignInt->framePenWidth(useInnerFrame, selbi->depth()), selbi);
            setFrameBrushColor(useInnerFrame, mapDesignInt->frameBrushColor(useInnerFrame, selbi->depth()), selbi);
	}

        emitDataChanged(selbi);
        branchPropertyEditor->updateControls();

        saveStateBranch(selbi, uc, rc, comment);
        saveStateEndScript();
    }
}

void VymModel::setFrameType(const bool &useInnerFrame, const FrameContainer::FrameType &t, BranchItem *bi)
{
    QList<BranchItem *> selbis = getSelectedBranches(bi);
    BranchContainer *bc;
    foreach (BranchItem *selbi, selbis) {
        bc = selbi->getBranchContainer();
        if (bc->frameType(useInnerFrame) == t)
            break;

        QString uif = toS(useInnerFrame);

        QString oldFrameTypeName = bc->frameTypeString(bc->frameType(useInnerFrame));
        QString newFrameTypeName = FrameContainer::frameTypeString(t);
        QString uc = QString("setFrameType(%1, \"%2\");").arg(uif, oldFrameTypeName);
        QString rc = QString("setFrameType(%1, \"%2\");").arg(uif, newFrameTypeName);
        QString comment = QString("Set type of frame to %1").arg(newFrameTypeName);

        logAction(rc, comment, __func__);

        bool saveCompleteFrame = false;

        if (t == FrameContainer::NoFrame) {
            // Save also penWidth, colors, etc. to restore frame on undo
            saveCompleteFrame = true;

            saveStateBeginScript("Set frame parameters");   // setFrameType, calls setFrame* functions
            QString colorName = bc->framePenColor(useInnerFrame).name();
            saveStateBranch(selbi,
                    QString("setFramePenColor (%1, \"%2\");").arg(uif, colorName),
                    "",
                    QString("set pen color of frame to %1").arg(colorName));

            colorName = bc->frameBrushColor(useInnerFrame).name();
            saveStateBranch(bi,
                    QString("setFrameBrushColor (%1, \"%2\");").arg(uif, colorName),
                    "",
                    QString("set background color of frame to %1").arg(colorName));

            int i = bc->framePenWidth(useInnerFrame);
            saveStateBranch(selbi,
                    QString("setFramePenWidth (%1, \"%2\");").arg(uif).arg(i),
                    "",
                    QString("set pen width of frame to %1").arg(i));

            i = bc->framePadding(useInnerFrame);
            saveStateBranch(selbi,
                    QString("setFramePadding (%1, \"%2\");").arg(uif, i),
                    "",
                    QString("set padding of frame to %1").arg(i));
        }


        bc->setFrameType(useInnerFrame, t);

        saveStateBranch(selbi, uc, rc, comment);

        if (saveCompleteFrame)
            saveStateEndScript();

        emitDataChanged(selbi);
    }

    if (!selbis.isEmpty()) {
        branchPropertyEditor->updateControls();
        reposition();
    }
    reposition();
}

void VymModel::setFrameType(const bool &useInnerFrame, const QString &s, BranchItem *bi)
{
    setFrameType(useInnerFrame, FrameContainer::frameTypeFromString(s), bi);
}

void VymModel::setFramePenColor(const bool &useInnerFrame, const QColor &col, BranchItem *bi)

{
    QList<BranchItem *> selbis = getSelectedBranches(bi);
    foreach (BranchItem *selbi, selbis) {
        BranchContainer *bc = selbi->getBranchContainer();
        if (bc->frameType(useInnerFrame) != FrameContainer::NoFrame)  {
            QString uif = toS(useInnerFrame);
            QString colorNameOld = bc->framePenColor(useInnerFrame).name();
            QString uc = QString("setFramePenColor (%1, \"%2\");").arg(uif, colorNameOld);
            QString colorNameNew = col.name();
            QString rc = QString("setFramePenColor (%1, \"%2\");").arg(uif, colorNameNew);
            QString comment = QString("Set pen color of frame to %1").arg(colorNameNew);

            logAction(rc, comment, __func__);

            emitDataChanged(selbi);
            branchPropertyEditor->updateControls();

            saveStateBranch(selbi, uc, rc, comment);

            bc->setFramePenColor(useInnerFrame, col);
        }
    }
}

void VymModel::setFrameBrushColor(
    const bool &useInnerFrame, const QColor &col, BranchItem *bi)
{
    QList<BranchItem *> selbis = getSelectedBranches(bi);
    foreach (BranchItem *selbi, selbis) {
        BranchContainer *bc = selbi->getBranchContainer();
        if (bc->frameType(useInnerFrame) != FrameContainer::NoFrame)  {
            QString uif = toS(useInnerFrame);
            QString colorNameOld = bc->framePenColor(useInnerFrame).name();
            QString uc = QString("setFrameBrushColor (%1, \"%2\");").arg(uif, colorNameOld);
            QString colorNameNew = col.name();
            QString rc = QString("setFrameBrushColor (%1, \"%2\");").arg(uif, colorNameNew);
            QString comment = QString("Set background color of frame to %1").arg(colorNameNew);

            logAction(rc, comment, __func__);

            saveStateBranch(selbi, uc, rc, comment);

            bc->setFrameBrushColor(useInnerFrame, col);
        }
        emitDataChanged(selbi);  // Notify HeadingEditor to eventually change BG color
        branchPropertyEditor->updateControls();
    }
}

void VymModel::setFramePadding(
    const bool &useInnerFrame, const int &i, BranchItem *bi)
{
    QList<BranchItem *> selbis = getSelectedBranches(bi);
    foreach (BranchItem *selbi, selbis) {
        BranchContainer *bc = selbi->getBranchContainer();
        if (bc->frameType(useInnerFrame) != FrameContainer::NoFrame)  {
            QString uif = toS(useInnerFrame);
            QString uc = QString("setFramePadding (%1, \"%2\");").arg(uif).arg(bc->framePadding(useInnerFrame));
            QString rc = QString("setFramePadding (%1, \"%2\");").arg(uif).arg(i);
            QString comment = QString("Set padding of frame to '%1").arg(i);

            logAction(rc, comment, __func__);

            saveStateBranch(selbi, uc, rc, comment);

            bc->setFramePadding(useInnerFrame, i);
            emitDataChanged(selbi);
        }
    }
    if (!selbis.isEmpty()) {
        branchPropertyEditor->updateControls();
        reposition();
    }
}
void VymModel::setFramePenWidth(
    const bool &useInnerFrame, const int &i, BranchItem *bi)
{
    QList<BranchItem *> selbis = getSelectedBranches(bi);
    foreach (BranchItem *selbi, selbis) {
        BranchContainer *bc = selbi->getBranchContainer();
        if (bc->frameType(useInnerFrame) != FrameContainer::NoFrame)  {
            QString uif = toS(useInnerFrame);
            QString uc = QString("setFramePenWidth (%1, \"%2\");").arg(uif).arg(bc->framePenWidth(useInnerFrame));
            QString rc = QString("setFramePenWidth (%1, \"%2\");").arg(uif).arg(i);
            QString comment = QString("Set pen width of frame to '%1").arg(i);

            logAction(rc, comment, __func__);

            saveStateBranch(selbi, uc, rc, comment);

            bc->setFramePenWidth(useInnerFrame, i);
            emitDataChanged(selbi);
        }
    }

    if (!selbis.isEmpty()) {
        branchPropertyEditor->updateControls();
        reposition();
    }
}

void VymModel::setHeadingColumnWidthAutoDesign(const bool &b, BranchItem *bi)
{
    QList<BranchItem *> selbis = getSelectedBranches(bi);
    BranchContainer *bc;
    foreach (BranchItem *selbi, selbis) {
        bc = selbi->getBranchContainer();
        if (bc->columnWidthAutoDesign() != b) {
            if (b)
                bc->setColumnWidth(mapDesignInt->headingColumnWidth(selbi->depth()));
	    QString v = b ? "Enable" : "Disable";
	    QString uc = QString("setHeadingColumnWidthAutoDesign (%1);").arg(toS(!b));
	    QString rc = QString("setHeadingColumnWidthAutoDesign (%1);").arg(toS(b));
            QString comment = QString("%1 automatic heading width").arg(v);

            logAction(rc, comment, __func__);

            saveStateBranch(selbi, uc, rc, comment);
            bc->setColumnWidthAutoDesign(b);
            branchPropertyEditor->updateControls();
            emitDataChanged(selbi);
        }
    }

    if (!selbis.isEmpty()) {
        branchPropertyEditor->updateControls();
        reposition();
    }
}

void VymModel::setHeadingColumnWidth (const int &i, BranchItem *bi)
{
    QList<BranchItem *> selbis = getSelectedBranches(bi);
    foreach (BranchItem *selbi, selbis) {
        BranchContainer *bc = selbi->getBranchContainer();
	if (bc->columnWidth() != i) {
	    QString uc = QString("setHeadingColumnWidth (%1);").arg(bc->columnWidth());
	    QString rc = QString("setHeadingColumnWidth (%1);").arg(i);
            QString comment = QString("Set heading column width to %1").arg(i);

            logAction(rc, comment, __func__);

            saveStateBranch(selbi, uc, rc, comment);

            bc->setColumnWidth(i);
            emitDataChanged(selbi);
        }
    }

    if (!selbis.isEmpty()) {
        branchPropertyEditor->updateControls();
        reposition();
    }
}

void VymModel::setRotationAutoDesign(const bool &b, BranchItem *bi)
{
    QList<BranchItem *> selbis = getSelectedBranches(bi);

    foreach (BranchItem *selbi, selbis) {
        BranchContainer *bc = selbi->getBranchContainer();
        if (bc->rotationsAutoDesign() != b) {
            QString s = b ? "Enable" : "Disable";
            QString uc = QString("setRotationAutoDesign(%1);").arg(toS(bc->rotationsAutoDesign()));
            QString rc = QString("setRotationAutoDesign(%1);").arg(toS(b));
            QString comment = QString("%1 automatic rotation heading and subtree").arg(s);

            logAction(rc, comment, __func__);

            saveStateBeginScript(comment); // setRotationsAD, calls setRotation* functions
            if (b) {
                setRotationHeading(mapDesignInt->rotationHeading(selbi->depth()));
                setRotationSubtree(mapDesignInt->rotationSubtree(selbi->depth()));
            }
            saveStateBranch(selbi, uc, rc);

            bc->setRotationsAutoDesign(b);
            emitDataChanged(selbi);

            saveStateEndScript();
        }
    }

    if (!selbis.isEmpty()) {
        branchPropertyEditor->updateControls();
        reposition();
    }
}

void VymModel::setRotationHeading (const int &i, BranchItem* bi)
{
    QList<BranchItem *> selbis = getSelectedBranches(bi);

    foreach (BranchItem *selbi, selbis) {
        BranchContainer *bc = selbi->getBranchContainer();
	if (bc->rotationHeading() != i) {

            QString uc = QString("setRotationHeading(\"%1\");").arg(toS(bc->rotationHeading(), 1));
            QString rc = QString("setRotationHeading(\"%1\");").arg(i);
            QString comment = QString("Set rotation angle of heading and flags to %1").arg(i);

            logAction(rc, comment, __func__);

            saveStateBranch(selbi, uc, rc, comment);

            bc->setRotationHeading(i);
            emitDataChanged(selbi);
        }
    }

    if (!selbis.isEmpty()) {
        branchPropertyEditor->updateControls();
        reposition();
    }
}

void VymModel::setRotationSubtree (const int &i, BranchItem *bi)
{
    QList<BranchItem *> selbis = getSelectedBranches(bi);

    foreach (BranchItem *selbi, selbis) {
        BranchContainer *bc = selbi->getBranchContainer();
	if (bc->rotationSubtree() != i) {
            QString uc = QString("setRotationSubtree(\"%1\");").arg(toS(bc->rotationSubtree(), 1));
            QString rc = QString("setRotationSubtree(\"%1\");").arg(i);
            QString comment = QString("Set rotation angle of subtree to %1").arg(i);

            logAction(rc, comment, __func__);

            saveStateBranch(selbi, uc, rc, comment);

            bc->setRotationSubtree(i);
            emitDataChanged(selbi);
	}
    }

    if (!selbis.isEmpty()) {
        branchPropertyEditor->updateControls();
        reposition();
    }
}

void VymModel::rotateSubtree(qreal a)
{
    QList<BranchItem *> selbis = getSelectedBranches();

    foreach (BranchItem *selbi, selbis) {
        BranchContainer *bc = selbi->getBranchContainer();
        qreal a_old = bc->rotationSubtree();
        qreal a_new = a_old + a;
        QString uc = QString("setRotationSubtree(\"%1\");").arg(toS(a_old, 1));
        QString rc = QString("setRotationSubtree(\"%1\");").arg(a_new);
        QString comment = QString("Set rotation angle of subtree to %1").arg(a_new);

        logAction(rc, comment, __func__);

        saveStateBranch(selbi, uc, rc, comment);

        bc->setRotationSubtree(a_new);
        emitDataChanged(selbi);
    }

    if (!selbis.isEmpty()) {
        branchPropertyEditor->updateControls();
        reposition();
    }
}

void VymModel::setScaleAutoDesign (const bool & b, BranchItem *bi)
{
    QList<BranchItem *> selbis = getSelectedBranches(bi);

    BranchContainer *bc;
    foreach (BranchItem *selbi, selbis) {
        bc = selbi->getBranchContainer();
        if (bc->scaleAutoDesign() != b) {
            QString s = b ? "Enable" : "Disable";
            QString uc = QString("setScaleAutoDesign(%1);").arg(toS(bc->scaleAutoDesign()));
            QString rc = QString("setScaleAutoDesign(%1);").arg(toS(b));
            QString c = QString("%1 automatic scaling").arg(s);

            logAction(rc, c, __func__);

            saveStateBeginScript(c);    // setScaleAD, calls setScale* functions
            if (b) {
                setScaleHeading(mapDesignInt->scaleHeading(selbi->depth()));
                setScaleSubtree(mapDesignInt->scaleSubtree(selbi->depth()));
            }
            saveStateBranch(selbi, uc, rc);
            bc->setScaleAutoDesign(b);
            emitDataChanged(selbi);

            saveStateEndScript();
        }
    }

    if (!selbis.isEmpty()) {
        branchPropertyEditor->updateControls();
        reposition();
    }
}

void VymModel::setScaleHeading (const qreal &f, const bool relative, BranchItem *bi)
{
    QList<BranchItem *> selbis = getSelectedBranches(bi);

    BranchContainer *bc;
    foreach (BranchItem *selbi, selbis) {
        bc = selbi->getBranchContainer();
        qreal f_old = bc->scaleHeading();
        qreal f_new = relative ? f_old + f : f;

	if (bc->scaleHeading() != f_new) {
            QString uc = QString("setScaleHeading(%1);").arg(toS(f_old, 3));
            QString rc = QString("setScaleHeading(%1);").arg(toS(f_new, 3));
            QString c  = QString("Set heading scale factor to %1").arg(f_new);

            logAction(rc, c, __func__);

            saveStateBranch(selbi, uc, rc, c);

            bc->setScaleHeading(f_new);
            emitDataChanged(selbi);
	}
    }

    if (!selbis.isEmpty()) {
        branchPropertyEditor->updateControls();
        reposition();
    }
}

qreal VymModel::getScaleHeading ()
{
    QList<BranchItem *> selbis = getSelectedBranches();

    if (selbis.isEmpty()) return 1;

    return selbis.first()->getBranchContainer()->scaleHeading();
}


void VymModel::setScaleSubtree (const qreal &f_new, BranchItem *bi)
{
    QList<BranchItem *> selbis = getSelectedBranches(bi);

    BranchContainer *bc;
    foreach (BranchItem *selbi, selbis) {
        bc = selbi->getBranchContainer();
        qreal f_old = bc->scaleSubtree();

	if (f_new != f_old) {
            QString uc = QString("setScaleSubtree(%1);").arg(toS(f_old, 3));
            QString rc = QString("setScaleSubtree(%1);").arg(toS(f_new,3));
            QString c  = QString("Set subtree scale factor to %1").arg(toS(f_new, 3));
            logAction(rc, c, __func__);
            saveStateBranch(selbi, uc, rc, c);

            bc->setScaleSubtree(f_new);
            branchPropertyEditor->updateControls();
            emitDataChanged(selbi);
	}
    }

    if (!selbis.isEmpty()) {
        branchPropertyEditor->updateControls();
        reposition();
    }
}

qreal VymModel::getScaleSubtree ()
{
    QList<BranchItem *> selbis = getSelectedBranches();

    if (selbis.isEmpty()) return 1;

    return selbis.first()->getBranchContainer()->scaleSubtree();
}

void VymModel::setScaleImage(const qreal &f, const bool relative, ImageItem *ii)
{
    QList<ImageItem *> seliis = getSelectedImages(ii);

    foreach (ImageItem *selii, seliis) {
        qreal f_old = selii->scale();
        qreal f_new = relative ? f_old + f : f;
        if (selii->scale() != f_new) {
            QString iv = setImageVar(selii);
            QString uc = iv + QString("i.setScale(%1);").arg(toS(f_old, 3));
            QString rc = iv + QString("i.setScale(%1);").arg(toS(f_new,3));
            QString c  = QString("Set image scale factor to %1").arg(toS(f_new, 3));
            logAction(rc, c, __func__);
            saveState(uc, rc, c);

            selii->setScale(f_new);
            emitDataChanged(selii);
        }
    }

    if (!seliis.isEmpty()) {
        branchPropertyEditor->updateControls();
        reposition();
    }
}

void VymModel::setScale(const qreal &f, const bool relative)
{
    // Grow/shring branches and/or images using shortcuts
    setScaleAutoDesign(false);
    setScaleHeading(f, relative);
    setScaleImage(f, relative);
}

void VymModel::growSelectionSize()
{
    setScale(0.05, true);
}

void VymModel::shrinkSelectionSize()
{
    setScale(- 0.05, true);
}

void VymModel::resetSelectionSize()
{
    setScale(1, false);
}

void VymModel::setBranchesLayout(const QString &s, BranchItem *bi)
{
    // qDebug() << "VM::setBranchesLayout for " << headingText(bi) << s;
    QList<BranchItem *> selbis = getSelectedBranches(bi);
    BranchContainer *bc;
    foreach (BranchItem *selbi, selbis) {
        Container::Layout layout;
        bc = selbi->getBranchContainer();

        if (s == "Auto") {
            // Set layout to "auto"
            bc->branchesContainerAutoLayout = true;

            // Get layout from mapDesign
            layout = mapDesignInt->branchesContainerLayout(selbi->depth());
        } else {
            bc->branchesContainerAutoLayout = false;
            layout = Container::layoutFromString(s);
        }
        if (bc->branchesContainerLayout() != layout  && layout != Container::UndefinedLayout) {
            QString bv = setBranchVar(bi);
            QString uc = bv + "map.loadBranchReplace(\"UNDO_PATH\", b);";
            QString rc = bv + QString("b.setBranchesLayout (\"%1\")").arg(s);
            QString com = QString("Set branches layout of %1 to %2").arg(getObjectName(bi), layout);
            logAction(rc, com, __func__);

            saveState(uc, rc, com, bi);

            bc->setBranchesContainerLayout(layout);
            emitDataChanged(selbi);
        }
    }

    // Links might have been added or removed, Nested lists, etc...
    foreach (BranchItem *selbi, selbis)
        applyDesignRecursively(MapDesign::LayoutChanged, selbi);

    if (!selbis.isEmpty()) {
        branchPropertyEditor->updateControls();

        // Create and delete containers, update their structure
        reposition();
    }

}

void VymModel::setImagesLayout(const QString &s, BranchItem *bi)
{
    BranchContainer *bc;
    QList<BranchItem *> selbis = getSelectedBranches(bi);
    foreach (BranchItem *selbi, selbis) {
        Container::Layout layout;
        bc = selbi->getBranchContainer();
        if (s == "Auto") {
            bc->imagesContainerAutoLayout = true;
            layout = mapDesignInt->imagesContainerLayout(selbi->depth());
        } else {
            bc->imagesContainerAutoLayout = false;
            layout = Container::layoutFromString(s);
        }

        if (bc->imagesContainerLayout() != layout  && layout != Container::UndefinedLayout) {
            QString bv = setBranchVar(selbi);
            QString uc = bv + "map.loadBranchReplace(\"UNDO_PATH\", b);";
            QString rc = bv + QString("b.setImagesLayout (\"%1\")").arg(s);
            QString com = QString("Set images layout of %1 to %2").arg(getObjectName(selbi), s);
            logAction(rc, com, __func__);

            saveState(uc, rc, com, selbi);

            bc->setImagesContainerLayout(layout);
            emitDataChanged(selbi);
        }
    }

    if (!selbis.isEmpty()) {
        branchPropertyEditor->updateControls();
        reposition();
    }
}

void VymModel::setHideLinkUnselected(bool b, TreeItem *ti)
{
    QList <TreeItem*> seltis = getSelectedItems(ti);

    foreach (TreeItem *selti, seltis) {
        if (selti->getType() == TreeItem::Image || selti->hasTypeBranch()) {
            QString v = b ? "Hide" : "Show";
            QString tiv;
            if (selti->hasTypeBranch())
                tiv = setBranchVar((BranchItem*)selti, "ti");
            else if (selti->hasTypeImage())
                tiv = setImageVar((ImageItem*)selti, "ti");
            else {
                qWarning() << "VymModel::setHideLinkUnselected  no branch or image";
                return;
            }
            QString uc = tiv + QString("ti.setHideLinkUnselected(%1);").arg(toS(!b));
            QString rc = tiv + QString("ti.setHideLinkUnselected(%1);").arg(toS(b));
            QString comment = QString("%1 link if item %2 is not selected").arg(v, getObjectName(selti));

            logAction(rc, comment, __func__);

            saveState( uc, rc, comment);
            ((MapItem *)selti)->setHideLinkUnselected(b);
        }
        emitDataChanged(selti);
    }

    if (!seltis.isEmpty()) {
        branchPropertyEditor->updateControls();
        reposition();
    }
}

void VymModel::setHideExport(bool b, BranchItem *bi)
{
    QList <BranchItem*> selbis = getSelectedBranches(bi);

    foreach (BranchItem *selbi, selbis) {
        if (selbi->hideTemporary() != b) {
            selbi->setHideTemporary(b);
            QString u = toS(!b);
            QString r = toS(b);
            QString uc = QString("setHideExport (%1)").arg(u);
            QString rc = QString("setHideExport (%1)").arg(r);

            QString comment = "Set hide export of " + getObjectName(selbi) + " to " + r;

            logAction(rc, comment, __func__);

            saveStateBranch(selbi, uc, uc, comment);

            emitDataChanged(selbi);
        }
    }

    if (!selbis.isEmpty())
        reposition();
}

void VymModel::toggleHideExport()
{
    QList<BranchItem *> selbis = getSelectedBranches();
    foreach (BranchItem *selbi, selbis) {
        bool b = !selbi->hideTemporary();
        setHideExport(b, selbi);
    }
}

void VymModel::toggleTask(BranchItem *bi)
{
    QList<BranchItem *> selbis = getSelectedBranches(bi);
    foreach (auto selbi, selbis) {
        QString uc = "toggleTask();";
        QString comment = QString("Toggle task of %1").arg(getObjectName(selbi));

        logAction(uc, comment, __func__);

        saveStateBranch( selbi, uc, uc, comment);
        Task *task = selbi->getTask();
        if (!task) {
            task = taskModel->createTask(selbi);
            taskEditor->select(task);
        }
        else
            taskModel->deleteTask(task);

        emitDataChanged(selbi);
        reposition();
    }
}

bool VymModel::cycleTaskStatus(BranchItem *bi, bool reverse)
{
    bool repositionRequired = false;
    QList<BranchItem *> selbis = getSelectedBranches(bi);
    foreach (BranchItem *selbi, selbis) {
        Task *task = selbi->getTask();
        if (task) {
            QString uc, rc;
            if (!reverse) {
                uc = "cycleTask(true);";
                rc = "cycleTask();";
            } else {
                uc = "cycleTask();";
                rc = "cycleTask(true);";
            }
            QString comment = QString("Cycle task of %1").arg(getObjectName(selbi));

            logAction(rc, comment, __func__);

            saveStateBranch(selbi, uc, rc, comment);

            task->cycleStatus(reverse);
            task->setDateModification();

            // make sure task is still visible  // FIXME-3 for multi-selections?
            taskEditor->select(task);
            emitDataChanged(selbi);
            repositionRequired = true;
        }
    }
    if (repositionRequired) {
        reposition();
        return true;
    }
    return false;
}

bool VymModel::setTaskSleep(const QString &s, BranchItem *bi) // FIXME-4 (WIP) Rename "sleep" to "alarm" in code, commands, doc
{
    bool ok = false;
    QList<BranchItem *> selbis = getSelectedBranches(bi);
    foreach (auto selbi, selbis) {
        Task *task = selbi->getTask();
        if (task) {
            QDateTime oldAlarmTime = task->alarmTime();

            // Parse the string, which could be days, hours or one of several
            // time formats

            if (s == "0") {
                // Reset sleep time and wake up task
                ok = task->setSecsSleep(0);
            }
            else {
                static QRegularExpression re;

                // Only digits considered as days
                re.setPattern("^\\s*(\\d+)\\s*$");
                re.setPatternOptions(QRegularExpression::InvertedGreedinessOption);
                QRegularExpressionMatch match = re.match(s);
                if (match.hasMatch()) {
                    ok = task->setDaysSleep(match.captured(1).toInt());
                }
                else {
                    // Digit followed by "h", considered as hours
                    re.setPattern("^\\s*(\\d+)\\s*h\\s*$");
                    match = re.match(s);
                    if (match.hasMatch()) {
                        ok = task->setHoursSleep(match.captured(1).toInt());
                    }
                    else {
                        // Digits followed by "w", considered as weeks
                        re.setPattern("^\\s*(\\d+)\\s*w\\s*$");
                        match = re.match(s);
                        if (match.hasMatch()) {
                            ok = task->setDaysSleep(7 * match.captured(1).toInt());
                        }
                        else {
                            // Digits followed by "s", considered as seconds
                            re.setPattern("^\\s*(\\d+)\\s*s\\s*$");
                            match = re.match(s);
                            if (match.hasMatch()) {
                                ok = task->setSecsSleep(match.captured(1).toInt());
                            }
                            else {
                                // Try setting ISO date YYYY-MM-DDTHH:mm:ss
                                ok = task->setDateSleep(s);

                                if (!ok) {
                                    // German format, e.g. "24.12.2012"
                                    re.setPattern("(\\d+)\\.(\\d+)\\.(\\d+)");
                                    re.setPatternOptions(QRegularExpression::NoPatternOption);
                                    match = re.match(s);
                                    if (match.hasMatch()) {
                                        QDateTime d(
                                            QDate(match.captured(3).toInt(),
                                                  match.captured(2).toInt(),
                                                  match.captured(1).toInt()).startOfDay());
                                        ok = task->setDateSleep(d); 
                                    }
                                    else {
                                        // Short German format, e.g. "24.12."
                                        re.setPattern("(\\d+)\\.(\\d+)\\.");
                                        re.setPatternOptions(QRegularExpression::InvertedGreedinessOption);
                                        match = re.match(s);
                                        if (match.hasMatch()) {
                                            int month = match.captured(2).toInt();
                                            int day = match.captured(1).toInt();
                                            int year =
                                                QDate::currentDate().year();
                                            QDateTime d(QDate(year, month, day).startOfDay());
                                            // d = QDate(year, month,
                                            // day).startOfDay();
                                            if (QDateTime::currentDateTime()
                                                    .daysTo(d) < 0) {
                                                year++;
                                                d = QDateTime(
                                                    QDate(year, month, day).startOfDay());
                                                // d = QDate(year, month,
                                                // day).startOfDay();
                                            }
                                            ok = task->setDateSleep(d);
                                        }
                                        else {
                                            // Time HH:MM
                                            re.setPattern("(\\d+)\\:(\\d+)");
                                            match = re.match(s);
                                            if (match.hasMatch()) {
                                                int hour = match.captured(1).toInt();
                                                int min = match.captured(2).toInt();
                                                QDateTime d(
                                                    QDate::currentDate(),
                                                    QTime(hour, min));
                                                ok = task->setDateSleep(d);
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
                QString oldAlarmTimeString;
                if (oldAlarmTime.isValid())
                    oldAlarmTimeString = oldAlarmTime.toString(Qt::ISODate);
                else
                    oldAlarmTimeString = "1970-01-26T00:00:00"; // Some date long ago...

                QString newAlarmTimeString = task->alarmTime().toString(Qt::ISODate);
                task->setDateModification();
                selbi->updateTaskFlag(); // If tasks changes awake mode, then
                                         // flag needs to change
                QString bv = setBranchVar(selbi);
                QString uc = QString("setTaskSleep (\"%1\")").arg(oldAlarmTimeString);
                QString rc = QString("setTaskSleep (\"%1\")").arg(newAlarmTimeString);
                QString comment = "Set sleep time for task";

                logAction(rc, comment, __func__);  // FIXME-3 Logging command should be done before actual change. 
                                                    // Would require separate checks in task, if new alarmTime is valid

                saveStateBranch(selbi, uc, rc, comment);

                emitDataChanged(selbi);
                reposition();
            }

        } // Found task
        if (!ok)
            return false;
    }     // Looping over selected branches
    return ok;
}

void VymModel::setTaskPriorityDelta(const int &pd, BranchItem *bi)
{
    QList<BranchItem *> selbis = getSelectedBranches(bi);

    foreach (BranchItem *selbi, selbis) {
        Task *task = selbi->getTask();
        if (task) {
            QString bv = setBranchVar(selbi);
            QString uc = QString("setTaskPriorityDelta (%1)").arg(task->getPriorityDelta());
            QString rc = QString("setTaskPriorityDelta (%1)").arg(pd);
            QString comment = "Set delta for priority of task";
            logAction(rc, comment, __func__);
            saveStateBranch(selbi, uc, rc, comment);
            task->setPriorityDelta(pd);
            emitDataChanged(selbi);
        }
    }
}

int VymModel::getTaskPriorityDelta(BranchItem *bi)
{
    BranchItem *selbi = getSelectedBranch(bi);
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
    BranchItem *selbi = getSelectedBranch();

    if (selbi) {
        QDate today = QDate::currentDate();
        QChar c = '0';
        QString s = QString("%1-%2-%3")
                       .arg(today.year(), 4, 10, c)
                       .arg(today.month(), 2, 10, c)
                       .arg(today.day(), 2, 10, c);
        QString comment = "Add branch with current date as heading: " + s;

        saveStateBeginScript(comment);  // addTimeStamp, calls setHeadingPlain and indirect setUrl
        BranchItem *newbi = addNewBranch(selbi);
        setHeadingPlainText(s, newbi);
        saveStateEndScript();

        select(newbi);
    }
    return selbi;
}

void VymModel::copy()
{
    if (readonly)
        return;

    QList<TreeItem *> itemList = getSelectedItems();

    QStringList clipboardFileNames;

    if (itemList.count() > 0) {

        QStringList uids;

        uint i = 1;
        QString fn;
        foreach (TreeItem *ti, itemList) {
            uids << QString("\"%1\"").arg(ti->getUuid().toString());
            fn = QString("%1/%2-%3.xml")
                     .arg(clipboardDir.path())
                     .arg(clipboardFileName)
                     .arg(i);
            QString content = saveToDir(clipboardDir.path(), clipboardFileName,
                                        FlagRowMaster::NoFlags, QPointF(), false, false, false,  ti);

            if (!saveStringToDisk(fn, content))
                qWarning() << "ME::saveStringToDisk failed: " << fn;
            else {
                i++;
                clipboardFileNames.append(fn);
            }
        }
        QClipboard *clipboard = QApplication::clipboard();
        QMimeData *mimeData = new QMimeData;
        mimeData->setData("application/x-vym", clipboardFileNames.join(",").toLatin1());
        clipboard->setMimeData(mimeData);

        QString rc = QString("map.selectUids([%1]); map.copy();").arg(uids.join(","));
        QString comment = QString("Copy %1 selected %2 to clipboard: [%3]")
            .arg(itemList.count())
            .arg(pluralize(QString("item"), itemList.count()))
            .arg(uids.join(","));

        logAction(rc, "Copy selection", __func__);
        saveState("", rc, comment);

    }
}

void VymModel::paste()
{
    if (readonly)
        return;

    const QClipboard *clipboard = QApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();

    BranchItem *selbi = getSelectedBranch();
    ImageItem *selii = getSelectedImage();

    // Special case: When image is selected and image is pasted, try to append 
    // pasted image to current set of images in parent
    if (!selbi && selii && mimeData->hasImage())
        selbi = selii->parentBranch();

    if (selbi) {
        if (mimeData->formats().contains("application/x-vym")) {
            QStringList clipboardFileNames = QString(mimeData->data("application/x-vym")).split(",");

            QString bv = setBranchVar(selbi);
            QString uc = bv + QString("map.loadBranchReplace(\"UNDO_PATH\", b);");
            QString rc = bv + QString("b.select(); map.paste();");
            QString comment = QString("Paste to branch \"%1\"").arg(selbi->headingText());

            logAction(rc, comment, __func__);

            saveState(uc, rc, comment, selbi, selbi);

            bool zippedOrg = zipped;
            foreach(QString fn, clipboardFileNames) {
                if (!loadMap(fn,
                            File::ImportAdd,
                            File::VymMap,
                            VymReader::SlideContent,
                            selbi,
                            selbi->branchCount()))
                    logWarning("Failed to load clipboard from " + fn, __func__);
            }
            select(selbi);
            zipped = zippedOrg;
        } else if (mimeData->hasImage()) {
            //qDebug() << "VM::paste  mimeData->hasImage";
            QImage image = qvariant_cast<QImage>(mimeData->imageData());
            QString fn = clipboardDir.path() + "/" + "image.png";
            if (!image.save(fn))
                logWarning("Could not save copy of image in system clipboard " + fn, __func__);
            else {
                ImageItem *ii = loadImage(selbi, fn);
                if (ii) {
                    setScaleImage(300.0 / image.width(), false, ii);    // FIXME-3 Better use user-defined fixed width when pasting images

                    if (selii)
                        // In case we pasted onto an existing image, select the new one
                        select(ii);
                }
            }
        } else if (mimeData->hasHtml()) {
            //setText(mimeData->html());
            //setTextFormat(Qt::RichText);
            qDebug() << "VM::paste found html...";
        } else if (mimeData->hasText()) {
            //setText(mimeData->text());
            //setTextFormat(Qt::PlainText);
            qDebug() << "VM::paste found text...";
        } else {
            logWarning("Cannot paste data, mimeData->formats=" + mimeData->formats().join(","), __func__);
        }
    }
}

void VymModel::cut()
{
    if (readonly)
        return;

    copy();
    deleteSelection();
}

bool VymModel::canMoveUp(TreeItem *ti)
{
    if (ti) {
        BranchItem *pbi;
        if (ti->hasTypeBranch())
            pbi = ((BranchItem*)ti)->parentBranch();
        else if (ti->hasTypeImage())
            pbi = ((ImageItem*)ti)->parentBranch();
        else 
            return false;

        if (pbi == rootItem)
            return false;

        return (pbi->num(ti) > 0);
    }

    return false;
}

bool VymModel::canMoveDown(TreeItem *ti)
{
    if (ti) {
        BranchItem *pbi;

        if (ti->hasTypeBranch()) {
            pbi = ((BranchItem*)ti)->parentBranch();
            if (pbi == rootItem)
                return false;
            return (pbi->num(ti) < pbi->branchCount() - 1);
        } else if (ti->hasTypeImage()) {
            pbi = ((ImageItem*)ti)->parentBranch();
            return (pbi->num(ti) < pbi->imageCount() - 1);
        }
    }

    return false;
}

void VymModel::moveUp(TreeItem *ti)
{
    if (readonly) return;


    QList<BranchItem *> selbis = getSelectedBranches(ti);

    if (!selbis.isEmpty()){
        foreach (BranchItem *selbi, sortBranchesByNum(selbis, false)) {
            if (canMoveUp(selbi)) {
                logAction("", "Starting to move branch up: " + headingText(selbi), __func__);
                relinkBranch(selbi, selbi->parentBranch(), selbi->num() - 1);
            }
        }
    }

    QList<ImageItem *> seliis = getSelectedImages(ti);

    if (!seliis.isEmpty()){
        foreach (ImageItem *selii, sortImagesByNum(seliis, false)) {
            if (canMoveUp(selii)) {
                logAction("", "Starting to move image up: " + headingText(selii), __func__);
                relinkImage(selii, selii->parentBranch(), selii->num() - 1);
            }
        }
    }
}

void VymModel::moveDown(TreeItem *ti)
{
    if (readonly) return;

    QList<BranchItem *> selbis = getSelectedBranches(ti);
    if (!selbis.isEmpty()) {
        foreach (BranchItem *selbi, sortBranchesByNum(selbis, true))
            if (canMoveDown(selbi)) {
                logAction("", "Starting to move branch down: " + headingText(selbi), __func__);
                relinkBranch(selbi, selbi->parentBranch(), selbi->num() + 1);
            }
    }

    QList<ImageItem *> seliis = getSelectedImages(ti);
    if (!seliis.isEmpty()) {
        foreach (ImageItem *selii, sortImagesByNum(seliis, true))
            if (canMoveDown(selii)) {
                logAction("", "Starting to move image down: " + headingText(selii), __func__);
                relinkImage(selii, selii->parentBranch(), selii->num() + 1);
            }
    }
}

void VymModel::moveUpDiagonally()
{
    if (readonly) return;   // FIXME-3 readonly needs be checked for every 
                            // public function in model, which modifies data...

    QList<BranchItem *> selbis = getSelectedBranches();

    foreach (BranchItem *selbi, selbis) {
        logAction("", "Starting to move up diagonally: " + headingText(selbi), __func__);
        BranchItem *pbi = selbi->parentBranch();
        if (pbi == rootItem) break;

        int n = selbi->num();
        if (n == 0) break;

        BranchItem *dst = pbi->getBranchNum(n - 1);
        if (!dst) break;

        relinkBranch(selbi, dst, dst->branchCount() + 1);
    }
}

void VymModel::moveDownDiagonally()
{
    if (readonly) return;


    QList<BranchItem *> selbis = getSelectedBranches();
    foreach (BranchItem *selbi, selbis) {
        logAction("", "Starting to move down diagonally: " + headingText(selbi), __func__);
        BranchItem *pbi = selbi->parentBranch();
        if (pbi == rootItem) break;
        BranchItem *parentParent = pbi->parentBranch();
        int n = pbi->num();

        relinkBranch(selbi, parentParent, n + 1);
    }
}

void VymModel::detach(BranchItem *bi)
{
    QList<BranchItem *> selbis;
    if (bi)
        selbis << bi;
    else
        selbis = getSelectedBranches();
    foreach (BranchItem *selbi, selbis) {
        if (selbi->depth() > 0) {
            BranchContainer *bc = selbi->getBranchContainer();
            if (bc)
                bc->setOriginalPos();
            relinkBranch(selbi, rootItem, -1);
            bc->updateUpLink();
        }
    }
}

QList <BranchItem*> VymModel::sortBranchesByNum(QList <BranchItem*> unsortedList, bool inverse)
{
    // Shortcut
    if (unsortedList.count() < 2)
        return unsortedList;

    // We use QMultiMap because unsortedList might have branches 
    // with identical depths, but different parentBranches e.g.
    // when moving up/down. Then parts of the list would be lost.
    QMultiMap <int, BranchItem*> multimap;
    foreach (BranchItem *bi, unsortedList)
        multimap.insert(bi->num(), bi);

    QList <BranchItem*> sortedList;

    QMultiMapIterator<int, BranchItem*> i(multimap);
    if (inverse) {
            i.toBack();
            while (i.hasPrevious()) {
                i.previous();
                sortedList << i.value();
            }
    } else while (i.hasNext()) {
        i.next();
        sortedList << i.value();
    }

    return sortedList;
}

QList <BranchItem*> VymModel::sortBranchesByHeading(QList <BranchItem*> unsortedList, bool inverse) // FIXME-4 This funcion is currently never called
{
    QMap <QString, BranchItem*> map;
    foreach (BranchItem *bi, unsortedList)
        map.insert(bi->headingPlain(), bi);

    QList <BranchItem*> sortedList;

    if (inverse)
        for (auto i = map.cend(), begin = map.cbegin(); i != begin; --i)
            sortedList << i.value();
    else
        for (auto i = map.cbegin(), end = map.cend(); i != end; ++i)
            sortedList << i.value();

    return sortedList;
}

void VymModel::sortChildren(bool inverse, BranchItem *bi)
{
    QList<BranchItem *> selbis = getSelectedBranches(bi);

    if (selbis.isEmpty()) return;

    foreach (BranchItem *selbi, selbis) {
        if (selbi) {
            if (selbi->branchCount() > 1) {
                QString bv = setBranchVar(selbi);
                QString uc = bv + QString("map.loadBranchReplace(\"UNDO_PATH\", b);");
                QString com;
                QString rc;
                if (!inverse) {
                    rc = bv + QString("b.sortChildren(false);");
                    com = QString("Sort children of \"%1\"").arg(getObjectName(selbi));
                } else {
                    bv = setBranchVar(selbi);
                    uc = bv + QString("map.loadBranchReplace(\"UNDO_PATH\", b);");
                    rc = bv + QString("b.sortChildren(false);");
                    com = QString("Inverse sort children of \"%1\"").arg(getObjectName(selbi));
                }
                logAction(rc, com, __func__);
                saveState(uc, rc, com, selbi);

                QMultiMap <QString, BranchItem*> multimap;
                for (int i = 0; i < selbi->branchCount(); i++)
                    multimap.insert(selbi->getBranchNum(i)->headingPlain(), selbi->getBranchNum(i));

                int n = 0;
                QMultiMapIterator<QString, BranchItem*> i(multimap);
                if (inverse) {
                    i.toBack();
                    while (i.hasPrevious()) {
                        i.previous();
                        if (i.value()->num() != n) {
                            // Only relink if not already at this position
                            // and don't saveState while relinking
                            bool oldSaveStateBlocked = saveStateBlocked;
                            saveStateBlocked = true;
                            relinkBranch(i.value(), selbi, n);
                            saveStateBlocked = oldSaveStateBlocked;
                        }
                        n++;
                    }
                } else while (i.hasNext())  {
                    i.next();
                    if (i.value()->num() != n) {
                        // Only relink if not already at this position
                        bool oldSaveStateBlocked = saveStateBlocked;
                        saveStateBlocked = true;
                        relinkBranch(i.value(), selbi, n);
                        saveStateBlocked = oldSaveStateBlocked;
                    }
                    n++;
                }
            }
        }
    }
}

QList <ImageItem*> VymModel::sortImagesByNum(QList <ImageItem*> unsortedList, bool inverse)
{   // for moving up/down *multiple* images in lists or grid...

    if (unsortedList.count() < 2)
        return unsortedList;

    // We use QMultiMap because unsortedList might have branches 
    // with identical depths, but different parentBranches e.g.
    // when moving up/down. Then parts of the list would be lost.
    QMultiMap <int, ImageItem*> multimap;
    foreach (ImageItem *ii, unsortedList)
        multimap.insert(ii->num(), ii);

    QList <ImageItem*> sortedList;

    QMultiMapIterator<int, ImageItem*> i(multimap);
    if (inverse) {
            i.toBack();
            while (i.hasPrevious()) {
                i.previous();
                sortedList << i.value();
            }
    } else while (i.hasNext()) {
        i.next();
        sortedList << i.value();
    }

    return sortedList;
}

BranchItem *VymModel::createBranchWhileLoading(BranchItem *dst, int insertPos)
{
    BranchItem *newbi;
    if (!dst || dst == rootItem)
        newbi = addMapCenterAtPos(QPointF(0, 0));
    else {
        if (insertPos < 0)
            newbi = addNewBranchInt(dst, -2);
        else
            newbi = addNewBranchInt(dst, insertPos);
    }

    // Set default design styles, e.g. font
    applyDesign(MapDesign::LoadingMap, newbi);
    return newbi;
}

ImageItem *VymModel::createImage(BranchItem *dst)
{
    if (dst) {
        QModelIndex parix;
        int n;

        ImageItem *newii = new ImageItem();

        emit layoutAboutToBeChanged();

        parix = index(dst);
        if (!parix.isValid())
            qDebug() << "VM::createII invalid index\n";
        n = dst->getRowNumAppend(newii);
        beginInsertRows(parix, n, n);
        dst->appendChild(newii);
        endInsertRows();

        emit layoutChanged();

        dst->addToImagesContainer(newii->createImageContainer());

        latestAddedItemUuid = newii->getUuid();
        reposition();
        return newii;
    }
    return nullptr;
}

bool VymModel::createXLink(XLink *xlink)
{
    BranchItem *begin = xlink->getBeginBranch();
    BranchItem *end = xlink->getEndBranch();

    if (!begin || !end) {
        qWarning() << "VM::createXLinkNew part of XLink is nullptr";
        return false;
    }

    if (begin == end) {
        if (debug)
            qDebug() << "VymModel::createXLink begin==end, aborting";
        return false;
    }

    // check, if link already exists
    foreach (XLink *l, xlinks) {
        if ((l->getBeginBranch() == begin && l->getEndBranch() == end) ||
            (l->getBeginBranch() == end && l->getEndBranch() == begin)) {
            qWarning() << "VymModel::createXLink link exists already, aborting";
            return false;
        }
    }

    QModelIndex parix;
    int n;

    XLinkItem *newli = new XLinkItem();
    newli->setXLink(xlink);
    xlink->setBeginXLinkItem(newli);

    emit layoutAboutToBeChanged();

    parix = index(begin);
    n = begin->getRowNumAppend(newli);
    beginInsertRows(parix, n, n);
    begin->appendChild(newli);
    endInsertRows();

    newli = new XLinkItem();
    newli->setXLink(xlink);
    xlink->setEndXLinkItem(newli);

    parix = index(end);
    n = end->getRowNumAppend(newli);
    beginInsertRows(parix, n, n);
    end->appendChild(newli);
    endInsertRows();

    emit layoutChanged();

    xlinks.append(xlink);
    xlink->activate();

    latestAddedItemUuid = newli->getUuid();

    if (!xlink->getXLinkObj()) {
        xlink->createXLinkObj();
        reposition();
    }
    else
        xlink->updateXLink();

    xlink->setStyleBegin(mapDesignInt->defXLinkStyleBegin());
    xlink->setStyleEnd(mapDesignInt->defXLinkStyleEnd());

    QString xv = setXLinkVar(xlink, "xl");
    QString uc = xv + "map.removeXLink(xl);";
    QString rc = QString("map.loadDataInsert(\"REDO_PATH\");");
    QString com = QString("Add XLink from \"%1\" to \"%2\"")
        .arg(getObjectName(begin), getObjectName(end));
    saveState(uc, rc, com, nullptr, xlink->beginXLinkItem());

    return true;
}

QColor VymModel::getXLinkColor()
{
    XLink *xl = getSelectedXLink();
    if (xl)
        return xl->getPen().color();
    else
        return QColor();
}

int VymModel::getXLinkWidth()
{
    XLink *xl = getSelectedXLink();
    if (xl)
        return xl->getPen().width();
    else
        return -1;
}

Qt::PenStyle VymModel::getXLinkStyle()
{
    XLink *xl = getSelectedXLink();
    if (xl)
        return xl->getPen().style();
    else
        return Qt::NoPen;
}

QString VymModel::getXLinkStyleBegin()
{
    XLink *xl = getSelectedXLink();
    if (xl)
        return xl->getStyleBeginString();
    else
        return QString();
}

QString VymModel::getXLinkStyleEnd()
{
    XLink *xl = getSelectedXLink();
    if (xl)
        return xl->getStyleEndString();
    else
        return QString();
}
AttributeItem *VymModel::setAttribute( // FIXME-3 saveState( missing. For bulk changes like Jira maybe save whole branch use Script...
        BranchItem *dst,
        const QString &key,
        const QVariant &value,
        bool removeIfEmpty)
{
    if (!dst) return nullptr;

    bool keyFound = false;
    AttributeItem *ai;

    logAction("", "Set attribute " + key + " of " + headingText(dst) + " to '" + value.toString() + "'", __func__);

    for (int i = 0; i < dst->attributeCount(); i++) {
        // Check if there is already an attribute with same key
        ai = dst->getAttributeNum(i);
        if (ai->key() == key)
        {
            keyFound = true;
            if (value.toString().isEmpty() && removeIfEmpty) {
                // Remove attribute
                deleteAttribute(dst, key);
                ai = nullptr;
            } else {
                // Set new value (if required)
                if (value != ai->value())
                    ai->setValue(value);
            }
            break;
        }
    }
    if (!keyFound) {
        // Create new attribute
        ai = new AttributeItem(key, value);

        emit layoutAboutToBeChanged();

        QModelIndex parix = index(dst);
        int n = dst->getRowNumAppend(ai);
        beginInsertRows(parix, n, n);
        dst->appendChild(ai);
        endInsertRows();
        emit layoutChanged();

        // Special case: Jira attributes
        if (ai->key() == "Jira.issueUrl") {
            dst->setUrlType(TreeItem::JiraUrl);
            updateJiraFlag(dst);
        }
    }

    emitDataChanged(dst);
    reposition();

    return ai;
}

void VymModel::deleteAttribute(BranchItem *dst, const QString &key)
{
    AttributeItem *ai;

    for (int i = 0; i < dst->attributeCount(); i++) {
        // Check if there is already an attribute with same key
        ai = dst->getAttributeNum(i);
        if (ai->key() == key)
        {
            logAction("", "Delete attribute '" + key + "' of " + headingText(dst), __func__);

            // Key exists, delete attribute
            deleteItem(ai);
            break;
        }
    }
}

void VymModel::deleteAttributesKeyStartingWith(BranchItem *dst, const QString &key_start)
{
    AttributeItem *ai;

    if (!dst) return;

    logAction("", "Delete attribute starting with '" + key_start + "' of " + headingText(dst), __func__);
    for (int i = 0; i < dst->attributeCount(); i++) {
        // Check if there is already an attribute with same key
        ai = dst->getAttributeNum(i);
        if (ai->key().startsWith(key_start))
        {
            // Key exists, delete attribute
            deleteItem(ai);
            break;
        }
    }
}

AttributeItem *VymModel::getAttributeByKey(const QString &key, TreeItem *ti)
{
    TreeItem *selti = getSelectedItem(ti);
    if (selti) {
        for (int i = 0; i < selti->attributeCount(); i++) {
            AttributeItem *ai = selti->getAttributeNum(i);
            if (ai->key() == key)
                return ai;
        }
    }
    return nullptr;
}

BranchItem *VymModel::addMapCenter(bool interactive)
{
    if (interactive) {
        // Start to build undo/redo scripts
        // These script will be finished later when setHeading() is called
        if (hasContextPos)
            saveStateBeginScript(   // addMC
                    QString("Add new MapCenter at (%1)").arg(toS(contextPos)));
        else
            saveStateBeginScript("Add new MapCenter");
    }

    if (!hasContextPos) {
        // E.g. when called via keypresss:
        // Place new MCO in middle of existing ones,
        // Useful for "brainstorming" mode...
        contextPos = QPointF();
        BranchItem *bi;
        BranchContainer *bc;
        for (int i = 0; i < rootItem->branchCount(); ++i) {
            bi = rootItem->getBranchNum(i);
            bc = bi->getBranchContainer();
            if (bc)
                contextPos += bc->pos();
        }
        if (rootItem->branchCount() > 1)
            contextPos *= 1 / (qreal)(rootItem->branchCount());
    }

    BranchItem *newbi = addMapCenterAtPos(contextPos, interactive);

    if (interactive && mapEditor)
        mapEditor->editHeading(newbi);

    emitShowSelection();

    emitUpdateLayout();
    return newbi;
}

BranchItem *VymModel::addMapCenterAtPos(QPointF absPos, bool interactive)
// createMapCenter could then probably be merged with createBranch
{
    Q_UNUSED(interactive);

    // Create TreeItem
    QModelIndex parix = index(rootItem);

    BranchItem *newbi = new BranchItem(rootItem);
    int n = rootItem->getRowNumAppend(newbi);

    emit layoutAboutToBeChanged();
    beginInsertRows(parix, n, n);

    rootItem->appendChild(newbi);

    endInsertRows();
    emit layoutChanged();

    // Create BranchContainer
    BranchContainer *bc = newbi->createBranchContainer(getScene());
    if (bc) {
        bc->setPos(absPos);

        if (!saveStateBlocked) {
            // Don't apply design while loading map
            applyDesign(MapDesign::CreatedByUser, newbi);

            QString uc, rc, com;
            com = QString("Add new MapCenter at (%1)").arg(toS(absPos));
            uc = setBranchVar(newbi) + "map.removeBranch(b);";
            rc = setBranchVar(rootItem) + QString(" b.loadBranchInsert(\"REDO_PATH\", %1);").arg(newbi->num());
            logAction( rc, com, __func__);
            saveState( uc, rc, com, nullptr, newbi);

        }
    }

    reposition();
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
    BranchItem *parbi = dst;
    int n;
    BranchItem *newbi = new BranchItem;

    emit layoutAboutToBeChanged();

    if (pos == -2) {
        n = parbi->getRowNumAppend(newbi);
        beginInsertRows(index(parbi), n, n);
        parbi->appendChild(newbi);
        endInsertRows();
    }
    else if (pos == -1 || pos == -3) {
        // insert below selection
        parbi = dst->parentBranch();

        n = dst->row() + (3 + pos) / 2; //-1 |-> 1;-3 |-> 0
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
    emit layoutChanged();

    // Create Container
    newbi->createBranchContainer(getScene());

    // Update parent item and stacking order of container to match order in model
    newbi->updateContainerStackingOrder();

    // Reposition now for correct position of e.g. LineEdit later and upLink
    reposition();

    return newbi;
}

BranchItem *VymModel::addNewBranch(BranchItem *bi, int pos, bool interactive)
{
    BranchItem *selbi = getSelectedBranch(bi);
    if (!selbi)
        return nullptr;

    QString comment;
    if (interactive) {
        // saveStateEndScript will be called in VymModel::setHeading()
        if (pos == -2)
            comment = QString("Add new branch to %1").arg(getObjectName(selbi));
        else if (pos < -2)
            comment = QString("Add new branch above %1").arg(getObjectName(selbi));
        else
            comment = QString("Add new branch below %1").arg(getObjectName(selbi));

        logAction("", comment, __func__);
        saveStateBeginScript(comment);  // addNewBranch: edit new heading if interactive
    }

    BranchItem *newbi = addNewBranchInt(selbi, pos);

    // Required to initialize styles
    if (!saveStateBlocked)
        // Don't apply design while loading map
        applyDesign(MapDesign::CreatedByUser, newbi);

    if (newbi) {
        QString uc, rc;
        BranchItem *pbi;
        if (pos == -2)
            pbi = selbi;
        else
            pbi = selbi->parentBranch();

        uc = setBranchVar(newbi) + "map.removeBranch(b);";
        rc = setBranchVar(pbi) + QString(" b.loadBranchInsert(\"REDO_PATH\", %1);").arg(newbi->num());
        saveState( uc, rc, "", nullptr, newbi);

        latestAddedItemUuid = newbi->getUuid();
        // In Network mode, the client needs to know where the new branch
        // is, so we have to pass on this information via saveState.
        // TODO: Get rid of this positioning workaround
        /* FIXME  network problem:  QString ps=toS
           (newbo->getAbsPos()); sendData ("selectLatestAdded ()"); sendData
           (QString("move %1").arg(ps)); sendSelection();
           */
    }

    if (interactive && mapEditor) {
        select(newbi);
        mapEditor->editHeading();
    }

    return newbi;
}

BranchItem *VymModel::addNewBranchBefore(BranchItem *bi, bool interactive)    // FIXME-3 Use position of selbi for newbi, if floating
{
    BranchItem *newbi = nullptr;
    BranchItem *selbi = getSelectedBranch(bi);
    if (selbi && selbi->depth() > 0)
    // We accept no MapCenter here, so we _have_ a parent
    {
        QString comment;
        if (interactive) {
            // saveStateEndScript will be called in VymModel::setHeading()
            comment = QString("Add new branch before %1").arg(getObjectName(selbi));
            logAction("", comment, __func__);
            saveStateBeginScript(comment); // addNewBranchBefore (incl. relinking)
        }

        // add below selection
        newbi = addNewBranchInt(selbi, -1);

        if (newbi) {
            // Required to initialize styles
            if (!saveStateBlocked)
                // Don't apply design while loading map
                applyDesign(MapDesign::CreatedByUser, newbi);

            // newbi->move2RelPos (p);

            // Move selection to new branch
            // relink() would create another saveStateScript, block saveState for now
            bool saveStateBlockedOrg = saveStateBlocked;
            saveStateBlocked = true;
            relinkBranch(selbi, newbi, 0);
            saveStateBlocked = saveStateBlockedOrg;

            QString uc = setBranchVar(newbi) + "map.removeKeepChildren(b);";
            QString rc = setBranchVar(selbi) + "map.loadBranchReplace(\"REDO_PATH\",b);";
            saveState(uc, rc, "", nullptr, newbi);

            emitDataChanged(newbi);
        }

        if (interactive && mapEditor) {
            select(newbi);
            mapEditor->editHeading();
        }
    }

    return newbi;
}

bool VymModel::relinkBranch(BranchItem *branch, BranchItem *dst, int num_dst)
{
    if (!branch)
        return false;

    QList <BranchItem*> branches = {branch};
    return relinkBranches(branches, dst, num_dst);
}

bool VymModel::relinkBranches(QList <BranchItem*> branches, BranchItem *dst, int num_dst)   
{
    // qDebug() << "VM::relink " << branches.count() << " branches to " << headingText(dst) << "num_dst=" << num_dst;

    // Selection is lost when removing rows from model
    QList <TreeItem*> selectedItems = getSelectedItems();

    if (branches.isEmpty())
        branches = getSelectedBranches();

    if (!dst || branches.isEmpty())
        return false;

    if (num_dst < 0 || num_dst >= dst->branchCount())
        num_dst = dst->branchCount();

    if (!saveStateBlocked)
        // When ordering branches, we already saveState there and not for 
        // each branch individually
        saveStateBeginScript(   // relinkBranches: also save position if required
            QString("Relink %1 objects to \"%2\"")
                .arg(branches.count())
                .arg(dst->headingPlain()));

    foreach (BranchItem *bi, branches) {
        logAction("", QString("Relink %1 branches to %2").arg(branches.count()).arg(headingText(dst)), __func__);
        // Check if we link to ourself
        if (dst == bi) {
            logWarning("Attempting to relink to myself: " + bi->headingPlain(), __func__);
            return false;
        }

        // Check if we relink down to own children
        if (dst->isChildOf(bi)) {
            logWarning("Attempting to relink to my own children", __func__);
            return false;
        }

        // Save old selection for savestate
        QString preNumString = QString::number(bi->num(), 10);
        QString preParUidString = bi->parent()->getUuid().toString();

        // Remember original position for saveState
        bool rememberPos = false;
        BranchItem *pbi = bi->parentBranch();
        if (pbi == rootItem)
        {
            // Remember position before MapCenter is relinked
            rememberPos = true;
        } else {
            BranchContainer *pbc = pbi->getBranchContainer();
            if (pbc->hasFloatingBranchesLayout())
                // Remember position, if relinked branch is floating
                rememberPos = true;
        }

        // Prepare BranchContainers
        BranchContainer *bc = bi->getBranchContainer();
        BranchContainer *dstBC = dst->getBranchContainer(); // might be nullptr for MC!

        // Keep position when detaching
        bool detaching;
        QPointF preDetachPos;
        if (dst == rootItem) {
            detaching = true;
            preDetachPos = bc->getHeadingContainer()->scenePos();
        } else {
            // Save position, so // that it can be used // for undo command // later
            detaching = false; bc->setOriginalPos();
        }

        BranchItem *branchpi = bi->parentBranch();

        // Remove at current position
        int removeRowNum = bi->row();
        int dstRowNum = num_dst + dst->branchOffset();

        QModelIndex pix = index(branchpi);
        /* FIXME-4 remove debug stuff
        std::cout << "  VM::relink removing " << bi << " " << bi->headingPlain().toStdString()
                  << " at n=" << removeRowNum
                  << " from " << branchpi << "  " << branchpi->headingPlain().toStdString()
                  << " to " << dst << " " << dst->headingPlain().toStdString() << endl;
        std::cout << "      persIxList: " << (this->persistentIndexList()).count() << endl;
        std::cout << "         num_dst: " << num_dst << endl;
        std::cout << "    removeRowNum: " << removeRowNum << endl;
        std::cout << "       dstRowNum: " << dstRowNum << endl;
        */

        QModelIndex dix = index(dst);
        if (branchpi == dst && dstRowNum > removeRowNum) {
            // When moving down with same parent:
            // Be careful to insert *before* destination index using beginMoveRows
            // https://doc.qt.io/qt-6/qabstractitemmodel.html#moveRows
            dstRowNum = dstRowNum + 1;
            if (num_dst > bi->num() + 1)
                // if not just moving down one level but further, adapt num_dst
                num_dst--;
        }

        emit layoutAboutToBeChanged();
        bool b = beginMoveRows(pix, removeRowNum, removeRowNum, dix, dstRowNum);
        Q_ASSERT(b);
        /*
        std::cout << "beginMoveRows=" << toS(b).toStdString()
                  << " removeRowNum=" << removeRowNum
                  << "      num_dst=" << num_dst
                  << "    dstRowNum=" << dstRowNum << endl;
        */
        branchpi->removeChild(removeRowNum);
        dst->insertBranch(num_dst, bi);
        endMoveRows();
        emit layoutChanged();

        // Update upLink of BranchContainer to *parent* BC of destination
        bc->linkTo(dstBC);

        // Update parent item and stacking order of container
        bi->updateContainerStackingOrder();

        // reset parObj, fonts, frame, etc in related branch-container or other view-objects
        applyDesign(MapDesign::RelinkedByUser, bi);

        // Keep position when detaching
        if (detaching)
            bc->setPos(preDetachPos);

        // Savestate, but not if just moving up/down
        if (!saveStateBlocked) {
            QString uc, rc;

            if (rememberPos) {
                // For undo move back to original position in old floating layout
                uc = QString("setPos %1;").arg(toS(bc->getOriginalPos()));
                rc = "";

                saveStateBranch( bi, uc, rc, QString("Move %1") .arg(headingText(bi)));
            }

            QString postNumString = QString::number(bi->num(), 10);

            QString bv = setBranchVar(bi);
            if (pbi == rootItem)
                uc = bv + " detach ()";
            else {
                uc = bv + QString(" dst = map.findBranchById(\"%1\");").arg(preParUidString);
                uc += QString(" b.relinkToBranchAt (dst, \"%1\");").arg(preNumString);
            }
            rc = bv + QString(" dst = map.findBranchById(\"%1\");").arg(dst->getUuid().toString());
            rc += QString(" b.relinkToBranchAt (dst, \"%1\");").arg(postNumString);

            saveState(uc, rc,
                      QString("Relink %1 to %2")
                          .arg(getObjectName(bi), getObjectName(dst)));

            if (dstBC && dstBC->hasFloatingBranchesLayout()) {
                // Save current position for redo
                saveStateBranch(bi, "", 
                          QString("setPos %1;").arg(toS(bc->pos())),
                          QString("Move %1")
                              .arg(getObjectName(bi)));
            }
        } // saveState not blocked
    }   // Iterating over selbis    

    reposition();

    if (!saveStateBlocked)
        saveStateEndScript();

    // Restore selection, which was lost when removing rows
    select(selectedItems);

    return true;
}

bool VymModel::relinkImage(ImageItem* image, TreeItem *dst_ti, int num_new) {
    if (!image)
        return false;

    QList <ImageItem*> images = {image};
    return relinkImages(images, dst_ti, num_new);
}

bool VymModel::relinkImages(QList <ImageItem*> images, TreeItem *dst_ti, int num_new)
{
    // Selection is lost when removing rows from model
    QList <TreeItem*> selectedItems = getSelectedItems();

    BranchItem *dst;

    if (images.isEmpty())
        images = getSelectedImages();

    if (!dst_ti || images.isEmpty())
        return false;

    if (dst_ti->hasTypeImage()) {
        // Allow dropping on images,
        // append at this num in parentBranch then:
        dst = ((ImageItem*)dst_ti)->parentBranch();
        if (!dst || dst == rootItem)
            return false;
        num_new = dst_ti->num();
    } else if (dst_ti->hasTypeBranch()) {
        dst = (BranchItem*)dst_ti;
    } else
        return false;

    if (num_new < 0 || num_new >= dst->imageCount())
        num_new = dst->imageCount();

    if (!saveStateBlocked)
        // When ordering branches, we already saveState there and not for 
        // each branch individually
        saveStateBeginScript(   // FIXME-3 relink images: script used for multiselection
            QString("Relink %1 objects to \"%2\"")
                .arg(images.count())
                .arg(dst->headingPlain()));

    foreach(ImageItem *ii, images) {
        logAction("", QString("Relink %1 images to %2").arg(images.count()).arg(headingText(dst)), __func__);
        emit layoutAboutToBeChanged();

        BranchItem *pi = (BranchItem *)(ii->parent());
        // Remove at current position
        int n = ii->row();
        beginRemoveRows(index(pi), n, n);
        pi->removeChild(n);
        endRemoveRows();

        // Insert again
        int insertRowNum;
        if (dst->imageCount() == 0)
            // Append as last image to dst
            insertRowNum = 0;
        else
            insertRowNum = dst->getFirstImage()->row() + num_new;
        QModelIndex dstix = index(dst);
        n = dst->getRowNumAppend(ii);

        beginInsertRows(dstix, insertRowNum + num_new, insertRowNum + num_new);
        dst->insertImage(num_new, ii);
        endInsertRows();

        emit layoutChanged();

        ii->updateContainerStackingOrder();
        // FIXME-3 relinkImages issues:
        // - What about updating links of images (later)?
        // - What about updating design (later)?
        // - in ImageWrapper: num_new missing
        // - does not save positions currently

        QString iv = setImageVar(ii);
        QString uc = setBranchVar(pi) + iv + "i.relinkToBranch(b);";
        QString rc = setBranchVar(dst) + iv + "i.relinkToBranch(b);";
        QString com = QString("Relink image to %1").arg(getObjectName(dst));

        saveState(uc, rc, com);
    }

    reposition();

    if (!saveStateBlocked)
        saveStateEndScript();

    // Restore selection, which was lost when removing rows
    select(selectedItems);

    return true;
}

bool VymModel::relinkTo(const QString &dstString, int num)
{
    TreeItem *selti = getSelectedItem();
    if (!selti)
        return false; // Nothing selected to relink

    TreeItem *dst = findBySelectString(dstString);
    if (!dst)
        return false; // Could not find destination

    if (!dst->hasTypeBranch())
        return false; // Relinking only allowed to branchLike destinations

    if (selti->hasTypeBranch()) {
        BranchItem *selbi = (BranchItem *)selti;

        if (relinkBranch(selbi, (BranchItem *)dst, num))
            return true;

    } else if (selti->hasTypeImage()) {
        if (relinkImage(((ImageItem *)selti), (BranchItem *)dst))
            return true;
    }
    return false; // Relinking failed
}

void VymModel::deleteSelection(ulong selID)
{
    QList<ulong> selectedIDs;
    if (selID > 0)
        selectedIDs << selID;
    else
        selectedIDs = getSelectedIDs();

    unselectAll();

    if (mapEditor)
        mapEditor->stopContainerAnimations();  // FIXME-5 better tell ME about deleted items, so that ME can take care of race conditions, e.g. also deleting while moving objects

    foreach (ulong id, selectedIDs) {
        TreeItem *ti = findID(id);
        BranchItem *pbi;
        if (ti) {
            pbi = (BranchItem*)(ti->parent());
            if (pbi && !pbi->hasTypeBranch())
                pbi = nullptr;

            if (ti->hasTypeBranch()) { // Delete branch
                BranchItem *bi = (BranchItem *)ti;
                BranchItem *pbi = bi->parentBranch();
                QString bv = setBranchVar(bi);
                QString pbv = setBranchVar(pbi, "pb");
                QString uc = pbv + QString("pb.loadBranchInsert(\"UNDO_PATH\", %1)").arg(bi->num());
                QString rc = bv + "map.removeBranch(b);";
                QString com = QString("Remove branch \"%1\"").arg(bi->headingText());

                logAction(rc, com, __func__);

                saveState(uc, rc, com, bi, nullptr);

                BranchItem *pi = (BranchItem *)(deleteItem(bi));
                if (pi) {
                    if (pi->isScrolled() && pi->branchCount() == 0)
                        pi->unScroll();
                    emitDataChanged(pi);
                    select(pi);
                }
                else
                    emitDataChanged(rootItem);
                ti = nullptr;
            } else if (ti->getType() == TreeItem::Image) {
                QString iv = setImageVar((ImageItem*)ti);
                QString bv = setBranchVar(pbi);
                QString uc = bv + QString("map.loadBranchReplace(\"UNDO_PATH\", b);");
                QString rc = iv + QString("map.removeImage(i);");
                QString com = QString("Remove image \"%1\" from branch \"%2\"").arg(getObjectName(ti), getObjectName(pbi));

                logAction(rc, com, __func__);

                saveState(uc, rc, com, pbi);

                deleteItem(ti);
                emitDataChanged(pbi);
                select(pbi);
            } else if (ti->getType() == TreeItem::XLinkItemType) {
                deleteXLink(((XLinkItem*)ti)->getXLink());
            } else if (ti->getType() == AttributeItem::Attribute && pbi) {
                AttributeItem *ai = (AttributeItem*)ti;
                QString av = setAttributeVar((AttributeItem*)ti);
                QString bv = setBranchVar(pbi);
                QString uc = bv + QString("b.setAttribute(\"%1\",\"%2\");").arg(ai->key(), ai->value().toString());
                QString rc = QString("map.removeImage(i);");
                QString com = QString("Remove image \"%1\" from branch \"%2\"").arg(getObjectName(ti), getObjectName(pbi));

                logAction(rc, com, __func__);

                saveState(uc, rc, com, pbi);
                deleteItem(ti);
            } else
                qWarning("VymmModel::deleteSelection()  unknown type?!");
        } // ti found
    } // Loop over selectedIDs

    emptyXLinksTrash();

    reposition();
}

void VymModel::deleteKeepChildren(BranchItem *bi)   // FIXME-3 does not work really with MapCenters. Floating positions also not saved for undo
{
    QList<BranchItem *> selbis = getSelectedBranches(bi);
    unselectAll();

    foreach (BranchItem *selbi, selbis) {
        if (selbi->depth() < 1) {
            while (selbi->branchCount() > 0)
                detach(selbi->getBranchNum(0));

            deleteSelection(selbi->getID());
        } else {
            // Check if we have children at all to keep
            if (selbi->branchCount() == 0)
                deleteSelection(selbi->getID());
            else {

                BranchItem *pi = (BranchItem *)(selbi->parent());

                QString pbv = setBranchVar(pi, "pb");
                QString bv = setBranchVar(selbi);
                QString uc = pbv + "map.loadBranchReplace(\"UNDO_PATH\", pb);";
                QString rc = bv + "map.removeKeepChildren(b);";
                QString com = QString("Remove branch \"%1\" and keep children").arg(selbi->headingText());
                logAction(rc, com, __func__);
                saveState(uc, rc, com, pi);

                bool oldSaveState = saveStateBlocked;
                saveStateBlocked = true;

                QString sel = getSelectString(selbi);
                int num_dst = selbi->num();
                BranchItem *bi = selbi->getFirstBranch();
                while (bi) {
                    relinkBranch(bi, pi, num_dst);
                    bi = selbi->getFirstBranch();
                    num_dst++;
                }
                saveStateBlocked = oldSaveState;

                deleteItem(selbi);
                reposition();

                // Select the "new" branch  // FIXME-4 not really working with multiple selected branches...
                select(sel);
            }
        }
    }

    emptyXLinksTrash();

}

void VymModel::deleteChildren(BranchItem *bi)
{
    QList<BranchItem *> selbis = getSelectedBranches(bi);
    foreach (BranchItem *selbi, selbis) {
        QString bv = setBranchVar(selbi);
        QString uc = bv + "map.loadBranchReplace(\"UNDO_PATH\", b);";
        QString rc = bv + "b.removeChildren();";
        QString com = QString("Remove children of \"%1\"").arg(selbi->headingText());
        logAction(rc, com, __func__);
        saveState(uc, rc, com, selbi);
        emit layoutAboutToBeChanged();

        QModelIndex ix = index(selbi);
        int n = selbi->childCount() - 1;
        beginRemoveRows(ix, 0, n);
        removeRows(0, n + 1, ix);
        endRemoveRows();
        if (selbi->isScrolled()) unscrollBranch(selbi);

        updateJiraFlag(selbi);
        emit layoutChanged();

        emitDataChanged(selbi);
        reposition();
    }
    emptyXLinksTrash();
}

void VymModel::deleteChildrenBranches(BranchItem *bi)
{
    QList<BranchItem *> selbis = getSelectedBranches(bi);
    foreach (BranchItem *selbi, selbis) {
        if (selbi->branchCount() > 0) {
            int n_first = selbi->getFirstBranch()->row();
            int n_last  = selbi->getLastBranch()->row();

            QString bv = setBranchVar(selbi);
            QString uc = bv + "map.loadBranchReplace(\"UNDO_PATH\", b);";
            QString rc = bv + "b.removeChildrenBranches();";
            QString com = QString("Remove children branches of \"%1\"").arg(selbi->headingText());
            logAction(rc, com, __func__);
            saveState(uc, rc, com, selbi);

            emit layoutAboutToBeChanged();

            QModelIndex ix = index(selbi);
            beginRemoveRows(ix, n_first, n_last);
            if (!removeRows(n_first, n_last - n_first + 1, ix))
                qWarning() << "VymModel::deleteChildBranches removeRows() failed";
            endRemoveRows();
            if (selbi->isScrolled()) unscrollBranch(selbi);

            emit layoutChanged();
        }
    }

    if (selbis.count() > 0)  {
        emptyXLinksTrash();
        reposition();
    }
}

TreeItem *VymModel::deleteItem(TreeItem *ti)
{
    if (ti) {
        TreeItem *pi = ti->parent();

        bool wasAttribute = ti->hasTypeAttribute();
        TreeItem *parentItem = ti->parent();

        QModelIndex parentIndex = index(pi);

        emit layoutAboutToBeChanged();

        int n = ti->row();
        beginRemoveRows(parentIndex, n, n);
        bool r = removeRows(n, 1, parentIndex);  // Deletes object!
        if (!r)
            qWarning() << "removeRows failed in " << __func__;
        endRemoveRows();

        emit layoutChanged();

        emitUpdateQueries();

        if (wasAttribute)
            updateJiraFlag(parentItem);

        emitDataChanged(parentItem);
        // reposition() is triggered in calling functions!

        if (pi->depth() >= 0)
            return pi;
    }
    return nullptr;
}

void VymModel::deleteXLink(XLink *xlink)
{
    //qDebug() << "VM::deleteXLink start";


    QString xv = setXLinkVar(xlink);
    QString uc = QString("map.loadDataInsert(\"UNDO_PATH\");");
    QString rc = xv + QString("map.removeXLink(x);");

    QString com = QString("Remove XLink");
    
    logAction(rc, com, __func__);

    saveState(uc, rc, com, xlink->beginXLinkItem());

    deleteXLinkInt(xlink);
}

void VymModel::deleteXLinkLater(XLink *xlink)
{
    //qDebug() << "VM::deleteXLinkLater: " << xlink;

    if (!xlinksTrash.contains(xlink)) {
        xlinksTrash << xlink;
        xlinks.removeOne(xlink);
    }
}

void VymModel::emptyXLinksTrash()
{
    //qDebug() << "*** VM::emptyXLinksTrash " << xlinksTrash;

    foreach (XLink *xlink, xlinksTrash)
        deleteXLinkInt(xlink);

    xlinksTrash.clear();
}

void VymModel::deleteXLinkInt(XLink *xlink)
{
    //qDebug() << __func__ << xlink;

    if (!xlink) {
        qWarning() << __func__ << "No xlink ?!";
        return;
    }

    // Remove XLinkItems from TreeModel
    XLinkItem *xli;
    xli = xlink->beginXLinkItem();
    if (xli) {
        //qDebug() << "  Deleting begin xli" << xli;
        xli->setXLink(nullptr);
        deleteItem(xli);
    }
    xli = xlink->endXLinkItem();
    if (xli) {
        //qDebug() << "  Deleting end xli" << xli;
        xli->setXLink(nullptr);
        deleteItem(xli);
    }

    // Remove from list of items and delete xlink itself, including XLinkObj
    if (xlinks.removeOne(xlink)) {
        //qDebug() << "  Removing xlink from xlinks";
        delete (xlink);
    }
    if (xlinksTrash.removeOne(xlink)) {
        //qDebug() << "  Removing xlink from xlinksTrash";
        delete (xlink);
    }

    reposition();
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
            QString u, r, c;
            r = setBranchVar(bi) + " b.scroll();";
            u = setBranchVar(bi) + " b.unscroll();";
            c = QString("Scroll %1").arg(getObjectName(bi));
            logAction(r, c, __func__);
            saveState(u, r, c);
            emitDataChanged(bi);
            reposition();
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
            QString u, r, c;
            u = setBranchVar(bi) + " b.scroll();";
            r = setBranchVar(bi) + " b.unscroll();";
            c = QString("Uncroll %1").arg(getObjectName(bi));
            logAction(r, c, __func__);
            saveState(u, r, c);
            emitDataChanged(bi);

            reposition();
            return true;
        }
    }
    return false;
}

void VymModel::toggleScroll(BranchItem *bi)
{
    QList<BranchItem *> selbis = getSelectedBranches(bi);
    foreach (BranchItem *selbi, selbis) {
        if (selbi->isScrolled())
            unscrollBranch(selbi);
        else
            scrollBranch(selbi);
        // Note: saveState & reposition are called in above functions
    }
}

void VymModel::unscrollSubtree(BranchItem *bi)
{
    QList<BranchItem *> selbis = getSelectedBranches(bi);
    foreach (BranchItem *selbi, selbis) {
        QString bv = setBranchVar(selbi);
        QString uc = bv + QString("map.loadBranchReplace(\"UNDO_PATH\", b);");
        QString rc = bv + QString("b.unscrollSubtree();");
        QString comment = QString("Unscroll branch \"%1\" and all its scrolled children").arg(selbi->headingText());
        logAction(rc, comment, __func__);
        saveState(uc, rc, comment, selbi);

        BranchItem *prev = nullptr;
        BranchItem *cur = nullptr;
        nextBranch(cur, prev, true, selbi);
        while (cur) {
            if (cur->isScrolled()) {
                cur->toggleScroll();
                emitDataChanged(cur);
            }
            nextBranch(cur, prev, true, selbi);
        }
    }
    updateActions();
    reposition();
}

void VymModel::emitExpandAll() { emit expandAll(); }

void VymModel::emitExpandOneLevel() { emit expandOneLevel(); }

void VymModel::emitCollapseOneLevel() { emit collapseOneLevel(); }

void VymModel::emitCollapseUnselected() { emit collapseUnselected(); }

void VymModel::toggleTarget(BranchItem *bi)
{
    QList<BranchItem *> selbis = getSelectedBranches(bi);
    foreach (BranchItem *selbi, selbis) {
        selbi->toggleTarget();
        QString uc = QString("toggleTarget(%1);").arg(toS(!selbi->isTarget()));
        QString rc = QString("toggleTarget(%1);").arg(toS(selbi->isTarget()));
        QString com = "Toggle target flag of branch";
        logAction(rc, com, __func__);
        saveStateBranch(selbi, uc, rc, com);
    }
    reposition();
}

ItemList VymModel::getLinkedMaps()
{
    ItemList targets;

    // rmodel->setSearchString (s);

    BranchItem *cur = nullptr;
    BranchItem *prev = nullptr;
    nextBranch(cur, prev);

    QString s;

    while (cur) {
        if (cur->hasActiveSystemFlag("system-target") && cur->hasVymLink()) {
            s = cur->heading().getTextASCII();
            static QRegularExpression re;
            re.setPattern("\n+");
            s.replace(re, " ");
            re.setPattern("\\s+");
            s.replace(re, " ");
            re.setPattern("^\\s+");
            s.replace(re, "");

            QStringList sl;
            sl << s;
            sl << cur->vymLink();

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

    BranchItem *cur = nullptr;
    BranchItem *prev = nullptr;
    nextBranch(cur, prev);

    QString s;

    QRegularExpression re;
    while (cur) {
        if (cur->hasActiveSystemFlag("system-target")) {
            s = cur->heading().getTextASCII();
            re.setPattern("\n+");
            s.replace(re, " ");
            re.setPattern("\\s+");
            s.replace(re, " ");
            re.setPattern("^\\s+");
            s.replace(re, "");

            QStringList sl;
            sl << s;

            targets[cur->getID()] = sl;
        }
        nextBranch(cur, prev);
    }
    return targets;
}

Flag* VymModel::findFlagByName(const QString &name)
{
    BranchItem *bi = getSelectedBranch();

    if (bi) {
        Flag *f = standardFlagsMaster->findFlagByName(name);
        if (!f) {
            f = userFlagsMaster->findFlagByName(name);
            if (!f) {
                qWarning() << "VymModel::findFlagByName failed for flag named "
                           << name;
                return nullptr;
            }
        }
        return f;
    }

    // Nothing selected, so no flag found
    return nullptr;
}

void VymModel::setFlagByName(const QString &name, BranchItem *bi, bool useGroups)
{
    QList <BranchItem*> selbis = getSelectedBranches(bi);

    foreach (BranchItem* selbi, selbis)
    if (!selbi->hasActiveFlag(name))
        toggleFlagByName(name, selbi, useGroups);
}

void VymModel::unsetFlagByName(const QString &name, BranchItem *bi)
{
    QList <BranchItem*> selbis = getSelectedBranches(bi);

    foreach (BranchItem* selbi, selbis)
        if (selbi->hasActiveFlag(name))
            toggleFlagByName(name, selbi);
}

void VymModel::toggleFlagByUid( const QUuid &uid, BranchItem *bi, bool useGroups)
{
    QList <BranchItem*> selbis = getSelectedBranches(bi);

    foreach (BranchItem* selbi, selbis) {
        // For undo save all currently set flags. (Independely of usage of flag groups)
        QList <QUuid> oldFlags = selbi->activeFlagUids();
        QStringList sl;
        foreach (QUuid id, oldFlags)
            sl << QString("\"%1\"").arg(id.toString());

        QString uc = QString("setOnlyFlags([%1]);").arg(sl.join(","));

        Flag *flag = selbi->toggleFlagByUid(uid, useGroups);
        if (flag) {
            QString fname = flag->getName();
            QString rc = QString("toggleFlagByUid(\"%1\");").arg(uid.toString());
            QString com = QString("Toggle flag %1 of %2").arg(fname, getObjectName(selbi));
            logAction(rc, com, __func__);
            saveStateBranch(selbi, uc, rc, com);
            emitDataChanged(selbi);
        } else
            qWarning() << "VymModel::toggleFlag failed for flag with uid "
                       << uid;
    }
    reposition();
}

void VymModel::toggleFlagByName(const QString &name, BranchItem *bi, bool useGroups)
{
    QList <BranchItem*> selbis = getSelectedBranches(bi);

    foreach (BranchItem* bi, selbis) {
        Flag *flag = standardFlagsMaster->findFlagByName(name);
        if (!flag) {
            flag = userFlagsMaster->findFlagByName(name);
            if (!flag) {
                qWarning() << "VymModel::toggleFlag failed to find flag named "
                           << name;
                return;
            }
        }

        QUuid uid = flag->getUuid();

        toggleFlagByUid(uid, bi, useGroups);
    }
}

void VymModel::setOnlyFlags(QList <QUuid> uids, BranchItem *bi)
{
    if (!bi) {
        qWarning() << __func__ << "bi == nullptr";
        return;
    }

    QList <QUuid> oldUids = bi->activeFlagUids();

    QStringList sl_old;
    QStringList sl_new;
    foreach (QUuid id, oldUids)
        sl_old << QString("\"%1\"").arg(id.toString());
    foreach (QUuid id, uids)
        sl_new << QString("\"%1\"").arg(id.toString());

    QString uc = QString("setOnlyFlags([%1]);").arg(sl_old.join(","));
    QString rc = QString("setOnlyFlags([%1]);").arg(sl_new.join(","));
    QString com = QString("Set only flags %1").arg(getObjectName(bi));
    logAction(rc, com, __func__);

    foreach (QUuid uid, oldUids)
        if (!uids.contains(uid))
            // Unset flag
            bi->toggleFlagByUid(uid);

    foreach (QUuid uid, uids)
        if (!oldUids.contains(uid))
            // Set flag
            bi->toggleFlagByUid(uid);

    saveStateBranch(bi, uc, rc, com);
    emitDataChanged(bi);
    reposition();
}

void VymModel::clearFlags(BranchItem *bi)
{
    QList<BranchItem *> selbis = getSelectedBranches(bi);
    foreach (BranchItem *selbi, selbis) {
        selbi->deactivateAllStandardFlags();
        reposition();
        emitDataChanged(selbi);
        setChanged();
    }
}

void VymModel::colorBranch(QColor c, BranchItem *bi)
{
    QList<BranchItem *> selbis = getSelectedBranches(bi);
    foreach (BranchItem *selbi, selbis) {
        QString uc = QString("colorBranch (\"%1\");")
                      .arg(selbi->headingColor().name());
        QString rc = QString("colorBranch (\"%1\");").arg(c.name());
        QString com = QString("Set color of %1 to %2").arg(getObjectName(selbi), c.name());

        logAction(rc, com, __func__);
        saveStateBranch(selbi, uc, rc, com);
        selbi->setHeadingColor(c); // color branch
        selbi->getBranchContainer()->updateUpLink();
        emitDataChanged(selbi);
        taskEditor->showSelection();
    }
    if (mapEditor)
        mapEditor->getScene()->update();
}

void VymModel::colorSubtree(QColor c, BranchItem *bi)
{
    QList<BranchItem *> selbis = getSelectedBranches(bi);

    foreach (BranchItem *bi, selbis) {
        QString bv = setBranchVar(bi);
        QString uc = bv + "map.loadBranchReplace(\"UNDO_PATH\", b);";
        QString rc = bv + QString("b.colorSubtree (\"%1\")").arg(c.name());
        QString com = QString("Set color of %1 and children to %2").arg(getObjectName(bi), c.name());
        logAction(rc, com, __func__);

        saveState(uc, rc, com, bi);

        BranchItem *prev = nullptr;
        BranchItem *cur = nullptr;
        nextBranch(cur, prev, true, bi);
        while (cur) {
            cur->setHeadingColor(c); // color links, color children
            cur->getBranchContainer()->updateUpLink();
            emitDataChanged(cur);
            nextBranch(cur, prev, true, bi);
        }
    }
    taskEditor->showSelection();
    if (mapEditor)
        mapEditor->getScene()->update();
}

QColor VymModel::getCurrentHeadingColor()
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi)
        return selbi->headingColor();

    QMessageBox::warning(
        0, "Warning",
        "Can't get color of heading,\nthere's no branch selected");
    return Qt::black;
}

void VymModel::note2URLs(BranchItem *bi) // FIXME-3 No saveState yet
{
    BranchItem *selbi = getSelectedBranch(bi);
    if (selbi) {
        /*
        saveStateChangingPart(  // note2Urls
            selbi, selbi, QString("note2URLs()"),
            QString("Extract URLs from note of %1").arg(getObjectName(selbi)));
        */

        QString n = selbi->getNoteASCII();
        if (n.isEmpty())
            return;
        static QRegularExpression re;
        re.setPattern("(http.*)(\\s|\"|')");
        re.setPatternOptions(QRegularExpression::InvertedGreedinessOption);

        BranchItem *bi;
        int pos = 0;
        QRegularExpressionMatch match;
        while (pos >= 0) {
            match = re.match(n, pos);
            if (match.hasMatch()) {
                bi = addNewBranch(selbi);
                bi->setHeadingPlainText(match.captured(1));
                bi->setUrl(match.captured(1));
                emitDataChanged(bi);
                pos = match.capturedEnd();
            } else
                pos = -1;
        }
        reposition();
    }
}

void VymModel::editHeading2URL()
{
    TreeItem *selti = getSelectedItem();
    if (selti)
        setUrl(selti->headingPlain());
}

void VymModel::getJiraData(bool subtree, BranchItem *bi)
// getJiraData FIXME-3 check if jiraClientAvail is set correctly
{
    if (!JiraAgent::available()) {
        WarningDialog dia;
        QString w = QObject::tr("JIRA agent not setup.");
        dia.setText(w);
        dia.setWindowTitle( tr("Warning") + ": " + w);
        dia.setShowAgainName("/JiraAgent/notdefined");
        dia.exec();

        if (!mainWindow->settingsJIRA())
            return;
    }

    BranchItem *selbi = getSelectedBranch(bi);

    if (selbi) {
        BranchItem *prev = nullptr;
        BranchItem *cur = nullptr;
        nextBranch(cur, prev, true, selbi);
        while (cur) {
            QString query = cur->attributeValue("Jira.query").toString();

            bool startAgent = false;
            JiraAgent *agent = new JiraAgent;
            if (!agent->setBranch(cur))
                qWarning () << "Could not set branch in JiraAgent to " << cur->headingPlain();
            else {
                if (!query.isEmpty() && agent->setQuery(query)) {
                    // Branch has a query defined in attribute, try to use that
                    agent->setJobType(JiraAgent::Query);
                    connect(agent, &JiraAgent::jiraQueryReady, this, &VymModel::processJiraJqlQuery);
                    startAgent = true;
                } else {
                    QString key = cur->attributeValue("Jira.key").toString();
                    if (!key.isEmpty() && agent->setTicket(key)) {
                        // Branch has issueKey, get info for ticket
                        startAgent = true;
                        connect(agent, &JiraAgent::jiraTicketReady, this, &VymModel::processJiraTicket);
                    } else {
                        if (agent->setTicket(cur->headingPlain())) {
                            connect(agent, &JiraAgent::jiraTicketReady, this, &VymModel::processJiraTicket);
                            startAgent = true;
                        } else {
                            mainWindow->statusMessage(tr("Could not setup JiraAgent to retrieve data from Jira"));
                        }
                    }
                }
            } // Successfully set branch in agent

            if (startAgent) {
                agent->startJob();
                mainWindow->statusMessage(tr("Contacting Jira...", "VymModel"));
            } else
                delete agent;

            if (subtree)
                nextBranch(cur, prev, true, selbi);
            else
                cur = nullptr;
        }
    }
}

void VymModel::initAttributesFromJiraIssue(BranchItem *bi, const JiraIssue &ji)
{
    if (!bi) {
        qWarning() << __FUNCTION__ << " called without BranchItem";
        return;
    }

    setAttribute(bi, "Jira.assignee", ji.assignee());
    setAttribute(bi, "Jira.components", ji.components());
    //setAttribute(bi, "Jira.description", ji.description());
    bi->setNote(VymText(ji.description()));
    setAttribute(bi, "Jira.fixVersions", ji.fixVersions());
    setAttribute(bi, "Jira.issuetype", ji.issueType());
    setAttribute(bi, "Jira.issueUrl", ji.url());
    setAttribute(bi, "Jira.key", ji.key());
    setAttribute(bi, "Jira.parentKey", ji.parentKey());
    setAttribute(bi, "Jira.status", ji.status());
    setAttribute(bi, "Jira.reporter", ji.reporter());
    setAttribute(bi, "Jira.resolution", ji.resolution());
    setAttribute(bi, "Jira.issueLinks", ji.issueLinks());
    setAttribute(bi, "Jira.subTasks", ji.subTasks());
}

void VymModel::updateJiraFlag(TreeItem *ti)
{
    if(!ti) return;

    AttributeItem *ai = getAttributeByKey("Jira.issueUrl", ti);
    if (ai)
        ti->setUrlType(TreeItem::JiraUrl);
    else {
        ai = getAttributeByKey("Jira.query");
        if (ai)
            ti->setUrlType(TreeItem::JiraUrl);  // FIXME-3 Dedicated flag for query missing
        else {
            if (ti->url().isEmpty())
                ti->setUrlType(TreeItem::NoUrl);
            else
                ti->setUrlType(TreeItem::GeneralUrl);
        }
    }

    emitDataChanged(ti);
}


void VymModel::processJiraTicket(QJsonObject jsobj)
{
    int branchID = jsobj["vymBranchId"].toInt();

    repositionBlocked = true; // FIXME-4 block reposition during processing of Jira query?

    BranchItem *bi = (BranchItem*)findID(branchID);
    if (bi) {
        JiraIssue ji;
        ji.initFromJsonObject(jsobj);
        initAttributesFromJiraIssue(bi, ji);

        QString keyName = ji.key();
        if (ji.isFinished())    {
            keyName = "(" + keyName + ")";
            colorSubtree (vymBlueColor, bi);
        }

        setHeadingPlainText(keyName + ": " + ji.summary(), bi);
        setUrl(ji.url(), false, bi);
        bi->setUrlType(TreeItem::JiraUrl);
        updateJiraFlag(bi);

        emitDataChanged(bi);

        // Pretty print JIRA ticket
        // ji.print();
    }

    repositionBlocked = false;

    mainWindow->statusMessage(tr("Received Jira data.", "VymModel"));

    // Update note
    reselect();

    reposition();
}

void VymModel::processJiraJqlQuery(QJsonObject jsobj)
{
    // Debugging only
    //qDebug() << "VM::processJiraJqlQuery result...";

    int branchId = jsobj.value("vymBranchId").toInt();
    BranchItem *bi = (BranchItem*)findID(branchId);
    if (!bi) {
        mainWindow->statusMessage("VM::processJiraJqlQUery could not find branch with ID=" + jsobj.value("vymBranchId").toString());
        return;
    }
    QJsonArray issues = jsobj["issues"].toArray();

    QString bv = setBranchVar(bi);
    QString uc = bv + QString("map.loadBranchReplace(\"UNDO_PATH\", b);");
    QString rc = bv + QString("b.getJiraData(%1);").arg(toS(jsobj["doSubtree"].toBool()));
    QString comment = QString("Process Jira Jql query on \"%1\"").arg(bi->headingText());
    logAction(rc, comment, __func__);
    saveState(uc, rc, comment, bi->parentBranch());


    saveStateBlocked = true;
    repositionBlocked = true;

    for (int i = 0; i < issues.size(); ++i) {
        QJsonObject issue = issues[i].toObject();
        JiraIssue ji(issue);

        BranchItem *bi2 = addNewBranchInt(bi, -2);
        if (bi2) {
            QString keyName = ji.key();
            if (ji.isFinished())    {
                keyName = "(" + keyName + ")";
                colorSubtree (vymBlueColor, bi2);
            }

            setHeadingPlainText(keyName + ": " + ji.summary(), bi2);
            setUrl(ji.url(), false, bi2);
            initAttributesFromJiraIssue(bi2, ji);
        }

        // Pretty print JIRA ticket
        // ji.print();
    }

    setAttribute(bi, "Jira.query", jsobj["vymJiraLastQuery"].toString());

    saveStateBlocked = false;
    repositionBlocked = false;

    mainWindow->statusMessage(tr("Received Jira data.", "VymModel"));

    reposition();
}

void VymModel::setConfluencePageDetails(bool recursive, BranchItem *bi)
{
    BranchItem *selbi = getSelectedBranch(bi);
    if (selbi) {
        QString url = selbi->url();
        if (!url.isEmpty() &&
                settings.contains("/atlassian/confluence/url") &&
                url.contains(settings.value("/atlassian/confluence/url").toString())) {

            logInfo("Preparing to get info from Confluence", __func__);

            ConfluenceAgent *ca_setHeading = new ConfluenceAgent(selbi);
            ca_setHeading->setPageURL(url);
            if (recursive)
                ca_setHeading->setJobType(ConfluenceAgent::GetPageDetailsRecursively);
            else
                ca_setHeading->setJobType(ConfluenceAgent::GetPageDetails);
            ca_setHeading->startJob();
        }
    }
}

void VymModel::setVymLink(const QString &s, BranchItem *bi)
{
    BranchItem *selbi = getSelectedBranch(bi);
    if (selbi) {
        QString uc = QString("setVymLink(\"%1\");").arg(selbi->vymLink());
        QString rc = QString("setVymLink(\"%1\");").arg(s);
        QString com = QString("Set vymlink of %1 to %2").arg(getObjectName(selbi), s);
        logAction(rc, com, __func__);
        saveStateBranch(selbi, uc, rc, com);
        selbi->setVymLink(s);
        emitDataChanged(selbi);
        reposition();
    }
}

void VymModel::deleteVymLink()
{
    setVymLink("");
}

QString VymModel::getVymLink()
{
    BranchItem *bi = getSelectedBranch();
    if (bi)
        return bi->vymLink();
    else
        return "";
}

QStringList VymModel::getVymLinks()
{
    QStringList links;
    BranchItem *selbi = getSelectedBranch();
    BranchItem *cur = nullptr;
    BranchItem *prev = nullptr;
    nextBranch(cur, prev, true, selbi);
    while (cur) {
        if (cur->hasVymLink())
            links.append(cur->vymLink());
        nextBranch(cur, prev, true, selbi);
    }
    return links;
}

void VymModel::editXLink()
{
    XLink *xlink = getSelectedXLink();
    if (xlink) {
        EditXLinkDialog dia;
        dia.setLink(xlink);
        if (dia.exec() == QDialog::Accepted) {
            if (dia.useSettingsGlobal()) {
                setDefXLinkPen(xlink->getPen());
                setDefXLinkStyleBegin(xlink->getStyleBeginString());
                setDefXLinkStyleEnd(xlink->getStyleEndString());
            }
        }
    }
}

void VymModel::setXLinkColor(const QString &new_col, XLink *xl)
{
    XLink *xlink = getSelectedXLink(xl);
    if (xlink) {
        QPen pen = xlink->getPen();
        QColor new_color = QColor(new_col);
        QColor old_color = pen.color();
        if (new_color == old_color)
            return;
        QString xv = setXLinkVar(xlink, "xl");
        QString uc = xv + QString("xl.setColor(\"%1\");").arg(old_color.name());
        QString rc = xv + QString("xl.setColor(\"%1\");").arg(new_color.name());
        QString com = QString("Set xlink color to \"%1\"").arg(new_color.name());
        logAction(rc, com, __func__);
        saveState(uc, rc, com);

        pen.setColor(new_color);
        xlink->setPen(pen);
    }
}

void VymModel::setXLinkStyle(const QString &new_style, XLink *xl)
{
    XLink *xlink = getSelectedXLink(xl);
    if (xlink) {
        QPen pen = xlink->getPen();
        QString old_style = penStyleToString(pen.style());
        if (new_style == old_style)
            return;
        QString xv = setXLinkVar(xlink, "xl");
        QString uc = xv + QString("xl.setStyle(\"%1\");").arg(old_style);
        QString rc = xv + QString("xl.setStyle(\"%1\");").arg(new_style);
        QString com = QString("Set xlink style to \"%1\"").arg(new_style);
        logAction(rc, com, __func__);
        saveState(uc, rc, com);

        bool ok;
        pen.setStyle(penStyle(new_style, ok));
        xlink->setPen(pen);
    }
}

void VymModel::setXLinkStyleBegin(const QString &new_style, XLink *xl)
{
    XLink *xlink = getSelectedXLink(xl);
    if (xlink) {
        QString old_style = xlink->getStyleBeginString();
        if (new_style == old_style)
            return;
        QString xv = setXLinkVar(xlink, "xl");
        QString uc = xv + QString("xl.setStyleBegin(\"%1\");").arg(old_style);
        QString rc = xv + QString("xl.setStyleBegin(\"%1\");").arg(new_style);
        QString com = QString("Set xlink begin style to \"%1\"").arg(new_style);
        logAction(rc, com, __func__);
        saveState(uc, rc, com);
        xlink->setStyleBegin(new_style);
    }
}

void VymModel::setXLinkStyleEnd(const QString &new_style, XLink *xl)
{
    XLink *xlink = getSelectedXLink(xl);
    if (xlink) {
        QString old_style = xlink->getStyleEndString();
        if (new_style == old_style)
            return;

        QString xv = setXLinkVar(xlink, "xl");
        QString uc = xv + QString("xl.setStyleEnd(\"%1\");").arg(old_style);
        QString rc = xv + QString("xl.setStyleEnd(\"%1\");").arg(new_style);
        QString com = QString("Set xlink end style to \"%1\"").arg(new_style);
        logAction(rc, com, __func__);
        saveState(uc, rc, com);
        xlink->setStyleEnd(new_style);
    }
}

void VymModel::setXLinkWidth(int new_width, XLink *xl)
{
    XLink *xlink = getSelectedXLink(xl);
    if (xlink) {
        QPen pen = xlink->getPen();
        int old_width = pen.width();
        if (new_width == old_width)
            return;
        QString xv = setXLinkVar(xlink, "xl");
        QString uc = xv + QString("xl.setWidth(%1);").arg(old_width);
        QString rc = xv + QString("xl.setWidth(%1);").arg(new_width);
        QString com = QString("Add xlink width to \"%1\"").arg(new_width);
        logAction(rc, com, __func__);
        saveState(uc, rc, com);

        pen.setWidth(new_width);
        xlink->setPen(pen);
    }
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
    if (!mapEditor) {
        qWarning() << __func__ << "mapEditor == nullptr";
        return QPointF();
    }

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
    if (!img.save(fname, format.toLocal8Bit())) {
        QMessageBox::critical(
            0, tr("Critical Error"),
            tr("Couldn't save QImage %1 in format %2").arg(fname, format));
        ex.setResult(ExportBase::Failed);
    } else
        ex.setResult(ExportBase::Success);

    setExportMode(false);

    ex.completeExport();

    return offset;
}

void VymModel::exportPDF(QString fname, bool askName)
{
    if (!mapEditor) {
        qWarning() << __func__ << "mapEditor == nullptr";
        return;
    }
    
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

    ex.setResult(ExportBase::Success);
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

        fname = lastImageDir.absolutePath() + "/" + getMapName() + ".svg";
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

    ex.setResult(ExportBase::Success);
    ex.completeExport();

    return offset;
}

void VymModel::exportTaskJuggler(QString fname, bool askForName) // FIXME-3 not scriptable yet
{
    if (fname == "") {
        if (!askForName) {
            qWarning("VymModel::exportTaskJuggler called without filename (and "
                     "askName==false)");
            return;
        }

        fname = lastImageDir.absolutePath() + "/" + getMapName() + ".svg";
    }

    ExportTaskJuggler ex;
    ex.setModel(this);
    ex.setFilePath(fname);
    ex.setWindowTitle(vymName + " - " + tr("Export to") + " Taskjuggler" +
                          tr("(still experimental)"));
    ex.addFilter("Taskjuggler (*.tjp)");

    if (askForName) {
        if (!ex.execDialog())
            return;
        fname = ex.getFilePath();
        lastImageDir = QDir(fname);
    }

    setExportMode(true);

    ex.doExport();

    setExportMode(false);

    ex.completeExport();
}

void VymModel::exportXML(QString fpath, bool useDialog)
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

        if (fd.exec() != QDialog::Accepted || fd.selectedFiles().isEmpty())
            return;

        fpath = fd.selectedFiles().first();

    }
    QString dpath = fpath.left(fpath.lastIndexOf("/"));
    if (!confirmDirectoryOverwrite(QDir(dpath)))
        return;

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
    exportBoundingBoxes = true;
    QString saveFile =
        saveToDir(dpath, mname + "-", FlagRowMaster::AllFlags, offset);
    exportBoundingBoxes = false;

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
    ts << saveFile;
    file.close();


    ex.setResult(ExportBase::Success);

    setExportMode(false);

    QStringList args;
    args << fpath;
    args << dpath;
    ex.completeExport(args);
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

    exportBoundingBoxes = true;
    ex.doExport(useDialog);
    exportBoundingBoxes = false;
}

void VymModel::exportConfluence(bool createPage, const QString &pageURL,
                                const QString &pageName, bool useDialog)
{
    ExportConfluence ex(this);
    ex.setCreateNewPage(createPage);
    ex.setUrl(pageURL);
    ex.setPageName(pageName);
    ex.setLastCommand(
        settings.localValue(filePath, "/export/last/command", "").toString());

    ex.doExport(useDialog);
}

void VymModel::exportImpress(QString fname, QString configFile, bool useDialog)
{
    if (fname == "")
        fname = mapName + ".odp";

    if (useDialog) {
        ExportImpressFileDialog fd;
        fd.setWindowTitle(vymName + " - " + tr("Export to") + " LibreOffice Impress");
        fd.setDirectory(lastExportDir);
        fd.setAcceptMode(QFileDialog::AcceptSave);
        fd.setFileMode(QFileDialog::AnyFile);
        if (fd.foundConfig()) {
            if (fd.exec() == QDialog::Accepted) {
                configFile = fd.selectedConfig();
                if (!fd.selectedFiles().isEmpty()) {
                    QString fname = fd.selectedFiles().first();
                    if (!fname.contains(".odp"))
                        fname += ".odp";
                }
            }
        }
        else {
            QMessageBox::warning(
                0, tr("Warning"),
                tr("Couldn't find configuration for export to LibreOffice Impress\n"));
            return;
        }
    }


    ExportImpress ex;
    ex.setFilePath(fname);

    ex.setModel(this);
    ex.setLastCommand(
        settings.localValue(filePath, "/export/last/command", "").toString());

    if (ex.setConfigFile(configFile)) {
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

        //lastExportDir = fname.left(fname.findRev ("/"));
    }
}

bool VymModel::exportLastAvailable(QString &description, QString &command,
                                   QString &dest)
{
    command =
        settings.localValue(filePath, "/export/last/command", "").toString();
    static QRegularExpression re;
    re.setPattern("exportMap\\((\".*)\\)");
    QRegularExpressionMatch match = re.match(command);
    if (match.hasMatch()) {
        command = QString("vym.currentMap().exportMap([%1]);").arg(match.captured(1));
        settings.setLocalValue(filePath, "/export/last/command", command);
        qDebug() << "Rewriting last export command to version " << vymVersion << " format: " << command;
    }

    description = settings.localValue(filePath, "/export/last/description", "")
                      .toString();
    dest = settings.localValue(filePath, "/export/last/displayedDestination", "")
               .toString();
    if (!command.isEmpty() && command.contains("exportMap"))
        return true;
    else
        return false;
}

void VymModel::setExportLastCommand(const QString &cmd)
{
    settings.setLocalValue(filePath, "/export/last/command", cmd);
}

void VymModel::setExportLastDescription(const QString &desc)
{
    settings.setLocalValue(filePath, "/export/last/description", desc);
}

void VymModel::setExportLastDestination(const QString &displayedDest)
{
    settings.setLocalValue(filePath, "/export/last/displayedDestination", displayedDest);
}

void VymModel::exportLast()
{
    QString desc, command,
        dest; // FIXME-3 better integrate configFile into command
    if (exportLastAvailable(desc, command, dest)) {
        mainWindow->runScript(command);
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
        ex.setFilePath(mapName + ".md");
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

void VymModel::registerMapEditor(QWidget *e) { mapEditor = (MapEditor *)e; }

void VymModel::setMapZoomFactor(const double &d)
{
    if (!mapEditor) {
        qWarning() << __func__ << "mapEditor == nullptr";
        return;
    }

    zoomFactor = d;
    mapEditor->setZoomFactorTarget(d);
}

void VymModel::setMapRotation(const double &a)
{
    if (!mapEditor) {
        qWarning() << __func__ << "mapEditor == nullptr";
        return;
    }

    if (a < 1)
        // Round to zero, otherwise selectionMode in MapEditor might be 
        // "Geometric" when it should be "Classic"
        mapRotationInt = 0;
    else
        mapRotationInt = a;
    mapEditor->setRotationTarget(mapRotationInt);
}

void VymModel::setMapAnimDuration(const int &d) { animDuration = d; }

void VymModel::setMapAnimCurve(const QEasingCurve &c) { animCurve = c; }

bool VymModel::centerOnID(const QString &id)
{
    if (!mapEditor) {
        qWarning() << __func__ << "mapEditor == nullptr";
        return false;
    }

    TreeItem *ti = findUuid(QUuid(id));
    if (ti && (ti->hasTypeBranch() || ti->hasTypeImage())) {
        QPointF p_center;
        if (ti->hasTypeBranch())
            p_center = ((BranchItem*)ti)->getBranchContainer()->ornamentsSceneRect().center();
        else {
            Container *c;
            c = ((MapItem*)ti)->getContainer();
            p_center = c->mapToScene(c->rect().center());
        }
        if (zoomFactor > 0 ) {
            mapEditor->setViewCenterTarget(p_center, zoomFactor,
                                           mapRotationInt, animDuration,
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

void VymModel::reposition(bool force)
{
    if (!force && repositionBlocked)
        return;

    //qDebug() << "VM::reposition start force=" << force;

    // Reposition containers
    BranchItem *bi;
    for (int i = 0; i < rootItem->branchCount(); i++) {
        bi = rootItem->getBranchNum(i);
        bi->repositionContainers();
    }

    repositionXLinks();

    if (mapEditor)
        mapEditor->minimizeView();  // Optimize view when geometry changes in reposition()

    //qDebug() << "VM::reposition end";
    if (force)
        qApp->processEvents();
}

void VymModel::repositionXLinks()
{
    // Reposition xlinks
    foreach (XLink *xlink, xlinks)
        xlink->updateXLink();
}

MapDesign* VymModel::mapDesign()
{
    return mapDesignInt;
}

void VymModel::applyDesign(
        MapDesign::UpdateMode updateMode,
        BranchItem *bi)
{
    /*
    qDebug() << "VM::applyDesign  mode="
        << MapDesign::updateModeString(updateMode)
        << " of " << headingText(bi);
    */

    QList<BranchItem *> selbis = getSelectedBranches(bi);

    bool saveStateBlockedOrg = saveStateBlocked;
    saveStateBlocked = true;

    bool updateRequired;
    foreach (BranchItem *selbi, selbis) {
        dataChangedBlocked = true;
        int depth = selbi->depth();
        bool selbiChanged = false;
        BranchContainer *bc = selbi->getBranchContainer();

        // Color of heading
        QColor col = mapDesignInt->headingColor(
                        updateMode,
                        selbi,
                        updateRequired);

        if (updateRequired)
            colorBranch(col, selbi);

        // Inner frame
        if (updateMode == MapDesign::CreatedByUser ||
                (updateMode == MapDesign::RelinkedByUser && mapDesignInt->updateFrameWhenRelinking(true, depth))) {
            bc->setFrameType(true, mapDesignInt->frameType(true, depth));
            bc->setFrameBrushColor(true, mapDesignInt->frameBrushColor(true, depth));
            bc->setFramePenColor(true, mapDesignInt->framePenColor(true, depth));
            bc->setFramePenWidth(true, mapDesignInt->framePenWidth(true, depth));
        }

        // Outer frame
        if (updateMode == MapDesign::CreatedByUser ||
                (updateMode == MapDesign::RelinkedByUser && mapDesignInt->updateFrameWhenRelinking(false, depth))) {
            bc->setFrameType(false, mapDesignInt->frameType(false, depth));
            bc->setFrameBrushColor(false, mapDesignInt->frameBrushColor(false, depth));
            bc->setFramePenColor(false, mapDesignInt->framePenColor(false, depth));
            bc->setFramePenWidth(false, mapDesignInt->framePenWidth(false, depth));
        }

        // Column width and font
        if (updateMode & MapDesign::CreatedByUser || updateMode & MapDesign::LoadingMap) {
            HeadingContainer *hc = bc->getHeadingContainer();
            hc->setColumnWidth(mapDesignInt->headingColumnWidth(depth));
            hc->setFont(mapDesignInt->font());
        }

        // Layouts
        if (bc->branchesContainerAutoLayout) {
                bc->setBranchesContainerLayout(
                        mapDesignInt->branchesContainerLayout(depth));
                selbiChanged = true;
                        mapDesignInt->branchesContainerLayout(depth);
        }

        if (bc->imagesContainerAutoLayout) {
                bc->setImagesContainerLayout(
                        mapDesignInt->imagesContainerLayout(depth));
                selbiChanged = true;
        }
	bc->setBranchesContainerVerticalAlignment(
		mapDesignInt->branchesContainerVerticalAlignment(depth));
	bc->setBranchesContainerAndOrnamentsVertical(
		mapDesignInt->branchesContainerAndOrnamentsVertical(depth));

        // Links and bottomlines
        bc->updateUpLink();

        // Rotations
        if (bc->rotationsAutoDesign()) {
            qreal a = mapDesignInt->rotationHeading(depth);
            if (a != bc->rotationHeading()) {
                bc->setRotationHeading(a);
                selbiChanged = true;
            }
            a = mapDesignInt->rotationSubtree(depth);
            if (a != bc->rotationHeading()) {
                bc->setRotationHeading(a);
                selbiChanged = true;
            }
        }

        if (bc->scaleAutoDesign()) {
            qreal z = mapDesignInt->scaleHeading(depth);
            if (z != bc->scaleHeading()) {
                bc->setScaleHeading(z);
                selbiChanged = true;
            }
            z = mapDesignInt->scaleSubtree(depth);
            if (z != bc->scaleSubtree()) {
                bc->setScaleSubtree(z);
                selbiChanged = true;
            }
        }

        dataChangedBlocked = false;
        if (selbiChanged)
            emitDataChanged(selbi);
    }

    saveStateBlocked = saveStateBlockedOrg;
}

void VymModel::applyDesignRecursively(
        MapDesign::UpdateMode updateMode,
        BranchItem *bi)
{
    if (!bi) return;

    for (int i = 0; i < bi->branchCount(); ++i)
        applyDesign(updateMode, bi->getBranchNum(i));
}

void VymModel::setDefaultFont(const QFont &font)    // FIXME-3 no saveState, no updates of existing headings ("applyDesign")
{
    mapDesignInt->setFont(font);
}

bool VymModel::setLinkStyle(const QString &newStyleString, int depth)
{
    auto style = LinkObj::styleFromString(newStyleString);

    if (depth >= 0) {
        QString currentStyleString = LinkObj::styleString(mapDesignInt->linkStyle(depth));

        QString uc = QString("map.setLinkStyle (\"%1\", %2);").arg(newStyleString, depth);
        QString rc = QString("map.setLinkStyle (\"%1\", %2);").arg(currentStyleString, depth);
        QString com = QString("Set map link style (\"%1\", %2)").arg(newStyleString, depth);
        logAction(rc, com, __func__);
        saveState(uc, rc, com);
        mapDesignInt->setLinkStyle(style, depth);
    } else {
        // Default depth == -1 is used for legacy styles from version < 2.9.518
        // or for using global setting from context menu
        // Only apply the "thick" part on first level
        QString com = QString("Set link styles for map to \"%1\"").arg(newStyleString);
        QString currentStyleString0 = LinkObj::styleString(mapDesignInt->linkStyle(0));
        QString currentStyleString1 = LinkObj::styleString(mapDesignInt->linkStyle(1));
        QString currentStyleString2 = LinkObj::styleString(mapDesignInt->linkStyle(2));
        QString uc0 = QString("map.setLinkStyle (\"%1\", 0);").arg(currentStyleString0);
        QString uc1 = QString("map.setLinkStyle (\"%1\", 1);").arg(currentStyleString1);
        QString uc2 = QString("map.setLinkStyle (\"%1\", 2);").arg(currentStyleString2);

        QString rc0 = QString("map.setLinkStyle (\"StyleNoLink\", 0);");
        mapDesignInt->setLinkStyle(LinkObj::NoLink, 0);

        QString rc1 = QString("map.setLinkStyle (\"%1\", 1);").arg(newStyleString);
        mapDesignInt->setLinkStyle(style, 1);

        QString rc2;
        if (style == LinkObj::PolyLine) {
            rc2 = QString("map.setLinkStyle (\"StyleLine\", 2);");
            mapDesignInt->setLinkStyle(LinkObj::Line, 2);
        } else if (style == LinkObj::PolyParabel) {
            rc2 = QString("map.setLinkStyle (\"StyleParabel\", 2);");
            mapDesignInt->setLinkStyle(LinkObj::Parabel, 2);
        }
        saveState(uc0 + uc1 + uc2, rc0 + rc1 +  rc2, com);
    }


    applyDesignRecursively(MapDesign::LinkStyleChanged, rootItem);
    reposition();

    return true;
}

uint VymModel::modelId() { return modelIdInt; }

void VymModel::setView(VymView *vv) { vymView = vv; }

void VymModel::setDefaultLinkColor(const QColor &col)
{
    if (!col.isValid()) return;

    QString uc = QString("map.setDefaultLinkColor (\"%1\");").arg(mapDesignInt->defaultLinkColor().name());
    QString rc = QString("map.setDefaultLinkColor (\"%1\");").arg(col.name());
    QString com = QString("Set map link color to %1").arg(col.name());
    logAction(rc, com, __func__);
    saveState(uc, rc, com);

    mapDesignInt->setDefaultLinkColor(col);

    // Set color for "link arrows" in TreeEditor
    vymView->updateColors();

    BranchItem *cur = nullptr;
    BranchItem *prev = nullptr;
    nextBranch(cur, prev);
    while (cur) {
        BranchContainer *bc = cur->getBranchContainer();
        bc->updateUpLink();
        // for (int i = 0; i < cur->imageCount(); ++i)
        // FIXME-4 setLinkColorHint: images currently use branch link color

        nextBranch(cur, prev);
    }
    updateActions();
}

void VymModel::setLinkColorHint(const LinkObj::ColorHint &newHint)
{
    LinkObj::ColorHint oldHint = mapDesignInt->linkColorHint();

    if (oldHint == newHint)
        return;

    mapDesignInt->setLinkColorHint(newHint);

    QString oldHintName = LinkObj::linkColorHintName(oldHint);
    QString newHintName = LinkObj::linkColorHintName(newHint);

    QString uc = QString("map.setLinkColorHint (\"%1\");").arg(oldHintName);
    QString rc = QString("map.setLinkColorHint (\"%1\");").arg(newHintName);
    QString com = QString("Set link color hint to %1").arg(newHintName);
    logAction(rc, com, __func__);
    saveState(uc, rc, com);
    
    BranchItem *cur = nullptr;
    BranchItem *prev = nullptr;
    nextBranch(cur, prev);
    while (cur) {
        BranchContainer *bc = cur->getBranchContainer();
        LinkObj *upLink = bc->getLink();
        if (upLink)
            upLink->setLinkColorHint(newHint);

        // FIXME-4 setLinkColorHint: images currently use branch link color
        for (int i = 0; i < cur->imageCount(); ++i) {
            upLink = cur->getImageNum(i)->getImageContainer()->getLink();
            if (upLink)
                upLink->setLinkColorHint(newHint);
        }
        nextBranch(cur, prev);
    }

    applyDesignRecursively(MapDesign::LinkStyleChanged, rootItem);
    reposition();
}

void VymModel::toggleLinkColorHint()
{
    if (mapDesignInt->linkColorHint() == LinkObj::HeadingColor)
        setLinkColorHint(LinkObj::DefaultColor);
    else
        setLinkColorHint(LinkObj::HeadingColor);
}

QColor VymModel::backgroundColor()
{
    return mapDesignInt->backgroundColor();
}

void VymModel::setBackgroundColor(QColor col)
{
    saveStateBeginScript("Set background color");   // Save background image in script

    if (hasBackgroundImage())
        unsetBackgroundImage();

    QString uc = QString("map.setBackgroundColor(\"%1\");").arg(mapDesignInt->backgroundColor().name());
    QString rc = QString("map.setBackgroundColor(\"%1\");").arg(col.name());
    QString com = QString("Set background color of map to %1").arg(col.name());
    logAction(rc, com, __func__);
    saveState(uc, rc, com);

    saveStateEndScript();

    mapDesignInt->setBackgroundColor(col);  // Used for backroundRole in TreeModel::data()

    vymView->updateColors();
}

bool VymModel::loadBackgroundImage( const QString &imagePath)
{
    // FIXME-4 maybe also use: view.setCacheMode(QGraphicsView::CacheBackground);

    if (!saveStateBlocked) {
        QString uc, rc;
        
        QString comment = QString("Load background image: \"%1\"").arg(imagePath);

        logAction(rc, comment, __func__);

        saveStateBeginScript(comment);  // load BG img, save previous img if used

        bool saveOldImage = false;

        QString oldImagePath = "images/background-image-old.png";
        QString newImagePath = "images/background-image-new.png";

        if (hasBackgroundImage()) {
            saveOldImage = true;
            uc = QString("map.loadBackgroundImage(\"HISTORY_PATH/%1\");").arg(oldImagePath);
        } else
            uc = QString("map.unsetBackgroundImage();");

        QFile newImage (imagePath);
        if (!newImage.exists()) {
            qWarning() << __FUNCTION__ << " Image to load as background does not exist: " << imagePath;
            return false;
        }

        rc = QString("map.loadBackgroundImage(\"HISTORY_PATH/%1\");").arg(newImagePath);

        QString historyPath = saveState(uc, rc, comment, nullptr, nullptr, true);
        if (saveOldImage) {
            if (!mapDesignInt->saveBackgroundImage(historyPath + oldImagePath)) {
                qWarning() << __FUNCTION__ << " Failed to save existing background image to: " << historyPath + oldImagePath;
                return false;   // FIXME-4 For all aborts, drop last history step...
            }
        }

        // Copy new image to historyPath and (if previously) used also old image
        if (!newImage.copy(historyPath + newImagePath)) {
            qWarning() << __FUNCTION__ << " failed to copy new background image to " << historyPath + newImagePath;
            return false;
        }
        
        setBackgroundImageName(basename(imagePath));

        saveStateEndScript();
    }

    if (mapDesignInt->loadBackgroundImage(imagePath)) {
        vymView->updateColors();
        return true;
    }

    qWarning() << __FUNCTION__ << " failed to load new background image from " << imagePath;

    return false;
}

void VymModel::setBackgroundImageName( const QString &newName)
{
    QString oldName = mapDesignInt->backgroundImageName();

    QString uc = QString("map.setBackgroundImageName(\"%1\");").arg(oldName);
    QString rc = QString("map.setBackgroundImageName(\"%1\");").arg(newName);
    QString com = QString("Set name of background image to \"%1\"").arg(newName);
    logAction(rc, com, __func__);
    saveState(uc, rc, com);
    mapDesignInt->setBackgroundImageName(newName);
}

void VymModel::unsetBackgroundImage()
{
    if (mapDesignInt->hasBackgroundImage()) {
        if (!saveStateBlocked) {
            QString uc, rc, com;

            QString oldImagePath = "images/background-image-old.png";

            uc = QString("map.loadBackgroundImage(\"HISTORY_PATH/%1\");").arg(oldImagePath);
            rc = QString("map.unsetBackgroundImage();");
            com = QString("Unset background image");
            logAction(rc, com, __func__);

            QString historyPath = saveState(uc, rc, com, nullptr, nullptr, true);
            if (!mapDesignInt->saveBackgroundImage(historyPath + oldImagePath)) {
                logWarning(" Failed to save existing background image to: " + historyPath + oldImagePath, __func__);
                return;   // FIXME-4 For all aborts, drop last history step...
            }
        }

        mapDesignInt->unsetBackgroundImage();
        vymView->updateColors();
    }
}

bool VymModel::hasBackgroundImage()
{
    return mapDesignInt->hasBackgroundImage();
}

QString VymModel::backgroundImageName()
{
    return mapDesignInt->backgroundImageName();
}

void VymModel::setDefXLinkPen(const QPen &p)
{
    mapDesignInt->setDefXLinkPen(p);
}

void VymModel::setDefXLinkStyleBegin(const QString &s)
{
    mapDesignInt->setDefXLinkStyleBegin(s);
}

void VymModel::setDefXLinkStyleEnd(const QString &s)
{
    mapDesignInt->setDefXLinkStyleEnd(s);
}

void VymModel::setPos(const QPointF &pos_new, TreeItem *selti)
{
    QList<TreeItem *> selItems;
    if (selti)
        selItems.append(selti);
    else
        selItems = getSelectedItems();

    QString com = "Move items (non-interactive";
    QString uc, rc, itemVar;
    foreach (TreeItem *ti, selItems) {
        if (ti->hasTypeBranch() || ti->hasTypeImage())
        {
            Container *c = ((MapItem*)ti)->getContainer();
            QString pos_new_str = toS(pos_new);

            if (ti->hasTypeBranch())
                itemVar = setBranchVar((BranchItem*)ti) + "b.";
            else 
                itemVar = setImageVar((ImageItem*)ti) + "i.";
            uc += QString("%1.setPos%2;").arg(itemVar, toS(c->getOriginalPos(), 5));
            rc += QString("%1.setPos%2;").arg(itemVar, toS(c->pos(), 5));
            c->setPos(pos_new);
        }
    }
    logAction(rc, com, __func__);
    saveState(uc, rc); 
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
        QMessageBox::critical(nullptr, "vym server",
                              QString("Unable to start the server: %1.")
                                  .arg(tcpServer->errorString()));
        // FIXME needed? we are no widget any longer... close();
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

void VymModel::readData()   // FIXME-5 not used currently
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
        // parseAtom (t,noErr,errMsg);
    }
    return;
}

void VymModel::displayNetworkError(QAbstractSocket::SocketError socketError)
{
    switch (socketError) {
    case QAbstractSocket::RemoteHostClosedError:
        break;
    case QAbstractSocket::HostNotFoundError:
        QMessageBox::information(nullptr, vymName + " Network client",
                                 "The host was not found. Please check the "
                                 "host name and port settings.");
        break;
    case QAbstractSocket::ConnectionRefusedError:
        QMessageBox::information(nullptr, vymName + " Network client",
                                 "The connection was refused by the peer. "
                                 "Make sure the fortune server is running, "
                                 "and check that the host name and port "
                                 "settings are correct.");
        break;
    default:
        QMessageBox::information(nullptr, vymName + " Network client",
                                 QString("The following error occurred: %1.")
                                     .arg(clientSocket->errorString()));
    }
}

void VymModel::downloadImage(const QUrl &url, BranchItem *bi)
{
    if (!bi)
        bi = getSelectedBranch();
    if (!bi) {
        qWarning("VM::download bi==nullptr");
        return;
    }

    // FIXME-4 delete tmp file of image download after running script
    QString script;
    script += QString("m = vym.currentMap();b = m.findBranchBySelection(\"%1\");")
                  .arg(bi->getUuid().toString());
    script += QString("b.loadImage(\"$TMPFILE\");");

    DownloadAgent *agent = new DownloadAgent(url);
    agent->setFinishedAction(this, script);
    connect(agent, SIGNAL(downloadFinished()), mainWindow,
            SLOT(downloadFinished()));
    QTimer::singleShot(0, agent, SLOT(execute()));
}

void VymModel::setSelectionPenColor(QColor col)
{
    if (!col.isValid())
        return;

    QPen selPen = mapDesignInt->selectionPen();
    QString uc = QString("map.setSelectionPenColor (\"%1\");").arg(selPen.color().name());
    QString rc = QString("map.setSelectionPenColor (\"%1\");").arg(col.name());
    QString com = QString("Set pen color of selection box to %1").arg(col.name());
    logAction(rc, com, __func__);
    saveState(uc, rc, com);

    selPen.setColor(col);
    mapDesignInt->setSelectionPen(selPen);
    vymView->updateColors();
}

QColor VymModel::getSelectionPenColor() {
    return mapDesignInt->selectionPen().color();
}

void VymModel::setSelectionPenWidth(qreal w)
{
    QPen selPen = mapDesignInt->selectionPen();
    
    QString uc = QString("map.setSelectionPenWidth (\"%1\");").arg(mapDesignInt->selectionPen().width());
    QString rc = QString("map.setSelectionPenWidth (\"%1\");").arg(w);
    QString com = QString("Set pen width of selection box to %1").arg(w);
    logAction(rc, com, __func__);
    saveState(uc, rc, com);

    selPen.setWidth(w);
    mapDesignInt->setSelectionPen(selPen);
    vymView->updateColors();
}

qreal VymModel::getSelectionPenWidth() {
    return mapDesignInt->selectionPen().width();
}

void VymModel::setSelectionBrushColor(QColor col)
{
    if (!col.isValid())
        return;

    QBrush selBrush = mapDesignInt->selectionBrush();
    QString uc = QString("map.setSelectionBrushColor (\"%1\");").arg(selBrush.color().name());
    QString rc = QString("map.setSelectionBrushColor (\"%1\");").arg(col.name());
    QString com = QString("Set Brush color of selection box to %1").arg(col.name());
    logAction(rc, com, __func__);
    saveState(uc, rc, com);

    selBrush.setColor(col);
    mapDesignInt->setSelectionBrush(selBrush);
    vymView->updateColors();
}

QColor VymModel::getSelectionBrushColor() {
    return mapDesignInt->selectionBrush().color();
}

void VymModel::setHideTmpMode(TreeItem::HideTmpMode mode)
{
    if (hideMode == mode)
        return;

    hideMode = mode;
    for (int i = 0; i < rootItem->branchCount(); i++)
        rootItem->getBranchNum(i)->setHideMode(mode);
    reposition();
    if (mode == TreeItem::HideExport)
        unselectAll();
    else
        reselect();

    reposition();

    qApp->processEvents();
}

void VymModel::toggleHideTmpMode() {
    if (hideMode == TreeItem::HideNone)
        setHideTmpMode(TreeItem::HideExport);
    else
        setHideTmpMode(TreeItem::HideNone);
}

//////////////////////////////////////////////
// Selection related
//////////////////////////////////////////////

void VymModel::updateSelection(QItemSelection newsel, QItemSelection dsel)
{
    // Set selection status in objects
    // Temporary unscroll or rescroll as required

    //qDebug() << "VM::updateSel  newsel=" << newsel << " dsel=" << dsel;
    QModelIndex ix;
    MapItem *mi;
    BranchItem *bi;
    bool do_reposition = false;
    // Unselect objects (if not part of selection)
    foreach (ix, dsel.indexes()) {
        mi = static_cast<MapItem *>(ix.internalPointer());

        if (mi->hasTypeBranch() || mi->getType() == TreeItem::Image || mi->getType() == TreeItem::XLinkItemType) {
            if (mi->hasTypeBranch()) {
                BranchContainer *bc = ((BranchItem*)mi)->getBranchContainer();
                bc->unselect();
                bc->updateVisibility();
                do_reposition =
                    do_reposition || ((BranchItem *)mi)->resetTmpUnscroll();
            }
            if (mi->hasTypeImage()) {
                ImageContainer *ic = ((ImageItem*)mi)->getImageContainer();
                ic->unselect();
                ic->updateVisibility();
            }
            if (mi->hasTypeXLink()) {
                ((XLinkItem*)mi)->getXLinkObj()->unselect();
                XLink *li = ((XLinkItem *)mi)->getXLink();

                do_reposition =
                    do_reposition || li->getBeginBranch()->resetTmpUnscroll();
                do_reposition =
                    do_reposition || li->getEndBranch()->resetTmpUnscroll();
            }
        }
    }

    foreach (ix, newsel.indexes()) {
        mi = static_cast<MapItem *>(ix.internalPointer());
        if (mi->hasTypeBranch()) {
            bi = (BranchItem *)mi;
            BranchContainer *bc = bi->getBranchContainer();
            bc->select();
            bc->updateVisibility();
            if (bi->hasScrolledParent()) {
                bi->tmpUnscroll();
                do_reposition = true;
            }
        }
        if (mi->hasTypeImage()) {
            ImageContainer *ic = ((ImageItem*)mi)->getImageContainer();

            // Make sure that images have correct upLinkPosSelf_sp
            // (Could be off after loading and IF no sibling branches are following)
            ic->updateUpLink();

            ic->select();
            ic->updateVisibility();
        }

        if (mi->getType() == TreeItem::XLinkItemType) {
            XLinkItem *xli = (XLinkItem*)mi;
            xli->setSelectionType();
            xli->getXLinkObj()->select(
                mapDesign()->selectionPen(),
                mapDesign()->selectionBrush());

            // begin/end branches need to be tmp unscrolled
            XLink *xl = ((XLinkItem *)mi)->getXLink();
            bi = xl->getBeginBranch();
            if (bi->hasScrolledParent()) {
                bi->tmpUnscroll();
                do_reposition = true;
            }
            bi = xl->getEndBranch();
            if (bi->hasScrolledParent()) {
                bi->tmpUnscroll();
                do_reposition = true;
            }
        }
    }

    // Show count of multiple selected items
    int selCount = selModel->selection().indexes().count();
    if (selCount > 1)
        mainWindow->statusMessage(
            tr("%1 items selected","Status message when selecting multiple items").arg(selCount));
    else
        mainWindow->statusMessage("");

    if (do_reposition)
        reposition();
}

void VymModel::setSelectionModel(QItemSelectionModel *sm) { selModel = sm; }

int VymModel::selectedItemsCount()
{
    return selModel->selectedIndexes().count();
}

QItemSelectionModel *VymModel::getSelectionModel() { return selModel; }

void VymModel::setSelectionBlocked(bool b) { selectionBlocked = b; }

bool VymModel::isSelectionBlocked() { return selectionBlocked; }

bool VymModel::select(const QString &s)
{
    if (s.isEmpty())
        return false;

    QStringList list = s.split(";");

    unselectAll();
    foreach (QString t, list) {
        TreeItem *ti = findBySelectString(t);
        if (ti)
            selectToggle(ti);
        else
            return false;
    }
    return true;
}

bool VymModel::selectUids(QStringList  uids)
{
    if (uids.isEmpty())
        return false;

    unselectAll();
    foreach (auto uid, uids) {
        TreeItem *ti = findUuid(QUuid(uid));
        if (!ti)
            return false;
        selectToggle(ti);
    }
    return true;
}

bool VymModel::selectToggle(TreeItem *ti)
{
    if (ti) {
        selModel->select(index(ti), QItemSelectionModel::Toggle);
        // appendSelectionToHistory();	// FIXME-3 selection history not implemented yet
        // for multiselections
        lastToggledUuid = ti->getUuid();
        return true;
    }
    return false;
}

bool VymModel::selectToggle(const uint &id)
{
    TreeItem *ti = findID(id);
    return selectToggle(ti);
}

bool VymModel::selectToggle(const QUuid &uid)
{
    TreeItem *ti = findUuid(uid);
    return selectToggle(ti);
}

bool VymModel::selectToggle(const QString &selectString)
{
    TreeItem *ti = findBySelectString(selectString);
    return selectToggle(ti);
}

bool VymModel::select(TreeItem *ti)
{
    if (ti)
        return select(index(ti));
    else
        return false;
}

bool VymModel::select(const QUuid &uuid)
{
    TreeItem *ti = findUuid(uuid);
    return select(ti);
}

bool VymModel::select(const QModelIndex &index)
{
    if (index.isValid()) {
        TreeItem *ti = getItem(index);
        if (ti->hasTypeBranch()) {
            if (((BranchItem *)ti)->tmpUnscroll())
                reposition();
        }
        selModel->select(index, QItemSelectionModel::ClearAndSelect);
        appendSelectionToHistory();
        return true;
    }
    return false;
}

void VymModel::select(QList <BranchItem*> selbis)
{
    unselectAll();
    foreach (BranchItem* selbi, selbis)
        selectToggle(selbi);
}

void VymModel::select(QList <TreeItem*> tis)
{
    unselectAll();
    foreach (TreeItem* ti, tis)
        selectToggle(ti);
}

void VymModel::unselectAll() {
    lastSelectString = getSelectString();
    selModel->clearSelection();
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

void VymModel::appendSelectionToHistory() // FIXME-3 history unable to cope with multiple
                                          // selections
{
    uint id = 0;
    TreeItem *ti = getSelectedItem();
    if (ti && !keepSelectionHistory) {
        if (ti->hasTypeBranch())
            ((BranchItem *)ti)->setLastSelectedBranch();
        id = ti->getID();
        selectionHistory.append(id);
        currentSelection = selectionHistory.count() - 1;
        updateActions();
    }
}

void VymModel::emitShowSelection(bool scaled, bool rotated)
{
    if (!repositionBlocked)
        emit showSelection(scaled, rotated);
}

TreeItem* VymModel::lastToggledItem()
{
    return findUuid(lastToggledUuid);
}

void VymModel::emitNoteChanged(TreeItem *ti)
{
    QModelIndex ix = index(ti);
    emit noteChanged(ix);
    mainWindow->updateNoteEditor(ti);
}

void VymModel::emitDataChanged(TreeItem *ti)
{
    //qDebug() << "VM::emitDataChanged ti=" << ti;
    if (!dataChangedBlocked && ti) {
        QModelIndex ix = index(ti);
        emit dataChanged(ix, ix);

        // Update taskmodel and recalc priorities there
        if (ti->hasTypeBranch() && ((BranchItem *)ti)->getTask()) {
            taskModel->emitDataChanged(((BranchItem *)ti)->getTask());
            taskModel->recalcPriorities();
        }
    }
}

void VymModel::emitUpdateQueries()
{
    // Used to tell MainWindow to update query results
    if (repositionBlocked) return;

    emit updateQueries(this);
}
void VymModel::emitUpdateLayout()
{
    if (settings.value("/mainwindow/autoLayout/use", "true") == "true")
        emit updateLayout();
}

bool VymModel::selectFirstBranch(BranchItem *bi)
{
    BranchItem* selbi = getSelectedBranch(bi);
    if (selbi) {
        TreeItem *par = selbi->parent();
        if (par) {
            TreeItem *ti2 = par->getFirstBranch();
            if (ti2)
                return select(ti2);
        }
    }
    return false;
}

bool VymModel::selectFirstChildBranch(BranchItem *bi)
{
    BranchItem *selbi = getSelectedBranch(bi);
    if (selbi) {
        BranchItem *bi2 = selbi->getFirstBranch();
        if (bi2)
            return select(bi2);
    }
    return false;
}

bool VymModel::selectLastBranch(BranchItem *bi)
{
    BranchItem* selbi = getSelectedBranch(bi);
    if (selbi) {
        TreeItem *par = selbi->parent();
        if (par) {
            BranchItem *bi2 = par->getLastBranch();
            if (bi2)
                return select(bi2);
        }
    }
    return false;
}

bool VymModel::selectLastChildBranch(BranchItem *bi)
{
    BranchItem *selbi = getSelectedBranch(bi);
    if (selbi) {
        BranchItem *bi2 = selbi->getLastBranch();
        if (bi2)
            return select(bi2);
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

bool VymModel::selectLatestAdded()
{
    TreeItem *ti = findUuid(latestAddedItemUuid);
        return select(ti);
}

bool VymModel::selectParent(TreeItem *ti)
{
    TreeItem *selti = getSelectedItem(ti);
    TreeItem *par;
    if (selti) {
        par = selti->parent();
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

BranchItem *VymModel::getSelectedBranch(BranchItem *bi)
{
    if (bi) return bi;

    // Return selected branch,
    // if several are selected, return last selected
    QList<BranchItem *> bis = getSelectedBranches();
    if (bis.count() == 0) return nullptr;

    return bis.last();
}

QList<BranchItem *> VymModel::getSelectedBranches(TreeItem *ti)
{
    // Return list of selected branches.
    // If ti != nullptr and is branch, return only this one
    QList<BranchItem *> selbis;

    if (ti && ti->hasTypeBranch()) {
        selbis << (BranchItem*)ti;
        return selbis;
    }

    foreach (TreeItem *ti, getSelectedItems()) {
        if (ti->hasTypeBranch())
            selbis.append((BranchItem *)ti);
    }
    return selbis;
}

ImageItem *VymModel::getSelectedImage()
{
    // Return selected image,
    // if several are selected, return last selected
    QList<ImageItem *> iis = getSelectedImages();
    if (iis.count() == 0) return nullptr;

    return iis.last();
}

QList<ImageItem *> VymModel::getSelectedImages(TreeItem *ti)
{
    // Return list of selected images.
    // If ii != nullptr, return only this one
    QList<ImageItem *> iis;

    if (ti && ti->hasTypeImage()) {
        iis << (ImageItem*)ti;
        return iis;
    }

    foreach (TreeItem *ti, getSelectedItems()) {
        if (ti->hasTypeImage())
            iis.append((ImageItem *)ti);
    }
    return iis;
}

Task *VymModel::getSelectedTask()
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi)
        return selbi->getTask();
    else
        return nullptr;
}

XLink *VymModel::getSelectedXLink(XLink *xl)
{
    if (xl)
        return xl;

    XLinkItem *xli = getSelectedXLinkItem();
    if (xli)
        return xli->getXLink();
    return nullptr;
}

XLinkItem *VymModel::getSelectedXLinkItem()
{
    TreeItem *ti = getSelectedItem();
    if (ti && ti->getType() == TreeItem::XLinkItemType)
        return (XLinkItem *)ti;
    else
        return nullptr;
}

AttributeItem *VymModel::getSelectedAttribute()
{
    TreeItem *ti = getSelectedItem();
    if (ti && ti->getType() == TreeItem::Attribute)
        return (AttributeItem *)ti;
    else
        return nullptr;
}

TreeItem *VymModel::getSelectedItem(TreeItem *ti)
{
    if (ti) return ti;

    if (!selModel)
        return nullptr;
    QModelIndexList list = selModel->selectedIndexes();
    if (list.count() == 1)
        return getItem(list.first());
    else
        return nullptr;
}

QList<TreeItem *> VymModel::getSelectedItems(TreeItem *ti)
{
    QList<TreeItem *> seltis;
    if (ti) {
        seltis << ti;
        return seltis;
    }

    if (!selModel)
        return seltis;

    QModelIndexList list = selModel->selectedIndexes();
    foreach (QModelIndex ix, list)
        seltis.append(getItem(ix));
    return seltis;
}

QList<TreeItem *> VymModel::getSelectedItemsReduced()
{
    // Remove items, whose have parents already in list

    QList<TreeItem *> list = getSelectedItems();

    if (list.isEmpty()) return list;

    // Bubble sort items by depth first
    for (int n = list.size(); n > 1; n--)
        for (int i = 0; i < n - 1; i++)
            if (list.at(i)->depth() > list.at(i + 1)->depth() )
                list.swapItemsAt(i, i + 1);

    // Remove items, which have parents which have smaller depth
    // (closer to center)
    int i = list.size() - 1;
    while (i > 0) {
        for (int j = 0; j < i; j++)
            if (list.at(i)->isChildOf(list.at(j))) {
                list.removeAt(i);
                break;
            }
        i--;
    }

    return list;
}

QModelIndex VymModel::getSelectedIndex()
{
    QModelIndexList list = selModel->selectedIndexes();
    if (list.count() == 1)
        return list.first();
    else
        return QModelIndex();
}

QList<ulong> VymModel::getSelectedIDs()
{
    QList<ulong> uids;
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
    QStringList list;
    QList <TreeItem*> seltis = getSelectedItems();
    foreach (TreeItem* selti, seltis)
        list << getSelectString(selti);

    return list.join(";");
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
    case TreeItem::XLinkItemType:
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

SlideModel *VymModel::getSlideModel() { return slideModel; }

int VymModel::slideCount() { return slideModel->count(); }

SlideItem *VymModel::addSlide()     // FIXME-3 missing saveState
{
    SlideItem *si = slideModel->getSelectedItem();
    if (si)
        si = slideModel->addSlide(nullptr, si->row() + 1);
    else
        si = slideModel->addSlide();

    TreeItem *seli = getSelectedItem();

    if (si) {
        if (seli) {
            QString inScript;
            if (!loadStringFromDisk(vymBaseDir.path() +
                                        "/macros/slideeditor-snapshot.vys",
                                    inScript)) {
                qWarning() << "VymModel::addSlide couldn't load template for "
                              "taking snapshot";
                return nullptr;
            }

            if (mapEditor) {
                inScript.replace(
                    "CURRENT_ZOOM",
                    QString().setNum(mapEditor->zoomFactorTarget()));
                inScript.replace("CURRENT_ANGLE",
                                 QString().setNum(mapEditor->rotationTarget()));
            }
            inScript.replace("CURRENT_ID",
                             "\"" + seli->getUuid().toString() + "\"");

            si->setInScript(inScript);
            slideModel->setData(slideModel->index(si), seli->headingPlain());
        }

        /*
        QString s = "<vymmap>" + si->saveToDir() + "</vymmap>";
        int pos = si->row();
        saveStateold(File::PartOfMap, getSelectString(),    // FIXME addAddSlide
                  QString("removeSlide (%1)").arg(pos), getSelectString(),
                  QString("addMapInsert (\"PATH\",%1)").arg(pos), "Add slide", nullptr,
                  s);
          */
        QString com = "Add slide";
        logAction("", com, __func__);
    }
    return si;
}

void VymModel::deleteSlide(SlideItem *si)  // FIXME-3 missing saveState
{
    if (si) {
        /*
        QString s = "<vymmap>" + si->saveToDir() + "</vymmap>";
        int pos = si->row();
        saveStateold(File::PartOfMap, getSelectString(),    // FIXME deleteAddSlide
                  QString("addMapInsert (\"PATH\",%1)").arg(pos),
                  getSelectString(), QString("removeSlide (%1)").arg(pos),
                  "Remove slide", nullptr, s);
                  */
        QString com = "Delete slide";
        logAction("", com, __func__);
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
    SlideItem *si = nullptr;
    if (n < 0) // default if called without parameters
    {
        si = slideModel->getSelectedItem();
        if (si)
            n = si->row();
        else
            return false;
    }
    else
        si = slideModel->getSlide(n);
    if (si && n >= 0 && n < slideModel->count() - 1) {
        QString uc = QString("map.moveSlideUp (%1);").arg(n + 1);
        QString rc = QString("map.moveSlideDown (%1);").arg(n);
        QString com = QString("Move slide %1 down").arg(n);
        logAction(rc, com, __func__);

        blockSlideSelection = true;
        slideModel->relinkSlide(si, si->parent(), n + 1);
        blockSlideSelection = false;
        saveState(uc, rc, com);
        return true;
    }
    else
        return false;
}

bool VymModel::moveSlideUp(int n)
{
    SlideItem *si = nullptr;
    if (n < 0) // default if called without parameters
    {
        si = slideModel->getSelectedItem();
        if (si)
            n = si->row();
        else
            return false;
    }
    else
        si = slideModel->getSlide(n);
    if (si && n > 0 && n < slideModel->count()) {
        QString uc = QString("map.moveSlideDown (%1);").arg(n + 1);
        QString rc = QString("map.moveSlideUp (%1);").arg(n);
        QString com = QString("Move slide %1 up").arg(n);
        logAction(rc, com, __func__);
        blockSlideSelection = true;
        slideModel->relinkSlide(si, si->parent(), n - 1);
        blockSlideSelection = false;
        saveState(uc, rc, com);
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
        scriptEditor->setSlideScript(modelIdInt, si->getID(), inScript);

        // Execute inScript
        mainWindow->runScript(inScript);
    }
}

void VymModel::logDebug(const QString &comment, const QString &caller)
{
    QString place = QString("\"%1\"").arg(fileName);
    if (!caller.isEmpty()) place += "  Called by: " + caller + "()";

    QString log = QString("\n// %1 [Debug] %2\n// MapID: %3 Map: %4").arg(
            QDateTime::currentDateTime().toString(Qt::ISODateWithMs),
            comment,
            QString::number(modelIdInt),
            place);

    if (debug)
        std::cout << log.toStdString() << std::endl << std::flush;

    if (!useActionLog) return;

    appendStringToFile(actionLogPath, log);
}

void VymModel::logInfo(const QString &comment, const QString &caller)
{
    if (!useActionLog) return;

    QString log = QString("\n// %1 [Info VymModel::%2 \"%3\"] %4").arg(
            QDateTime::currentDateTime().toString(Qt::ISODateWithMs),
            caller,
            fileName,
            comment
//            QString::number(modelIdInt)
    );

    // std::cout << log.toStdString() << std::endl << std::flush;

    appendStringToFile(actionLogPath, log);
}

void VymModel::logWarning(const QString &comment, const QString &caller)
{
    if (!useActionLog) return;

    QString log = QString("\n// %1 [Warning VymModel::%2 \"%3\"] %4").arg(
            QDateTime::currentDateTime().toString(Qt::ISODateWithMs),
            caller,
            fileName,
            comment
//            QString::number(modelIdInt)
    );

    std::cout << log.toStdString() << std::endl << std::flush;

    appendStringToFile(actionLogPath, log);
}

void VymModel::logAction(const QString &command, const QString &comment, const QString &caller)
{
    QString place = QString("\"%1\"").arg(fileName);
    if (!caller.isEmpty()) place += "  Called by: " + caller + "()";

    QString log = QString("\n// %1 [Action] %2\n// MapID: %3 Map: %4").arg(
            QDateTime::currentDateTime().toString(Qt::ISODateWithMs),
            comment,
            QString::number(modelIdInt),
            place);

    if (debug)
        std::cout << log.toStdString() << std::endl << std::flush;

    if (!useActionLog) return;

    appendStringToFile(actionLogPath, log + "\n" + command + "\n");
}
