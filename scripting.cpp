#include "scripting.h"

#include "branchitem.h"
#include "imageitem.h"
#include "mainwindow.h"
#include "misc.h"
#include "vymmodel.h"
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
    if ( File::Success == mainWindow->fileLoad( filename, NewMap, VymMap ) )
        return true;
    else
        return false;
}

int VymWrapper::mapCount()
{
    context()->engine()->globalObject().setProperty("lastResult", mainWindow->modelCount() );
    return mainWindow->modelCount();
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

QString VymWrapper::version()
{
    return setResult( vymVersion );
}

