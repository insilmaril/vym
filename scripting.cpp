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

QVariant VymModelWrapper::getParameter( bool &ok, const QString &key, const QStringList &parameters )
{
    // loop through parameters and try to find the one named "key"
    foreach ( QString par, parameters)
    {
        if ( par.startsWith( key ) )
        {
            qDebug()<<"getParam: "<<key<<"  has: "<< par;
            ok = true;
            return QVariant( par );
        }
    }

    // Nothing found
    ok = false;
    return QVariant::Invalid;
}

void VymModelWrapper::addBranch()
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi)
    {
        if (argumentCount() > 1 )
        {
            logError( context(), QScriptContext::SyntaxError, "Too many arguments");
            return;
        }

        int pos = -2;
        if (argumentCount() == 1 )
        {
            pos = argument(0).toInteger();
        }

        if (! model->addNewBranch( selbi, pos ) )
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

void VymModelWrapper::addXLink( const QString &begin, const QString &end, int width, const QString &color, const QString &penstyle)
{
    BranchItem *bbegin = (BranchItem*)(model->findBySelectString( begin ) );
    BranchItem *bend = (BranchItem*)(model->findBySelectString( end ) );
    if (bbegin && bend)
    {
        if (bbegin->isBranchLikeType() && bend->isBranchLikeType())
        {
            Link *li = new Link ( model );
            li->setBeginBranch ( (BranchItem*)bbegin );
            li->setEndBranch ( (BranchItem*)bend);

            model->createLink (li);
            QPen pen = li->getPen();
            if (width > 0 ) pen.setWidth( width );
            QColor col (color);
            if (col.isValid())
                pen.setColor ( col );
            else
            {
                logError( context(), QScriptContext::UnknownError, QString( "Could not set color to %1").arg(color) );
                return;
            }

            bool ok;
            Qt::PenStyle st1 = penStyle ( penstyle, ok);
            if (ok) 
            {
                pen.setStyle ( st1 );
                li->setPen( pen );	
            } else	
                logError( context(), QScriptContext::UnknownError, QString("Couldn't set penstyle %1").arg(penstyle));
        }
        else
            logError( context(), QScriptContext::UnknownError, "Begin or end of xLink are not branch or mapcenter");
        
    } else
        logError( context(), QScriptContext::UnknownError, "Begin or end of xLink not found");
}

int VymModelWrapper::branchCount()
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi)
        return selbi->branchCount();
    else
        return -1;
}

int VymModelWrapper::centerCount()
{
    return model->centerCount();
}

void VymModelWrapper::centerOnID( const QString &id)
{
    if ( !model->centerOnID( id ) ) 
        logError( context(), QScriptContext::UnknownError, QString("Could not center on ID %1").arg(id) );
}

void VymModelWrapper::clearFlags()
{
    return model->clearFlags();
}

void VymModelWrapper::colorBranch( const QString &color)
{
    QColor col(color);
    if ( !col.isValid() )
        logError( context(), QScriptContext::SyntaxError, QString( "Could not set color to %1").arg(color) );
    else
        model->colorBranch( col );
}

void VymModelWrapper::colorSubtree( const QString &color)
{
    QColor col(color);
    if ( !col.isValid() )
        logError( context(), QScriptContext::SyntaxError, QString( "Could not set color to %1").arg(color) );
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

void VymModelWrapper::cycleTask()
{
    if ( !model->cycleTaskStatus() )
        logError( context(), QScriptContext::SyntaxError, "Couldn't cycle task status");
}

bool VymModelWrapper::exportMap( )
{
    if (argumentCount() == 0 )
    {
        logError( context(), QScriptContext::SyntaxError, "Not enough arguments");
        return false;
    }
    
    QString format;
    format = argument(0).toString();

    if (argumentCount() == 1 )
    {
        if ( format == "Last")
        {
            model->exportLast();
            return true;
        } else
        {
            logError( context(), QScriptContext::SyntaxError, "Filename missing" );
            return false;
        }
    }
    
    QString filename;

    filename = argument(1).toString();

    if (format == "AO") 
    {
        model->exportAO (filename, false);
    } else if ( format == "ASCII" ) 
    {
        bool listTasks = false;
        if (argumentCount() == 3 && argument(2).toString() == "listTasks")
            listTasks = true;
        model->exportASCII (listTasks, filename, false);
    } else if ( format == "CSV" )
    {
        model->exportCSV (filename, false);
    } else if ( format == "HTML" )
    {
        if (argumentCount() < 3 )
        {
            logError( context(), QScriptContext::SyntaxError, "Path missing in HTML export" );
            return false;
        }   
        QString path = argument(2).toString();
        model->exportHTML (path, filename, false);
    } else if ( format == "Image" )
    {
        QString imgFormat; 
        if (argumentCount() == 2 )
            imgFormat = "PNG";
        else if (argumentCount() == 3 )
            imgFormat = argument(2).toString();

        QStringList formats;
        formats << "PNG"; 
        formats << "GIF"; 
        formats << "JPG"; 
        formats << "JPEG", 
        formats << "PNG", 
        formats << "PBM", 
        formats << "PGM", 
        formats << "PPM", 
        formats << "TIFF", 
        formats << "XBM", 
        formats << "XPM";
        if ( formats.indexOf( imgFormat ) < 0 )
        {
            logError( context(), QScriptContext::SyntaxError, QString("%1 not one of the known export formats: ").arg(imgFormat).arg(formats.join(",") ) );
            return false;
        }
        model->exportImage ( filename, false, imgFormat);
    } else if ( format == "Impress" )
    {
        if (argumentCount() < 3 )
        {
            logError( context(), QScriptContext::SyntaxError, "Template file  missing in export to Impress" );
            return false;
        }   
        QString templ = argument(2).toString();
        model->exportImpress (filename, templ);
    } else if ( format == "LaTeX" )
    {
        model->exportLaTeX (filename, false);
    } else if ( format == "OrgMode" )
    {
        model->exportOrgMode ( filename,false);
    } else if ( format == "PDF" )
    {
        model->exportPDF( filename, false);
    } else if ( format == "SVG" )
    {
        model->exportPDF( filename, false);
    } else if ( format == "XML" )
    {
        if (argumentCount() < 3 )
        {
            logError( context(), QScriptContext::SyntaxError, "path missing in export to Impress" );
            return false;
        }   
        QString path = argument(2).toString();
        model->exportXML (path, filename, false);
    } else
    {
        logError( context(), QScriptContext::SyntaxError, QString("Unknown export format: %1").arg(format) );
        return false;
    }
    return true;
}

QString VymModelWrapper::getDestPath()
{
    return model->getDestPath();
}

QString VymModelWrapper::getFileDir()
{
    return model->getFileDir();
}

QString VymModelWrapper::getFileName()
{
    return model->getFileName();
}

QString VymModelWrapper::getFrameType()
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi)
    {
        BranchObj *bo = (BranchObj*)(selbi->getLMO());
        if (!bo)
            logError( context(), QScriptContext::UnknownError, QString("No BranchObj available") );
        else
            return bo->getFrame()->getFrameTypeName();
    } 
    return QString();
}

QString VymModelWrapper::getHeadingPlainText()
{
    return model->getHeading().getTextASCII();
}

QString VymModelWrapper::getHeadingXML()
{
    return model->getHeading().saveToDir();
}

QString VymModelWrapper::getMapAuthor()
{
    return model->getAuthor();
}

QString VymModelWrapper::getMapComment()
{
    return model->getComment();
}

QString VymModelWrapper::getMapTitle()
{
    return model->getTitle();
}

QString VymModelWrapper::getNotePlainText()
{
    return model->getHeading().getTextASCII(); 
}

QString VymModelWrapper::getNoteXML()
{
    return model->getHeading().saveToDir(); 
}

QString VymModelWrapper::getSelectString()
{
    return model->getSelectString();
}

int VymModelWrapper::getTaskSleepDays()
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi)
    {
        Task *task = selbi->getTask();
        if (task)
            return task->getDaysSleep();
        else
            logError( context(), QScriptContext::UnknownError, "Branch has no task");
    }
    return -1;
}

QString VymModelWrapper::getURL()
{
    return model->getURL();
}

QString VymModelWrapper::getVymLink()
{
    return model->getVymLink();
}

QString VymModelWrapper::getXLinkColor()
{
    return model->getXLinkColor().name();
}

int VymModelWrapper::getXLinkWidth()
{
    return model->getXLinkWidth();
}

QString VymModelWrapper::getXLinkPenStyle()
{
    return penStyleToString( model->getXLinkPenStyle() );
}

QString VymModelWrapper::getXLinkStyleBegin()
{
    return model->getXLinkStyleBegin();
}

QString VymModelWrapper::getXLinkStyleEnd()
{
    return model->getXLinkStyleEnd();
}

bool VymModelWrapper::hasActiveFlag( const QString &flag)
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi)
        return selbi->hasActiveStandardFlag( flag );
    else
        return false;
}

bool VymModelWrapper::hasNote()
{
    return !model->getNote().isEmpty();
}

bool VymModelWrapper::hasRichTextNote()
{
    return model->hasRichTextNote();
}

bool VymModelWrapper::hasTask()
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi)
    {
        Task *task = selbi->getTask();
        if (task)
            return true;
        else
            logError( context(), QScriptContext::UnknownError, "Branch has no task");
    }
    return false;
}

void VymModelWrapper::importDir( const QString &path)
{
    model->importDir( path );    // FIXME-1 error handling missing (in vymmodel and here)
}

bool VymModelWrapper::isScrolled()
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi)
    {
        return selbi->isScrolled();
    }
    return false;
}

void VymModelWrapper::loadImage( const QString &filename)
{
    BranchItem *selbi = getSelectedBranch();
    if (selbi)
    {
        model->loadImage(selbi, filename );   // FIXME-1 error handling missing (in vymmodel and here)
    }
}

void VymModelWrapper::loadNote(  const QString &filename)
{
    model->loadNote( filename );    // FIXME-1 error handling missing (in vymmodel and here)
}

void VymModelWrapper::move( qreal x, qreal y)
{
    model->move(x, y);;
}

void VymModelWrapper::moveRel( qreal x, qreal y)
{
    model->moveRel(x, y);;
}

void VymModelWrapper::moveDown()
{
    model->moveDown();
}

void VymModelWrapper::moveUp()
{
    model->moveUp();
}

void VymModelWrapper::moveSlideDown( int n)
{
    if (! model->moveSlideDown( n ) )
        logError( context(), QScriptContext::UnknownError, "Could not move slide down");
}

void VymModelWrapper::moveSlideDown()
{
    moveSlideDown( -1 );
}

void VymModelWrapper::moveSlideUp( int n)
{
    if (! model->moveSlideUp( n ) )
        logError( context(), QScriptContext::UnknownError, "Could not move slide up");
}

void VymModelWrapper::moveSlideUp()
{
    moveSlideUp( -1 );
}

void VymModelWrapper::nop() {}

void VymModelWrapper::note2URLs()
{
    model->note2URLs();
}

void VymModelWrapper::parseVymText(const QString &text)
{
    model->parseVymText( text );
}

void VymModelWrapper::paste()
{
    model->paste();
}

void VymModelWrapper::redo()
{
    model->redo();
}

bool VymModelWrapper::relinkTo( const QString &parent, int num, qreal x, qreal y)
{
    if ( ! model->relinkTo( parent, num, QPointF( x, y ) ) )
    {
        logError( context(), QScriptContext::UnknownError, "Could not relink" );
        return false;
    } else
        return true;
}

bool VymModelWrapper::relinkTo( const QString &parent, int num)
{
    return relinkTo( parent, num, 0, 0);
}

bool VymModelWrapper::relinkTo( const QString &parent)
{
    return relinkTo( parent, -1, 0, 0);
}

void VymModelWrapper::remove()
{
    model->deleteSelection();
}

void VymModelWrapper::removeChildren()
{
    model->deleteChildren();
}

void VymModelWrapper::removeKeepChildren()
{
    model->deleteKeepChildren();
}

void VymModelWrapper::removeSlide(int n)
{
    if ( n < 0 || n >= model->slideCount() - 1)
        logError( context(), QScriptContext::RangeError, QString("Slide '%1' not available.").arg(n) );
}

void VymModelWrapper::saveImage( const QString &filename, const QString &format)
{
    model->saveImage( NULL, format, filename);
}

void VymModelWrapper::saveNote( const QString &filename)
{
    model->saveNote( filename );
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

bool VymModelWrapper::selectFirstChildBranch()
{
    bool r = false;
    BranchItem *selbi = getSelectedBranch();
    if (selbi)
    {
        r = model->selectFirstChildBranch();
        if (!r) logError( context(), QScriptContext::UnknownError, "Couldn't select first child branch");
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

bool VymModelWrapper::selectLastChildBranch()
{
    bool r = false;
    BranchItem *selbi = getSelectedBranch();
    if (selbi)
    {
        r = model->selectLastChildBranch();
        if (!r) logError( context(), QScriptContext::UnknownError, "Couldn't select last child branch");
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

void VymModelWrapper::setHeadingPlainText( const QString &text)// FIXME-2  what about RT?
{
    model->setHeadingPlainText( text );
}

void VymModelWrapper::setHideExport( bool b)
{
    model->setHideExport( b );
}

void VymModelWrapper::setIncludeImagesHorizontally( bool b)
{
    model->setIncludeImagesHor( b );
}

void VymModelWrapper::setIncludeImagesVertically( bool b)
{
    model->setIncludeImagesVer( b );
}

void VymModelWrapper::setHideLinkUnselected( bool b)
{
    model->setHideLinkUnselected( b );
}


void VymModelWrapper::setMapAnimCurve( int n )
{
    if ( n < 0 || n > QEasingCurve::OutInBounce)
        logError( context(), QScriptContext::RangeError, "Unknown animation curve type");
    else
    {
        QEasingCurve c;
        c.setType ( (QEasingCurve::Type) n);
        model->setMapAnimCurve( c );
    }

}

void VymModelWrapper::setMapAnimDuration( int n )
{
    model->setMapAnimDuration( n );
}

void VymModelWrapper::setMapAuthor(const QString &s)
{
    model->setAuthor( s );
}

void VymModelWrapper::setMapBackgroundColor( const QString &color)
{
    QColor col (color);
    if (col.isValid())
    {
        model->setMapBackgroundColor( col );
    } else
        logError( context(), QScriptContext::UnknownError, QString( "Could not set color to %1").arg(color) );
}

void VymModelWrapper::setMapComment(const QString &s)
{
    model->setComment( s );
}

void VymModelWrapper::setMapDefLinkColor( const QString &color)
{
    QColor col (color);
    if (col.isValid())
    {
        model->setMapDefLinkColor( col );
    } else
        logError( context(), QScriptContext::UnknownError, QString( "Could not set color to %1").arg(color) );
}

void VymModelWrapper::setMapLinkStyle(const QString &style)
{
    if (! model->setMapLinkStyle( style ) )
        logError( context(), QScriptContext::UnknownError, QString( "Could not set linkstyle to %1").arg(style) );
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

void VymModelWrapper::setFrameBorderWidth( int width )
{
    return model->setFrameBorderWidth( width );
}

void VymModelWrapper::setFrameBrushColor( const QString &color)
{
    return model->setFrameBrushColor( color );
}

void VymModelWrapper::setFrameIncludeChildren( bool b )
{
    model->setFrameIncludeChildren(b);
}

void VymModelWrapper::setFramePadding( int padding )
{
    return model->setFramePadding( padding );
}

void VymModelWrapper::setFramePenColor( const QString &color)
{
    return model->setFramePenColor( color );
}

void VymModelWrapper::setFrameType( const QString &type)
{
    model->setFrameType( type );
}

void VymModelWrapper::setScale( qreal x, qreal y)
{
    model->setScale( x, y);
}

void VymModelWrapper::setSelectionColor( const QString &color)
{
    QColor col(color);
    if ( !col.isValid() )
        logError( context(), QScriptContext::SyntaxError, QString( "Could not set color to %1").arg(color) );
    else
        model->setSelectionColor( col );
}

bool VymModelWrapper::setTaskSleep(const QString &s)
{
    return model->setTaskSleep( s );
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

void VymModelWrapper::setXLinkColor( const QString &color)
{
    QColor col(color);
    if ( !col.isValid() )
        logError( context(), QScriptContext::SyntaxError, QString( "Could not set color to %1").arg(color) );
    else
        // model->setXlinkColor( col );
        model->setXLinkColor( color);     // FIXME-2 try to use QColor here...
}

void VymModelWrapper::setXlinkLineStyle( const QString &style)
{
    model->setXLinkLineStyle( style );
}

void VymModelWrapper::setXlinkStyleBegin( const QString &style)
{
    model->setXLinkStyleBegin( style );
}

void VymModelWrapper::setXlinkStyleEnd( const QString &style)
{
    model->setXLinkStyleEnd( style );
}

void VymModelWrapper::setXlinkWidth( int w )
{
    model->setXLinkWidth( w );
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

