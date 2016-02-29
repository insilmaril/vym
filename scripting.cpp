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
    return vymVersion;
}

