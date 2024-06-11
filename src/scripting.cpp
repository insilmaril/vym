#include "scripting.h"

#include "branchitem.h"
#include "confluence-agent.h"
#include "imageitem.h"
#include "mainwindow.h"
#include "misc.h"
#include "vymmodelwrapper.h"
#include "vymtext.h"
#include "xlink.h"

#include <QJSValue> 

extern Main *mainWindow;
extern QString vymVersion;

extern QJSEngine *scriptEngine;

extern bool usingDarkTheme;

///////////////////////////////////////////////////////////////////////////
VymScriptContext::VymScriptContext() {}

QString VymScriptContext::setResult(const QString &r)
{
    if (scriptEngine) {
        scriptEngine->globalObject().setProperty("lastResult", r);
    } else
        qWarning() << "VymScriptContext: No scriptEngine defined";
    return r;
}

bool VymScriptContext::setResult(bool r)
{
    if (scriptEngine) {
        scriptEngine->globalObject().setProperty("lastResult", r);
    } else
        qWarning() << "VymScriptContext: No scriptEngine defined";
    return r;
}

int VymScriptContext::setResult(int r)
{
    if (scriptEngine) {
        scriptEngine->globalObject().setProperty("lastResult", r);
    } else
        qWarning() << "VymScriptContext: No scriptEngine defined";
    return r;
}

uint VymScriptContext::setResult(uint r)
{
    if (scriptEngine) {
        scriptEngine->globalObject().setProperty("lastResult", r);
    } else
        qWarning() << "VymScriptContext: No scriptEngine defined";
    return r;
}

qreal VymScriptContext::setResult(qreal r)
{
    if (scriptEngine) {
        scriptEngine->globalObject().setProperty("lastResult", r);
    } else
        qWarning() << "VymScriptContext: No scriptEngine defined";
    return r;
}

///////////////////////////////////////////////////////////////////////////
VymWrapper::VymWrapper() {}

void VymWrapper::abortScript(const QString &s)
{
    mainWindow->statusMessage(s);
    scriptEngine->throwError(QString("abortScript(\"%1\") called").arg(s));
}

void VymWrapper::clearConsole() { mainWindow->clearScriptOutput(); }

bool VymWrapper::closeMapWithID(uint n)
{
    bool r = mainWindow->closeModelWithId(n);
    if (!r) {
        scriptEngine->throwError(
                QJSValue::ReferenceError, 
                QString("Map '%1' not available.").arg(n));
        return false;
    }
    return setResult(r);
}

QString VymWrapper::currentColor()
{
    return setResult(mainWindow->getCurrentColor().name());
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

bool VymWrapper::fileExists(const QString &fileName)
{
    return QFile::exists(fileName);
}

void VymWrapper::gotoMap(uint n)
{
    if (!mainWindow->gotoModelWithId(n)) {
        scriptEngine->throwError(
                QJSValue::ReferenceError, 
                QString("Map '%1' not available.").arg(n));
        return;
    }
}

bool VymWrapper::isConfluenceAgentAvailable()
{
    return setResult(ConfluenceAgent::available());
}

QString VymWrapper::loadFile(
    const QString
        &filename) // FIXME-3 error handling missing (in vymmodel and here)
{
    QString s;
    loadStringFromDisk(filename, s);
    return setResult(s);
}

bool VymWrapper::loadMap(const QString &filename)
{
    bool r;
    if (File::Success == mainWindow->fileLoad(filename, File::NewMap, File::VymMap))
        r = true;
    else
        r = false;
    return setResult(r);
}

int VymWrapper::mapCount()
{
    return setResult(mainWindow->modelCount());
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
    uint id = mainWindow->currentMapId();
    return setResult(id);
}

void VymWrapper::toggleTreeEditor() { mainWindow->windowToggleTreeEditor(); }

void VymWrapper::saveFile(
    const QString &filename,
    const QString &s) // FIXME-3 error handling missing (in vymmodel and here)
{
    saveStringToDisk(filename, s);
}

bool VymWrapper::usesDarkTheme() {
    return usingDarkTheme;
}

QString VymWrapper::version() {
    qDebug() << "VymWrapper::version  v=" << vymVersion;
    return setResult(vymVersion);
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
