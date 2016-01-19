#include "scripting.h"

#include "branchitem.h"
#include "imageitem.h"
#include "mainwindow.h"
#include "vymmodel.h"
#include "vymtext.h"

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
    if (!selbi) logError( context(),  QScriptContext::ReferenceError, "No branch selected" );
    return selbi;
}

void VymModelWrapper::addBranch()
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi)
    {
        if (! model->addNewBranch() )
            logError( context(), QScriptContext::UnknownError, "Couldn't add branch to map");
    } 
}

void VymModelWrapper::addBranchBefore()
{
    if (! model->addNewBranchBefore() )
        logError( context(), QScriptContext::UnknownError, "Couldn't add branch before selection to map");
}

void VymModelWrapper::addMapCenter( qreal x, qreal y)
{
    if (! model->addMapCenter( QPointF (x, y) ) )
        logError( context(), QScriptContext::UnknownError, "Couldn't add mapcenter");
}

void VymModelWrapper::addMapInsert( QString fileName, int pos, int contentFilter)
{
    if (QDir::isRelativePath( fileName )) 
        fileName = QDir::currentPath() + "/" + fileName;

    model->saveStateBeforeLoad (ImportAdd, fileName);

    if (File::Aborted == model->loadMap (fileName, ImportAdd, VymMap, contentFilter, pos) )
        logError( context(), QScriptContext::UnknownError, QString( "Couldn't load %1").arg(fileName) );
}

void VymModelWrapper::addMapInsert( const QString &fileName, int pos)
{
    addMapInsert( fileName, pos, 0x0000);
}

void VymModelWrapper::addMapInsert( const QString &fileName)
{
    addMapInsert( fileName, -1, 0x0000);
}

void VymModelWrapper::addMapReplace( QString fileName )
{
    if (QDir::isRelativePath( fileName )) 
        fileName = QDir::currentPath() + "/" + fileName;

    model->saveStateBeforeLoad (ImportReplace, fileName);

    if (File::Aborted == model->loadMap (fileName, ImportReplace, VymMap) )
        logError( context(), QScriptContext::UnknownError, QString( "Couldn't load %1").arg(fileName) );
}

void VymModelWrapper::addSlide()
{
    model->addSlide();
}

void VymModelWrapper::addXLink( const QString &begin, const QString &end, int width, QColor color, const QString &penstyle)
{
    // FIXME-0 missing
}

void VymModelWrapper::colorBranch( const QString &color)
{
    QColor col(color);
    if ( !col.isValid() )
        logError( context(), QScriptContext::SyntaxError, QString( "Couldn't parse color %1").arg(color) );
    else
        model->colorBranch( col );
}

void VymModelWrapper::colorSubtree( const QString &color)
{
    QColor col(color);
    if ( !col.isValid() )
        logError( context(), QScriptContext::SyntaxError, QString( "Couldn't parse color %1").arg(color) );
    else
        model->colorSubtree( col );
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
    return model->getHeading().getTextASCII();
}

QString VymModelWrapper::getNotePlainText()
{
    return model->getHeading().getTextASCII(); 
}

void VymModelWrapper::moveDown()
{
    model->moveDown();
}

void VymModelWrapper::moveUp()
{
    model->moveUp();
}

void VymModelWrapper::nop() {}

void VymModelWrapper::paste()
{
    model->paste();
}

void VymModelWrapper::redo()
{
    model->redo();
}

void VymModelWrapper::scroll()
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi)
    {
        if (! model->scrollBranch(selbi) )
            logError( context(), QScriptContext::UnknownError, "Couldn't scroll branch");
    } 
}

bool VymModelWrapper::select(const QString &s)
{
    bool r = model->select( s );
    if (!r) logError( context(), QScriptContext::UnknownError, QString("Couldn't select %1").arg(s));
    return r;
}

bool VymModelWrapper::selectID(const QString &s)
{
    bool r = model->selectID( s );
    if (!r) logError( context(), QScriptContext::UnknownError, QString("Couldn't select ID %1").arg(s));
    return r;
}

bool VymModelWrapper::selectFirstBranch()
{
    bool r = false;
    BranchItem *selbi = getSelectedBranch();
    if (selbi)
    {
        r = model->selectFirstBranch();
        if (!r) logError( context(), QScriptContext::UnknownError, "Couldn't select first branch");
    }
    return r;
}

bool VymModelWrapper::selectLastBranch()
{
    bool r = false;
    BranchItem *selbi = getSelectedBranch();
    if (selbi)
    {
        r = model->selectLastBranch();
        if (!r) logError( context(), QScriptContext::UnknownError, "Couldn't select last branch");
    }
    return r;
}

bool VymModelWrapper::selectLastImage()
{
    bool r = false;
    BranchItem *selbi = getSelectedBranch();
    if (selbi)
    {
        ImageItem* ii = selbi->getLastImage();
        if (!ii) 
            logError( context(), QScriptContext::UnknownError, "Couldn't get last image");
        else
        {
            r = model->select( ii );
            if (!r) 
                logError( context(), QScriptContext::UnknownError, "Couldn't select last image");
        }
    }
    return r;
}

bool VymModelWrapper::selectParent()
{
    bool r = model->selectParent();
    if (!r) 
        logError( context(), QScriptContext::UnknownError, "Couldn't select parent item");
    return r;
}

bool VymModelWrapper::selectLatestAdded()
{
    bool r =  model->selectLatestAdded();
    if (!r) 
        logError( context(), QScriptContext::UnknownError, "Couldn't select latest added item");
    return r;
}

void VymModelWrapper::setFlag(const QString &s)
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi) selbi->activateStandardFlag( s );
}

void VymModelWrapper::setHeadingPlainText(const QString &s)
{
    model->setHeading( s );
}

void VymModelWrapper::setMapAuthor(const QString &s)
{
    model->setAuthor( s );
}

void VymModelWrapper::setMapComment(const QString &s)
{
    model->setComment( s );
}

void VymModelWrapper::setMapRotation( float a)
{
    model->setMapRotationAngle( a );
}

void VymModelWrapper::setMapTitle(const QString &s)
{
    model->setTitle( s );
}

void VymModelWrapper::setMapZoom( float z)
{
    model->setMapZoomFactor( z );
}

void VymModelWrapper::setNotePlainText(const QString &s)
{
    VymNote vn;
    vn.setPlainText(s);
    model->setNote (vn);
}

void VymModelWrapper::setURL(const QString &s)
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi) selbi->setURL( s );
}

void VymModelWrapper::setVymLink(const QString &s)
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi) selbi->setVymLink( s );
}

void VymModelWrapper::sleep( int n)
{
    sleep( n );
}

void VymModelWrapper::sortChildren(bool b)
{
    model->sortChildren( b );
}

void VymModelWrapper::sortChildren()
{
    sortChildren( false );
}

void VymModelWrapper::toggleFlag(const QString &s)
{
    model->toggleStandardFlag( s );
}

void VymModelWrapper::toggleFrameIncludeChildren()
{
    model->toggleFrameIncludeChildren();
}

void VymModelWrapper::toggleScroll()
{
    model->toggleScroll();
}

void VymModelWrapper::toggleTarget()
{
    model->toggleTarget();
}

void VymModelWrapper::toggleTask()
{
    model->toggleTask();
}

void VymModelWrapper::undo()
{
    model->undo();
}

bool VymModelWrapper::unscroll()
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi)
    {
        if ( model->unscrollBranch(selbi) )
            return true;
        else
            logError( context(), QScriptContext::UnknownError, "Couldn't unscroll branch");
    } 
    return false;
}

void VymModelWrapper::unscrollChildren()
{
    model->unscrollChildren();
}

void VymModelWrapper::unselectAll()
{
    model->unselectAll();
}

void VymModelWrapper::unsetFlag(const QString &s)
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi) selbi->deactivateStandardFlag( s );
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

