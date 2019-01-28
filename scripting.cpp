#include "scripting.h"

#include "branchitem.h"
#include "imageitem.h"
#include "mainwindow.h"
#include "misc.h"
#include "vymmodelwrapper.h"
#include "vymtext.h"
#include "xlink.h"

extern Main *mainWindow;
extern QString vymVersion;

///////////////////////////////////////////////////////////////////////////
void logError(QScriptContext *context, QScriptContext::Error error, const QString &text)
{
    if (context)
        context->throwError( error, text);
    else
        qDebug()<<"VymWrapper: "<<text;
}

///////////////////////////////////////////////////////////////////////////
VymScriptContext::VymScriptContext()
{
}

QString VymScriptContext::setResult( const QString &r)
{
    context()->engine()->globalObject().setProperty("lastResult", r );
    return r;
}

bool VymScriptContext::setResult( bool r)
{
    context()->engine()->globalObject().setProperty("lastResult", r );
    return r;
}

int  VymScriptContext::setResult( int r)
{
    context()->engine()->globalObject().setProperty("lastResult", r );
    return r;
}

///////////////////////////////////////////////////////////////////////////
VymWrapper::VymWrapper()
{
}


void VymWrapper::clearConsole()
{
    mainWindow->clearScriptOutput();
}

QObject* VymWrapper::currentMap()   
{
    return mainWindow->getCurrentModelWrapper();
}

bool VymWrapper::loadMap( const QString &filename )
{
    bool r;
    if ( File::Success == mainWindow->fileLoad( filename, NewMap, VymMap ) )
        r = true;
    else
        r = false;
    return setResult( r );
}

int VymWrapper::mapCount()
{
    context()->engine()->globalObject().setProperty("lastResult", mainWindow->modelCount() );
    return setResult( mainWindow->modelCount() );
}

void VymWrapper::selectMap(uint n)   
{
    if ( !mainWindow->gotoWindow( n ))
    {
        logError( context(), QScriptContext::RangeError, QString("Map '%1' not available.").arg(n) );
    }
}
void VymWrapper::toggleTreeEditor()
{
    mainWindow->windowToggleTreeEditor();
}

QString VymWrapper::loadFile( const QString &filename) // FIXME-3 error handling missing (in vymmodel and here)
{
    QString s;
    loadStringFromDisk(filename, s);
    return s;
}

void VymWrapper::saveFile( const QString &filename, const QString &s) // FIXME-3 error handling missing (in vymmodel and here)
{
    saveStringToDisk(filename, s);
}

QString VymWrapper::version()
{
    return setResult( vymVersion );
}


// See also http://doc.qt.io/qt-5/qscriptengine.html#newFunction
Selection::Selection()
{
    modelWrapper = NULL;
}

void Selection::test()
{
    qDebug() << "Selection::testSelection called"; // TODO debug
    if (modelWrapper) modelWrapper->setHeadingPlainText("huhu!");
}

void Selection::setModel(VymModelWrapper *mw)
{
    qDebug() << "Selection::setModel called: " << mw; // TODO debug
    modelWrapper = mw;
}

