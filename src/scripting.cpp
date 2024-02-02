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

///////////////////////////////////////////////////////////////////////////
// FIXME-0 Qt6 void logError(QScriptContext *context, QScriptContext::Error error,
//              const QString &text)
void logErrorNew(const QString &text)
{
    /*
    if (context)
        context->throwError(error, text);
    else
    */
        qDebug() << "VymWrapper: " << text;
}

///////////////////////////////////////////////////////////////////////////
VymScriptContext::VymScriptContext() {}

QString VymScriptContext::setResult(const QString &r)
{
    // FIXME-0 Qt6 context()->engine()->globalObject().setProperty("lastResult", r);
    return r;
}

bool VymScriptContext::setResult(bool r)
{
    // FIXME-0 Qt6 context()->engine()->globalObject().setProperty("lastResult", r);
    return r;
}

int VymScriptContext::setResult(int r)
{
    // FIXME-0 Qt6 context()->engine()->globalObject().setProperty("lastResult", r);
    return r;
}

uint VymScriptContext::setResult(uint r)
{
    // FIXME-0 Qt6 context()->engine()->globalObject().setProperty("lastResult", r);
    return r;
}

qreal VymScriptContext::setResult(qreal r)
{
    // FIXME-0 Qt6 context()->engine()->globalObject().setProperty("lastResult", r);
    return r;
}

int VymScriptContext::argumentCount()   // FIXME-0 Qt6 placeholder from QScriptable, no longer available
{
    return 1;
}

QJSValue VymScriptContext::argument(int index)   // FIXME-0 Qt6 placeholder from QScriptable, no longer available
{
    return QJSValue("Foobar");
}

///////////////////////////////////////////////////////////////////////////
VymWrapper::VymWrapper() {}

void VymWrapper::clearConsole() { mainWindow->clearScriptOutput(); }

bool VymWrapper::isConfluenceAgentAvailable()
{
    return setResult(ConfluenceAgent::available());
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
    // FIXME-0 Qt6 context()->engine()->globalObject().setProperty("lastResult",
    //                                                mainWindow->modelCount());
    return setResult(mainWindow->modelCount());
}

void VymWrapper::gotoMap(uint n)
{
    if (!mainWindow->gotoWindow(n)) {
        //logErrorOld(context(), QScriptContext::RangeError,
        logErrorNew(QString("Map '%1' not available.").arg(n));
    }
}

bool VymWrapper::closeMapWithID(uint n)
{
    bool r = mainWindow->closeModelWithID(n);
    if (!r)
        //logErrorOld(context(), QScriptContext::RangeError,
        logErrorNew(QString("Map '%1' not available.").arg(n));
    return setResult(r);
}

void VymWrapper::selectQuickColor(int n)
{
    mainWindow->selectQuickColor(n);
}

QString VymWrapper::currentColor()
{
    return setResult(mainWindow->getCurrentColor().name());
}

uint VymWrapper::currentMapID()
{
    uint id = mainWindow->currentMapID();
    return setResult(id);
}

void VymWrapper::toggleTreeEditor() { mainWindow->windowToggleTreeEditor(); }

QString VymWrapper::loadFile(
    const QString
        &filename) // FIXME-3 error handling missing (in vymmodel and here)
{
    QString s;
    loadStringFromDisk(filename, s);
    return setResult(s);
}

void VymWrapper::saveFile(
    const QString &filename,
    const QString &s) // FIXME-3 error handling missing (in vymmodel and here)
{
    saveStringToDisk(filename, s);
}

QString VymWrapper::version() { return setResult(vymVersion); }

// See also http://doc.qt.io/qt-5/qscriptengine.html#newFunction
Selection::Selection() { modelWrapper = nullptr; }

void Selection::test()
{
    qDebug() << "Selection::testSelection called"; // TODO debug
    if (modelWrapper)
        modelWrapper->setHeadingPlainText("huhu!");
}

void Selection::setModel(VymModelWrapper *mw)
{
    qDebug() << "Selection::setModel called: " << mw; // TODO debug
    modelWrapper = mw;
}
