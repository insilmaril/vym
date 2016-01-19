#include "scripting.h"

#include "vymmodel.h"
#include "mainwindow.h"

extern Main *mainWindow;

///////////////////////////////////////////////////////////////////////////
void logError(QScriptContext *context, QScriptContext::Error error, const QString &text)
{
    if (context)
        context->throwError( error, text);
    else
        qDebug()<<"VymWrapper: "<<text;
}

///////////////////////////////////////////////////////////////////////////
VymModelWrapper::VymModelWrapper(VymModel *m)
{
    model = m;
}

BranchItem*  VymModelWrapper::getSelectedBranch()
{
    BranchItem *selbi = model->getSelectedBranch();
    if (selbi)
    {
        if (! model->addNewBranch() )
            logError( context(), QScriptContext::UnknownError, "Couldn't add branch to map");
    } else
        logError( context(),  QScriptContext::ReferenceError, "No branch selected" );
}

void VymModelWrapper::addBranch()
{
    BranchItem *selbi = model->getSelectedBranch();
    if (selbi)
    {
        if (! model->addNewBranch() )
            logError( context(), QScriptContext::UnknownError, "Couldn't add branch to map");
    } else
        logError( context(), QScriptContext::ReferenceError, "No branch selected" );
}

void VymModelWrapper::copy()
{
    model->copy();
}

void VymModelWrapper::cut()
{
    model->cut();
}

QString VymModelWrapper::getFileName()
{
    return model->getFileName();
}

QString VymModelWrapper::getHeadingPlainText()
{
    return model->getHeading().getTextASCII(); //FIXME-2 testing
}

void VymModelWrapper::paste()
{
    model->paste();
}

void VymModelWrapper::setHeadingPlainText(const QString &s)
{
    model->setHeading(s);
}

///////////////////////////////////////////////////////////////////////////
VymWrapper::VymWrapper()
{
}

void VymWrapper::toggleTreeEditor()
{
    mainWindow->windowToggleTreeEditor();
}

QObject* VymWrapper::getCurrentMap()    // FIXME-1 No syntax highlighting
{
    return mainWindow->getCurrentModelWrapper();
}

void VymWrapper::selectMap(uint n)      // FIXME-1 No syntax highlighting
{
    if ( !mainWindow->gotoWindow( n ))
    {
        logError( context(), QScriptContext::RangeError, QString("Map '%1' not available.").arg(n) );
    }
}

