#include "scripting.h"

#include "vymmodel.h"
#include "mainwindow.h"

extern Main *mainWindow;

VymScriptable::VymScriptable()
{
    ctxt = context();
}

void VymScriptable::throwError(QScriptContext::Error error, const QString &text)
{
    if (ctxt)
        ctxt->throwError( error, text);
    else
        qDebug()<<"VymWrapper: "<<text;
}

VymModelWrapper::VymModelWrapper(VymModel *m)
{
    model = m;
}

void VymModelWrapper::addBranch()
{
    BranchItem *selbi = model->getSelectedBranch();
    if (selbi)
    {
        if (! model->addNewBranch() )
            throwError( QScriptContext::UnknownError, "Couldn't add branch to map");
    } else
        throwError( QScriptContext::ReferenceError, "No branch selected" );
}

void VymModelWrapper::setHeadingPlainText(const QString &s)
{
    model->setHeading(s);
}

QString VymModelWrapper::getHeadingPlainText()
{
    return model->getHeading().getTextASCII(); //FIXME-2 testing
}

QString VymModelWrapper::getFileName()
{
    return model->getFileName();
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
        throwError( QScriptContext::RangeError, QString("Map '%1' not available.").arg(n) );
}

