#include "vym-wrapper.h"

#include <QJSValue> 

#include "branchitem.h"
#include "confluence-agent.h"
#include "imageitem.h"
#include "mainwindow.h"
#include "mapeditor.h"
#include "misc.h"
#include "vymtext.h"
#include "xlink.h"


extern Main *mainWindow;
extern QString vymVersion;

extern bool usingDarkTheme;

#include "vymmodelwrapper.h"

VymWrapper::VymWrapper()
{
    // qDebug() << "Constr. VymWrapper";
}

void VymWrapper::clearConsole() { mainWindow->clearScriptOutput(); }

bool VymWrapper::closeMapWithID(uint n)
{
    bool r = mainWindow->closeModelWithId(n);
    if (!r) {
        mainWindow->abortScript(
                QJSValue::ReferenceError, 
                QString("Map '%1' not available.").arg(n));
        return false;
    }
    mainWindow->setScriptResult(r);
    return r;
}

QString VymWrapper::currentColor()
{
    QString r = mainWindow->getCurrentColor().name();
    mainWindow->setScriptResult(r);
    return r;
}

QObject *VymWrapper::currentMap()
{
    return mainWindow->getCurrentModelWrapper();
}

void VymWrapper::editHeading()
{
    MapEditor *me = mainWindow->currentMapEditor();
    if (me) me->editHeading();
}

bool VymWrapper::directoryIsEmpty(const QString &directoryName)
{
    QDir d(directoryName);
    return d.isEmpty();
}

bool VymWrapper::directoryExists(const QString &directoryName)
{
    QDir d(directoryName);
    return d.exists();
}

bool VymWrapper::fileCopy(const QString &srcPath, const QString &dstPath)
{
    QFile file(srcPath);
    bool r; 
    if (!file.exists()) {
        qDebug() << "VymWrapper::fileCopy()   srcPath does not exist:" << srcPath;
        mainWindow->abortScript(
                QJSValue::ReferenceError, 
                QString("File '%1' does not exist.").arg(srcPath));
        r = false;
    } else
        r = file.copy(dstPath);

    mainWindow->setScriptResult(r);
    return r;
}

bool VymWrapper::fileExists(const QString &fileName)
{
    bool r = QFile::exists(fileName);
    mainWindow->setScriptResult(r);
    return r;
}

bool VymWrapper::fileRemove(const QString &fileName)
{
    QFile file(fileName);
    bool r = file.remove();
    mainWindow->setScriptResult(r);
    return r;
}

void VymWrapper::gotoMap(uint n)
{
    if (!mainWindow->gotoModelWithId(n)) {
        mainWindow->abortScript(
                QJSValue::ReferenceError, 
                QString("Map '%1' not available.").arg(n));
        return;
    }
}

bool VymWrapper::isConfluenceAgentAvailable()
{
    bool r = ConfluenceAgent::available();
    mainWindow->setScriptResult(r);
    return r;
}

QString VymWrapper::loadFile(
    const QString
        &filename) // FIXME-3 error handling missing (in vymmodel and here)
{
    QString r;
    loadStringFromDisk(filename, r);
    mainWindow->setScriptResult(r);
    return r;
}

bool VymWrapper::loadMap(const QString &filename)
{
    bool r;
    if (File::Success == mainWindow->fileLoad(filename, File::NewMap, File::VymMap))
        r = true;
    else
        r = false;
    mainWindow->setScriptResult(r);
    return r;
}

int VymWrapper::mapCount()
{
    int r = mainWindow->modelCount();
    mainWindow->setScriptResult(r);
    return r;
}

bool VymWrapper::mkdir(const QString &directoryName)
{
    QDir d;
    return d.mkpath(directoryName);
}

void VymWrapper::print(const QString &s)
{
    mainWindow->scriptPrint(s);
}

bool VymWrapper::removeDirectory(const QString &directoryName)
{
    QDir d(directoryName);
    qWarning() << "VW::removeDir " << directoryName;
    return false;
    return d.removeRecursively();
}

bool VymWrapper::removeFile(const QString &fileName)
{
    return QFile::remove(fileName);
}

void VymWrapper::statusMessage(const QString &s)
{
    mainWindow->statusMessage(s);
}

void VymWrapper::selectQuickColor(int n)
{
    mainWindow->selectQuickColor(n);
}

uint VymWrapper::currentMapID()
{
    uint r = mainWindow->currentMapId();
    mainWindow->setScriptResult(r);
    return r;
}

void VymWrapper::toggleTreeEditor() { mainWindow->windowToggleTreeEditor(); }

void VymWrapper::saveFile(
    const QString &filename,
    const QString &s) // FIXME-3 error handling missing (in vymmodel and here)
{
    saveStringToDisk(filename, s);
}

bool VymWrapper::usesDarkTheme() {
    mainWindow->setScriptResult(usingDarkTheme);
    return usingDarkTheme;
}

QString VymWrapper::version() {
    qDebug() << "VymWrapper::version  v=" << vymVersion;
    QString r = vymVersion;
    mainWindow->setScriptResult(r);
    return r;
}

// See also http://doc.qt.io/qt-5/qscriptengine.html#newFunction
Selection::Selection() { modelWrapper = nullptr; }

void Selection::test()
{
    qDebug() << "Selection::testSelection called"; // TODO debug
    /*
    if (modelWrapper)
        modelWrapper->setHeadingPlainText("huhu!");
    */
}

void Selection::setModel(VymModelWrapper *mw)
{
    qDebug() << "Selection::setModel called: " << mw; // TODO debug
    modelWrapper = mw;
}
