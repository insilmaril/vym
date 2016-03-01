#include "vymmodelwrapper.h"

#include "branchitem.h"
#include "branchobj.h"
#include "imageitem.h"
#include "misc.h"
#include "scripting.h"
#include "vymmodel.h"
#include "vymtext.h"
#include "xlink.h"


///////////////////////////////////////////////////////////////////////////
VymModelWrapper::VymModelWrapper(VymModel *m)
{
    model = m;
}

QVariant VymModelWrapper::lastResult()
{
    return result;
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
    int r;
    BranchItem *selbi = getSelectedBranch();
    if (selbi)
        r = selbi->branchCount();
    else
        r = -1;
    result.setValue( r );
    return r;
}

int VymModelWrapper::centerCount()
{
    int r = model->centerCount();
    result.setValue( r );
    return r;
}

void VymModelWrapper::centerOnID( const QString &id)
{
    if ( !model->centerOnID( id ) ) 
        logError( context(), QScriptContext::UnknownError, QString("Could not center on ID %1").arg(id) );
}

void VymModelWrapper::clearFlags()
{
    model->clearFlags();
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
    bool r = false;
    result.setValue(false);

    if (argumentCount() == 0 )
    {
        logError( context(), QScriptContext::SyntaxError, "Not enough arguments");
        return r;
    }
    
    QString format;
    format = argument(0).toString();

    if (argumentCount() == 1 )
    {
        if ( format == "Last")
        {
            model->exportLast();
            r = true;
        } else
            logError( context(), QScriptContext::SyntaxError, "Filename missing" );
        result.setValue(r);
        return r;
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
            return r;
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
            return r;
        }
        model->exportImage ( filename, false, imgFormat);
    } else if ( format == "Impress" )
    {
        if (argumentCount() < 3 )
        {
            logError( context(), QScriptContext::SyntaxError, "Template file  missing in export to Impress" );
            return r;
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
            return r;
        }   
        QString path = argument(2).toString();
        model->exportXML (path, filename, false);
    } else
    {
        logError( context(), QScriptContext::SyntaxError, QString("Unknown export format: %1").arg(format) );
        return r;
    }
    result.setValue(true);
    return true;
}

QString VymModelWrapper::getDestPath()
{
    QString r = model->getDestPath();
    result.setValue(r);
    return r;
}

QString VymModelWrapper::getFileDir()
{
    QString r = model->getFileDir();
    result.setValue(r);
    return r;
}

QString VymModelWrapper::getFileName()
{
    QString r = model->getFileName();
    result.setValue(r);
    return r;
}

QString VymModelWrapper::getFrameType()
{
    QString r;
    BranchItem *selbi = getSelectedBranch();
    if (selbi)
    {
        BranchObj *bo = (BranchObj*)(selbi->getLMO());
        if (!bo)
            logError( context(), QScriptContext::UnknownError, QString("No BranchObj available") );
        else
            r = bo->getFrame()->getFrameTypeName();
    } 
    result.setValue(r);
    return r;
}

QString VymModelWrapper::getHeadingPlainText()
{
    QString r  = model->getHeading().getTextASCII();
    result = QVariant( r );
    return r;
}

QString VymModelWrapper::getHeadingXML()
{
    QString r  = model->getHeading().saveToDir();
    result = QVariant( r );
    return r;
}

QString VymModelWrapper::getMapAuthor()
{
    QString r = model->getAuthor();
    result.setValue( r );
    return r;
}

QString VymModelWrapper::getMapComment()
{
    QString r = model->getComment();
    result.setValue( r );
    return r;
}

QString VymModelWrapper::getMapTitle()
{
    QString r = model->getTitle();
    result.setValue( r );
    return r;
}

QString VymModelWrapper::getNotePlainText()
{
    QString r  = model->getNote().getTextASCII(); 
    result = QVariant( r );
    return r;
}

QString VymModelWrapper::getNoteXML()
{
    QString r  = model->getNote().saveToDir(); 
    result = QVariant( r );
    return r;
}

QString VymModelWrapper::getSelectString()
{
    QString r = model->getSelectString();
    result.setValue( r );
    return r;
}

int VymModelWrapper::getTaskSleepDays()
{
    int r = -1;
    BranchItem *selbi = getSelectedBranch();
    if (selbi)
    {
        Task *task = selbi->getTask();
        if (task)
            r = task->getDaysSleep();
        else
            logError( context(), QScriptContext::UnknownError, "Branch has no task");
    }
    result.setValue(r);
    return r;
}

QString VymModelWrapper::getURL()
{
    QString r = model->getURL();
    result.setValue(r);
    return r;
}

QString VymModelWrapper::getVymLink()
{
    QString r = model->getVymLink();
    result.setValue(r);
    return r;
}

QString VymModelWrapper::getXLinkColor()
{
    QString r = model->getXLinkColor().name();
    result.setValue(r);
    return r;
}

int VymModelWrapper::getXLinkWidth()
{
    int r = model->getXLinkWidth();
    result.setValue(r);
    return r;
}

QString VymModelWrapper::getXLinkPenStyle()
{
    QString r = penStyleToString( model->getXLinkStyle() );
    result.setValue(r);
    return r;
}

QString VymModelWrapper::getXLinkStyleBegin()
{
    QString r = model->getXLinkStyleBegin();
    result.setValue(r);
    return r;
}

QString VymModelWrapper::getXLinkStyleEnd()
{
    QString r = model->getXLinkStyleEnd();
    result.setValue(r);
    return r;
}

bool VymModelWrapper::hasActiveFlag( const QString &flag)
{
    bool r = false;
    BranchItem *selbi = getSelectedBranch();
    if (selbi) r = selbi->hasActiveStandardFlag( flag );
    result.setValue(r);
    return r;
}

bool VymModelWrapper::hasNote()
{
    bool r = !model->getNote().isEmpty();
    result.setValue( r );
    return r;
}

bool VymModelWrapper::hasRichTextNote()
{
    bool r = model->hasRichTextNote();
    result.setValue( r );
    return r;
}

bool VymModelWrapper::hasTask()
{
    bool r = false;
    BranchItem *selbi = getSelectedBranch();
    if (selbi)
    {
        Task *task = selbi->getTask();
        if (task)
            r = true;
    } else
        logError( context(), QScriptContext::UnknownError, "Selected item is not a branch");

    result.setValue( r );
    return r;
}

void VymModelWrapper::importDir( const QString &path)
{
    model->importDir( path );    // FIXME-1 error handling missing (in vymmodel and here)
}

bool VymModelWrapper::isScrolled()
{
    bool r = false;
    BranchItem *selbi = getSelectedBranch();
    if (selbi) r = selbi->isScrolled();
    result.setValue( r );
    return r;
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

bool VymModelWrapper::parseVymText(const QString &text)
{
    bool r = model->parseVymText( text );
    result.setValue(r);
    return r;
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
    bool r;
    r = model->relinkTo( parent, num, QPointF( x, y ) );
    if (!r) logError( context(), QScriptContext::UnknownError, "Could not relink" );
    result.setValue(r);
    return r;
}

bool VymModelWrapper::relinkTo( const QString &parent, int num)
{
    bool r = relinkTo( parent, num, 0, 0);
    result.setValue(r);
    return r;
}

bool VymModelWrapper::relinkTo( const QString &parent)
{
    bool r = relinkTo( parent, -1, 0, 0);
    result.setValue(r);
    return r;
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
    result.setValue(r);
    return r;
}

bool VymModelWrapper::selectID(const QString &s)
{
    bool r = model->selectID( s );
    if (!r) logError( context(), QScriptContext::UnknownError, QString("Couldn't select ID %1").arg(s));
    result.setValue(r);
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
    result.setValue(r);
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
    result.setValue(r);
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
    result.setValue(r);
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
    result.setValue(r);
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
    result.setValue(r);
    return r;
}

bool VymModelWrapper::selectParent()
{
    bool r = model->selectParent();
    if (!r) 
        logError( context(), QScriptContext::UnknownError, "Couldn't select parent item");
    result.setValue(r);
    return r;
}

bool VymModelWrapper::selectLatestAdded()
{
    bool r =  model->selectLatestAdded();
    if (!r) 
        logError( context(), QScriptContext::UnknownError, "Couldn't select latest added item");
    result.setValue(r);
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
    model->setFrameBorderWidth( width );
}

void VymModelWrapper::setFrameBrushColor( const QString &color)
{
    model->setFrameBrushColor( color );
}

void VymModelWrapper::setFrameIncludeChildren( bool b )
{
    model->setFrameIncludeChildren(b);
}

void VymModelWrapper::setFramePadding( int padding )
{
    model->setFramePadding( padding );
}

void VymModelWrapper::setFramePenColor( const QString &color)
{
    model->setFramePenColor( color );
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
    bool r = model->setTaskSleep( s );
    result.setValue(r);
    return r;
}
    
void VymModelWrapper::setURL(const QString &s)
{
    model->setURL( s );
}

void VymModelWrapper::setVymLink(const QString &s)
{
    model->setVymLink( s );
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

void VymModelWrapper::setXLinkStyle( const QString &style)
{
    model->setXLinkStyle( style );
}

void VymModelWrapper::setXLinkStyleBegin( const QString &style)
{
    model->setXLinkStyleBegin( style );
}

void VymModelWrapper::setXLinkStyleEnd( const QString &style)
{
    model->setXLinkStyleEnd( style );
}

void VymModelWrapper::setXLinkWidth( int w )
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
    bool r = false;
    BranchItem *selbi = getSelectedBranch();
    if (selbi)
    {
        r =  model->unscrollBranch(selbi);
        if (!r) logError( context(), QScriptContext::UnknownError, "Couldn't unscroll branch");
    } 
    result.setValue(r);
    return r;
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

