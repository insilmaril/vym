#include <QApplication>
#include <QSvgGenerator>

#if defined(VYM_DBUS)
#include <QtDBus/QDBusConnection>
#endif

#ifndef Q_OS_WIN
#include <unistd.h>
#else
#define sleep Sleep
#endif

#include "vymmodel.h"

#include "attributeitem.h"
#include "treeitem.h"
#include "branchitem.h"
#include "bugagent.h"
#include "downloadagent.h"
#include "editxlinkdialog.h"
#include "exports.h"
#include "exporthtmldialog.h"
#include "file.h"
#include "findresultmodel.h"
#include "mainwindow.h"
#include "misc.h"
#include "noteeditor.h"
#include "options.h"
#include "parser.h"
#include "process.h"
#include "scripteditor.h" 
#include "slideitem.h"
#include "slidemodel.h"
#include "taskeditor.h"
#include "taskmodel.h"
#include "warningdialog.h"
#include "xlinkitem.h"
#include "xlinkobj.h"
#include "xml-freemind.h"
#include "xmlobj.h"
#include "xml-vym.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

extern bool debug;
extern bool testmode;
extern Main *mainWindow;

#if defined(VYM_DBUS)
extern QDBusConnection dbusConnection;
#endif

extern Settings settings;
extern QString tmpVymDir;

extern NoteEditor *noteEditor;
extern TaskEditor *taskEditor;
extern ScriptEditor *scriptEditor;
extern FlagRow *standardFlagsMaster;

extern Options options;

extern QString clipboardDir;
extern QString clipboardFile;
extern bool clipboardEmpty;

extern ImageIO imageIO;

extern TaskModel* taskModel;

extern QString vymName;
extern QString vymVersion;
extern QDir vymBaseDir;

extern QDir lastImageDir;
extern QDir lastMapDir;
extern QDir lastExportDir;

extern bool bugzillaClientAvailable;

extern Settings settings;

uint VymModel::idLast=0;    // make instance

VymModel::VymModel()
{
    //qDebug()<< "Const VymModel";
    init();
    rootItem->setModel (this);
}


VymModel::~VymModel() 
{
    //qDebug() << "Destr VymModel begin this="<<this<<"  "<<mapName;
    mapEditor=NULL;
    blockReposition=true;
    autosaveTimer->stop();
    fileChangedTimer->stop();
    stopAllAnimation();

    //qApp->processEvents();	// Update view (scene()->update() is not enough)
    //qDebug() << "Destr VymModel end   this="<<this;
}   

void VymModel::clear() 
{
    while (rootItem->childCount() >0)
    {
	//qDebug()<<"VM::clear  ri="<<rootItem<<"  ri->count()="<<rootItem->childCount();
	deleteItem (rootItem->getChildNum(0) );
    }
}

void VymModel::init () 
{
    // No MapEditor yet
    mapEditor=NULL;

    // States and IDs
    idLast++;
    modelID=idLast;
    mapChanged=false;
    mapDefault=true;
    mapUnsaved=false;

    // Selection history
    selModel=NULL;
    selectionBlocked=false;
    resetSelectionHistory();

    resetHistory();

    // Create tmp dirs
    makeTmpDirectories();
    
    // Files
    zipped=true;
    filePath="";
    fileName=tr("unnamed");
    mapName="";
    blockReposition=false;
    blockSaveState=false;

    autosaveTimer=new QTimer (this);
    connect(autosaveTimer, SIGNAL(timeout()), this, SLOT(autosave()));

    fileChangedTimer=new QTimer (this);	
    fileChangedTimer->start(3000);
    connect(fileChangedTimer, SIGNAL(timeout()), this, SLOT(fileChanged()));


    // find routine
    findReset();

    // animations   // FIXME-4 switch to new animation system 
    animationUse=settings.value ("/animation/use",false).toBool();    // FIXME-4 add options to control _what_ is animated
    animationTicks=settings.value("/animation/ticks",20).toInt();
    animationInterval=settings.value("/animation/interval",5).toInt();
    animObjList.clear();    
    animationTimer=new QTimer (this);
    connect(animationTimer, SIGNAL(timeout()), this, SLOT(animate()));

    // View - map
    defaultFont.setPointSizeF (16);
    defLinkColor=QColor (0,0,255);
    linkcolorhint=LinkableMapObj::DefaultColor;
    linkstyle=LinkableMapObj::PolyParabel;
    defXLinkPen.setWidth (1);
    defXLinkPen.setColor ( QColor (50,50,255) );
    defXLinkPen.setStyle ( Qt::DashLine );

    hasContextPos=false;

    hidemode=TreeItem::HideNone;

    // Animation in MapEditor
    zoomFactor=1;
    rotationAngle=0;
    animDuration=2000;
    animCurve=QEasingCurve::OutQuint;

    // Initialize presentation slides
    slideModel=new SlideModel (this);
    blockSlideSelection=false;

    // Avoid recursions later
    cleaningUpLinks=false;

    // Network
    netstate=Offline;

#if defined(VYM_DBUS)
     // Announce myself on DBUS
    new AdaptorModel(this);    // Created and not deleted as documented in Qt
    if (!QDBusConnection::sessionBus().registerObject (QString("/vymmodel_%1").arg(modelID),this))
	qWarning ("VymModel: Couldn't register DBUS object!");
#endif
}

void VymModel::makeTmpDirectories()
{
    // Create unique temporary directories
    tmpMapDir = tmpVymDir+QString("/model-%1").arg(modelID);
    histPath = tmpMapDir+"/history";
    QDir d;
    d.mkdir (tmpMapDir);
}

MapEditor* VymModel::getMapEditor() 
{
    return mapEditor;
}

bool VymModel::isRepositionBlocked()
{
    return blockReposition;
}

void VymModel::updateActions()	
{
    // Tell mainwindow to update states of actions
    mainWindow->updateActions();
}



QString VymModel::saveToDir(const QString &tmpdir, const QString &prefix, bool writeflags, const QPointF &offset, TreeItem *saveSel)
{
    // tmpdir	    temporary directory to which data will be written
    // prefix	    mapname, which will be appended to images etc.
    // 
    // writeflags   Only write flags for "real" save of map, not undo
    // offset	    offset of bbox of whole map in scene. 
    //		    Needed for XML export

    XMLObj xml;

    // Save Header
    QString ls;
    switch (linkstyle)
    {
	case LinkableMapObj::Line: 
	    ls="StyleLine";
	    break;
	case LinkableMapObj::Parabel:
	    ls="StyleParabel";
	    break;
	case LinkableMapObj::PolyLine:	
	    ls="StylePolyLine";
	    break;
	default:
	    ls="StylePolyParabel";
	    break;
    }	

    QString s="<?xml version=\"1.0\" encoding=\"utf-8\"?><!DOCTYPE vymmap>\n";
    QString colhint="";
    if (linkcolorhint==LinkableMapObj::HeadingColor) 
	colhint=xml.attribut("linkColorHint","HeadingColor");

    QString mapAttr=xml.attribut("version",vymVersion);
    if (!saveSel)
	mapAttr+= xml.attribut("author",author) +
		  xml.attribut("title",title) +
		  xml.attribut("comment",comment) +
		  xml.attribut("date",getDate()) +
		  xml.attribut("branchCount", QString().number(branchCount())) +
		  xml.attribut("backgroundColor", mapEditor->getScene()->backgroundBrush().color().name() ) +
		  xml.attribut("defaultFont", defaultFont.toString() ) +
		  xml.attribut("selectionColor", mapEditor->getSelectionColor().name() ) +
		  xml.attribut("linkStyle", ls ) +
		  xml.attribut("linkColor", defLinkColor.name() ) +
		  xml.attribut("defXLinkColor", defXLinkPen.color().name() ) +
		  xml.attribut("defXLinkWidth", QString().setNum(defXLinkPen.width(),10) ) +
		  xml.attribut("defXLinkPenStyle", penStyleToString (defXLinkPen.style() )) +
		  xml.attribut("mapZoomFactor", QString().setNum(mapEditor->getZoomFactorTarget()) ) +
		  xml.attribut("mapRotationAngle", QString().setNum(mapEditor->getAngleTarget()) ) +
		  colhint; 
    s+=xml.beginElement("vymmap",mapAttr); 
    xml.incIndent();

    // Find the used flags while traversing the tree	
    standardFlagsMaster->resetUsedCounter();
    

    // Temporary list of links
    QList <Link*> tmpLinks;

    // Build xml recursivly
    if (!saveSel)
    {
	// Save all mapcenters as complete map, if saveSel not set
	s+=saveTreeToDir(tmpdir,prefix,offset,tmpLinks);

	// Save local settings
	s+=settings.getDataXML (destPath);

	// Save selection
	if (getSelectedItem() && !saveSel ) 
	    s+=xml.valueElement("select",getSelectString());

    } else
    {
	switch (saveSel->getType())
	{
	    case TreeItem::Branch:
		// Save Subtree
		s+=((BranchItem*)saveSel)->saveToDir(tmpdir,prefix,offset,tmpLinks);
		break;
	    case TreeItem::MapCenter:
		// Save Subtree
		s+=((BranchItem*)saveSel)->saveToDir(tmpdir,prefix,offset,tmpLinks);
		break;
	    case TreeItem::Image:
		// Save Image
		s+=((ImageItem*)saveSel)->saveToDir(tmpdir,prefix);
		break;
	    default: 
		// other types shouldn't be safed directly...
		break;
	}
    }

    // Save XLinks
    for (int i=0; i<tmpLinks.count();++i)
	s+=tmpLinks.at(i)->saveToDir();

    // Save slides  
    s+=slideModel->saveToDir();	

    xml.decIndent();
    s+=xml.endElement("vymmap");

    if (writeflags) standardFlagsMaster->saveToDir (tmpdir+"/flags/","",writeflags);
    return s;
}

QString VymModel::saveTreeToDir (const QString &tmpdir,const QString &prefix, const QPointF &offset, QList <Link*> &tmpLinks)
{
    QString s;
    for (int i=0; i<rootItem->branchCount(); i++)
	s+=rootItem->getBranchNum(i)->saveToDir (tmpdir,prefix,offset,tmpLinks);
    return s;
}

void VymModel::setFilePath(QString fpath, QString destname)
{
    if (fpath.isEmpty() || fpath=="")
    {
	filePath="";
	fileName="";
	destPath="";
    } else
    {
	filePath=fpath;	    // becomes absolute path
	fileName=fpath;	    // gets stripped of path
	destPath=destname;  // needed for vymlinks and during load to reset fileChangedTime

	// If fpath is not an absolute path, complete it
	filePath=QDir(fpath).absolutePath();
	fileDir=filePath.left (1+filePath.lastIndexOf ("/"));

	// Set short name, too. Search from behind:
        fileName=basename(fileName);

	// Forget the .vym (or .xml) for name of map
	mapName=fileName.left(fileName.lastIndexOf(".",-1,Qt::CaseSensitive) );
    }
}

void VymModel::setFilePath(QString fpath)
{
    setFilePath (fpath,fpath);
}

QString VymModel::getFileDir()
{
    return fileDir;
}

QString VymModel::getFilePath()
{
    return filePath;
}

QString VymModel::getFileName()
{
    return fileName;
}

QString VymModel::getMapName()
{
    return mapName;
}

QString VymModel::getDestPath()
{
    return destPath;
}

File::ErrorCode VymModel::loadMap (	
    QString fname, 
    const LoadMode &lmode, 
    const FileType &ftype,
    const int &contentFilter,
    int pos)
{
    File::ErrorCode err=File::Success;

    // Get updated zoomFactor, before applying one read from file in the end
    if (mapEditor) 
    {
	zoomFactor=mapEditor->getZoomFactorTarget();	
	rotationAngle=mapEditor->getAngleTarget();
    }

    parseBaseHandler *handler;
    fileType=ftype;
    switch (fileType)
    {
	case VymMap: 
	    handler=new parseVYMHandler; 
	    ((parseVYMHandler*)handler)->setContentFilter (contentFilter);
	    break;
	case FreemindMap : handler=new parseFreemindHandler; break;
	default: 
	    QMessageBox::critical( 0, tr( "Critical Parse Error" ),
		   "Unknown FileType in VymModel::load()");
	return File::Aborted;	
    }

    bool zipped_org=zipped;

    if (lmode==NewMap)
    {
	// Reset timestamp to check for later updates of file
	fileChangedTime=QFileInfo (destPath).lastModified();

	selModel->clearSelection();
    } 

    // Create temporary directory for packing
    bool ok;
    QString tmpZipDir=makeTmpDir (ok,"vym-pack");
    if (!ok)
    {
	QMessageBox::critical( 0, tr( "Critical Load Error" ),
	   tr("Couldn't create temporary directory before load\n"));
	return File::Aborted; 
    }

    // Try to unzip file
    err=unzipDir (tmpZipDir,fname);
    QString xmlfile;
    if (err==File::NoZip)
    {
	xmlfile=fname;
	zipped=false;
    } else
    {
	zipped=true;
	
	// Look for mapname.xml
	xmlfile= fname.left(fname.lastIndexOf(".",-1,Qt::CaseSensitive));
	xmlfile=xmlfile.section( '/', -1 );
	QFile mfile( tmpZipDir + "/" + xmlfile + ".xml");
	if (!mfile.exists() )
	{
	    // mapname.xml does not exist, well, 
	    // maybe someone renamed the mapname.vym file...
	    // Try to find any .xml in the toplevel 
	    // directory of the .vym file
	    QStringList filters;
	    filters<<"*.xml";
	    QStringList flist=QDir (tmpZipDir).entryList(filters);
	    if (flist.count()==1) 
	    {
		// Only one entry, take this one
                xmlfile=tmpZipDir + "/"+flist.first();
            } else
	    {
		for ( QStringList::Iterator it = flist.begin(); it != flist.end(); ++it ) 
		    *it=tmpZipDir + "/" + *it;
		// FIXME-4 Multiple entries, load all (but only the first one into this ME)
		//mainWindow->fileLoadFromTmp (flist);
		//returnCode=1;	// Silently forget this attempt to load
		qWarning ("MainWindow::load (fn)  multimap found...");
	    }	
		
	    if (flist.isEmpty() )
	    {
		QMessageBox::critical( 0, tr( "Critical Load Error" ),
			   tr("Couldn't find a map (*.xml) in .vym archive.\n"));
		err=File::Aborted;		       
	    }	
	} //file doesn't exist	
	else
	    xmlfile=mfile.fileName();
    }

    QFile file( xmlfile);

    // I am paranoid: file should exist anyway
    // according to check in mainwindow.
    if (!file.exists() )
    {
	QMessageBox::critical( 0, tr( "Critical Parse Error" ),
		   tr(QString("Couldn't open map %1").arg(file.fileName()).toUtf8()));
	err=File::Aborted;	
    } else
    {
	bool blockSaveStateOrg=blockSaveState;
	blockReposition=true;
	blockSaveState=true;
	mapEditor->setViewportUpdateMode (QGraphicsView::NoViewportUpdate);
	QXmlInputSource source( &file);
	QXmlSimpleReader reader;
	reader.setContentHandler( handler );
	reader.setErrorHandler( handler );
	handler->setModel ( this);

	// We need to set the tmpDir in order  to load files with rel. path
	QString tmpdir;
	if (zipped)
	    tmpdir=tmpZipDir;
	else
	    tmpdir=fname.left(fname.lastIndexOf("/",-1));	
	handler->setTmpDir (tmpdir);
	handler->setInputFile (file.fileName());
	if (lmode==ImportReplace)
	    handler->setLoadMode (ImportReplace,pos);
	else	
	    handler->setLoadMode (lmode,pos);

	bool ok = reader.parse( source );
	blockReposition=false;
	blockSaveState=blockSaveStateOrg;
	mapEditor->setViewportUpdateMode (QGraphicsView::MinimalViewportUpdate);
	file.close();
	if ( ok ) 
	{
	    if (lmode==NewMap)
	    {
		mapDefault=false;
		mapChanged=false;
		mapUnsaved=false;
		autosaveTimer->stop();

		resetHistory();
		resetSelectionHistory();
	    }
    
	    reposition();   // to generate bbox sizes
	    emitSelectionChanged();

	    // Recalc priorities and sort   
	    taskModel->recalcPriorities();
	} else 
	{
	    QMessageBox::critical( 0, tr( "Critical Parse Error" ),
		       tr( handler->errorProtocol().toUtf8() ) );
	    // returnCode=1;	
	    // Still return "success": the map maybe at least
	    // partially read by the parser
	}   
    }	

    // Delete tmpZipDir
    removeDir (QDir(tmpZipDir));

    // Restore original zip state
    zipped=zipped_org;

    updateActions();
    
    if (lmode!=NewMap) emitUpdateQueries();

    if (mapEditor) 
    {
	mapEditor->setZoomFactorTarget (zoomFactor);
	mapEditor->setAngleTarget (rotationAngle);
    }

    if (vymView) vymView->readSettings();

    qApp->processEvents();  // Update view (scene()->update() is not enough)
    return err;
}

File::ErrorCode VymModel::save (const SaveMode &savemode)
{
    QString tmpZipDir;
    QString mapFileName;
    QString safeFilePath;

    File::ErrorCode err=File::Success;

    if (zipped)
	// save as .xml
	mapFileName=mapName+".xml";
    else
	// use name given by user, even if he chooses .doc
	mapFileName=fileName;

    // Look, if we should zip the data:
    if (!zipped)
    {
	QMessageBox mb( vymName,
	    tr("The map %1\ndid not use the compressed "
	    "vym file format.\nWriting it uncompressed will also write images \n"
	    "and flags and thus may overwrite files in the "
	    "given directory\n\nDo you want to write the map").arg(filePath),
	    QMessageBox::Warning,
	    QMessageBox::Yes | QMessageBox::Default,
	    QMessageBox::No ,
	    QMessageBox::Cancel | QMessageBox::Escape);
	mb.setButtonText( QMessageBox::Yes, tr("compressed (vym default)") );
	mb.setButtonText( QMessageBox::No, tr("uncompressed") );
	mb.setButtonText( QMessageBox::Cancel, tr("Cancel"));
	switch( mb.exec() ) 
	{
	    case QMessageBox::Yes:
		// save compressed (default file format)
		zipped=true;
		break;
	    case QMessageBox::No:
		// save uncompressed
		zipped=false;
		break;
	    case QMessageBox::Cancel:
		// do nothing
		return File::Aborted;
		break;
	}
    }

    // First backup existing file, we 
    // don't want to add to old zip archives
    QFile f(destPath);
    if (f.exists())
    {
	if ( settings.value ("/mapeditor/writeBackupFile").toBool())
	{
	    QString backupFileName(destPath + "~");
	    QFile backupFile(backupFileName);
	    if (backupFile.exists() && !backupFile.remove())
	    {
		QMessageBox::warning(0, tr("Save Error"),
				     tr("%1\ncould not be removed before saving").arg(backupFileName));
	    }
	    else if (!f.rename(backupFileName))
	    {
		QMessageBox::warning(0, tr("Save Error"),
				     tr("%1\ncould not be renamed before saving").arg(destPath));
	    }
	}
    }

    if (zipped)
    {
	// Create temporary directory for packing
	bool ok;
	tmpZipDir=makeTmpDir (ok,"vym-zip");
	if (!ok)
	{
	    QMessageBox::critical( 0, tr( "Critical Load Error" ),
	       tr("Couldn't create temporary directory before save\n"));
	    return File::Aborted; 
	}

	safeFilePath=filePath;
	setFilePath (tmpZipDir+"/"+ mapName+ ".xml", safeFilePath);
    } // zipped

    // Create mapName and fileDir
    makeSubDirs (fileDir);

    QString saveFile;
    if (savemode==CompleteMap || selModel->selection().isEmpty())
    {
	// Save complete map
	saveFile=saveToDir (fileDir,mapName+"-",true,QPointF(),NULL);
	mapChanged=false;
	mapUnsaved=false;
	autosaveTimer->stop();
    }
    else    
    {
	// Save part of map
	if (selectionType()==TreeItem::Image)
	    saveImage();
	else	
	    saveFile=saveToDir (fileDir,mapName+"-",true,QPointF(),getSelectedBranch());    
	// TODO take care of multiselections
    }	

    if (!saveStringToDisk(fileDir+mapFileName,saveFile))
    {
	err=File::Aborted;
	qWarning ("ME::saveStringToDisk failed!");
    }

    if (zipped)
    {
	// zip
	if (err==File::Success) err=zipDir (tmpZipDir,destPath);

	// Delete tmpDir
	removeDir (QDir(tmpZipDir));

	// Restore original filepath outside of tmp zip dir
	setFilePath (safeFilePath);
    }

    updateActions();
    fileChangedTime=QFileInfo (destPath).lastModified();
    return err;
}

void VymModel::loadImage (BranchItem *dst,const QString &fn)
{
    if (!dst) dst=getSelectedBranch();
    if (dst)
    {
	QString filter=QString (tr("Images") + " (*.png *.bmp *.xbm *.jpg *.png *.xpm *.gif *.pnm *.svg);;"+tr("All","Filedialog") +" (*.*)");
	QStringList fns;
	if (fn.isEmpty() )
	    fns=QFileDialog::getOpenFileNames( 
		NULL,
		vymName+" - " + tr("Load image"), 
		lastImageDir.path(), 
		filter);
	else
	    fns.append (fn);

	if (!fns.isEmpty() )
	{
	    lastImageDir.setPath(fns.first().left(fns.first().lastIndexOf ("/")) );
	    QString s;
	    for (int j=0; j<fns.count(); j++)
	    {
		s=fns.at(j);
		ImageItem *ii=createImage(dst);
		if (ii && ii->load (s) )
		{
		    saveState(
			(TreeItem*)ii,
			"delete ()",
			dst, 
			QString ("loadImage (\"%1\")").arg(s ),
			QString("Add image %1 to %2").arg(s).arg(getObjectName(dst))
		    );
		    // Find nice position
		    FloatImageObj *fio=(FloatImageObj*)(ii->getMO() );
		    if (fio)
			fio->move2RelPos (0,0);

		    // On default include image // FIXME-4 check, if we change default settings...
		    setIncludeImagesHor (true);
		    setIncludeImagesVer (true);

		    reposition();
		} else
		    // FIXME-4 loadFIO error handling
		    qWarning ()<<"Failed to load "+s;
	    }

	}
    }
}

void VymModel::saveImage (ImageItem *ii, QString format, QString fn)                                                    
{
    if (!ii) ii=getSelectedImage();
    if (ii)
    {
	QString filter=QString (tr("Images") + " (*.png *.bmp *.xbm *.jpg *.png *.xpm *.gif *.pnm *.svg);;"+tr("All","Filedialog") +" (*.*)");
	if (fn.isEmpty() )
	    fn=QFileDialog::getSaveFileName( 
		NULL,
		vymName+" - " + tr("Save image"), 
		lastImageDir.path(), 
		filter,
		NULL,
		QFileDialog::DontConfirmOverwrite);

	if (!fn.isEmpty() )
	{
	    lastImageDir.setPath(fn.left(fn.lastIndexOf ("/")) );
	    if (QFile (fn).exists() )
	    {
		QMessageBox mb( vymName,
		    tr("The file %1 exists already.\n"
		    "Do you want to overwrite it?").arg(fn),
		QMessageBox::Warning,
		QMessageBox::Yes | QMessageBox::Default,
		QMessageBox::Cancel | QMessageBox::Escape,
		QMessageBox::NoButton );

		mb.setButtonText( QMessageBox::Yes, tr("Overwrite") );
		mb.setButtonText( QMessageBox::No, tr("Cancel"));
		switch( mb.exec() ) 
		{
		    case QMessageBox::Yes:
			// save 
			break;
		    case QMessageBox::Cancel:
			// do nothing
			return;
			break;
		}
	    }
	    if (format.isEmpty() ) format=imageIO.guessType(fn);
	    if (format.isEmpty())
		QMessageBox::critical (0,tr("Critical Error"),tr("Unsupported format in %1").arg(fn));
	    else if (!ii->save (fn, format) )
		QMessageBox::critical (0,tr("Critical Error"),tr("Couldn't save %1").arg(fn));
	} 
    }
}


void VymModel::importDirInt(BranchItem *dst, QDir d) 
{
    bool oldSaveState=blockSaveState;
    blockSaveState=true;
    BranchItem *selbi=getSelectedBranch();
    BranchItem *bi;
    if (selbi)
    {
	int beginDepth=selbi->depth();

	d.setFilter(QDir::AllEntries | QDir::Hidden);
	QFileInfoList list = d.entryInfoList();
	QFileInfo fi;

	// Traverse directories
	for (int i = 0; i < list.size(); ++i) 
	{
	    fi=list.at(i);
	    if (fi.isDir() && fi.fileName() != "." && fi.fileName() != ".." )
	    {
		bi=addNewBranchInt(dst,-2);
		bi->setHeading (fi.fileName() );   
		bi->setHeadingColor (QColor("blue"));
		if ( !d.cd(fi.fileName()) ) 
		    QMessageBox::critical (0,tr("Critical Import Error"),tr("Cannot find the directory %1").arg(fi.fileName()));
		else 
		{
		    // Recursively add subdirs
		    importDirInt (bi,d);
		    d.cdUp();
		}
		emitDataChanged(bi);
	    }	
	}	

	for (int i = 0; i < list.size(); ++i) 
	{
	    fi=list.at(i);
	    if (fi.isFile())
	    {
		bi=addNewBranchInt (dst,-2);
		bi->setHeading (fi.fileName() );
		bi->setHeadingColor (QColor("black"));
		if (fi.fileName().right(4) == ".vym" )
		    bi->setVymLink (fi.filePath());
		emitDataChanged(bi);
	    }
	}   

	// Scroll at least some stuff
	if (dst->branchCount()>1 && dst->depth()-beginDepth>2)
	    dst->toggleScroll();
    }	    
    blockSaveState=oldSaveState;
}

void VymModel::importDirInt (const QString &s)	
{
    BranchItem *selbi=getSelectedBranch();
    if (selbi)
    {
	saveStateChangingPart (selbi,selbi,QString ("importDir (\"%1\")").arg(s),QString("Import directory structure from %1").arg(s));

	QDir d(s);
	importDirInt (selbi,d);
    }
}   

void VymModel::importDir()  
{
    BranchItem *selbi=getSelectedBranch();
    if (selbi)
    {
	QStringList filters;
	filters <<"VYM map (*.vym)";
	QFileDialog fd;
	fd.setWindowTitle (vymName+ " - " +tr("Choose directory structure to import"));
	fd.setFileMode (QFileDialog::DirectoryOnly);
	fd.setFilters (filters);
	fd.setWindowTitle(vymName+" - " +tr("Choose directory structure to import"));
	fd.setAcceptMode (QFileDialog::AcceptOpen);

	QString fn;
	if ( fd.exec() == QDialog::Accepted &&!fd.selectedFiles().isEmpty() )
	{
	    importDirInt (fd.selectedFiles().first() );
	    reposition();
	}
    }	
}


void VymModel::autosave()
{
    if (filePath=="") 
    {
	if (debug)
	    qDebug() << "VymModel::autosave rejected due to missing filePath\n";
    }

    QDateTime now=QDateTime().currentDateTime();

    // Disable autosave, while we have gone back in history
    int redosAvail=undoSet.readNumValue (QString("/history/redosAvail"));
    if (redosAvail>0) return;

    // Also disable autosave for new map without filename
    if (filePath.isEmpty()) return;


    if (mapUnsaved 
	&& mapChanged 
	&& mainWindow->useAutosave() 
	&& !testmode)
    {
	if (QFileInfo(filePath).lastModified()<=fileChangedTime) 
	    mainWindow->fileSave (this);
	else
	    if (debug)
		qDebug() <<"  ME::autosave  rejected, file on disk is newer than last save.\n"; 

    }	
}

void VymModel::fileChanged()
{
    // Check if file on disk has changed meanwhile
    if (!filePath.isEmpty())
    {
	QDateTime tmod=QFileInfo (filePath).lastModified();
	if (tmod>fileChangedTime)
	{
	    // FIXME-4 VM switch to current mapeditor and finish lineedits...
	    QMessageBox mb( vymName,
		tr("The file of the map  on disk has changed:\n\n"  
		   "   %1\n\nDo you want to reload that map with the new file?").arg(filePath),
		QMessageBox::Question,
		QMessageBox::Yes ,
		QMessageBox::Cancel | QMessageBox::Default,
		QMessageBox::NoButton );

	    mb.setButtonText( QMessageBox::Yes, tr("Reload"));
	    mb.setButtonText( QMessageBox::No, tr("Ignore"));
	    switch( mb.exec() ) 
	    {
		case QMessageBox::Yes:
		    // Reload map
		    mainWindow->initProgressCounter (1);
		    loadMap (filePath);
		    mainWindow->removeProgressCounter ();
		    break;
		case QMessageBox::Cancel:
		    fileChangedTime=tmod; // allow autosave to overwrite newer file!
	    }
	}
    }	
}

bool VymModel::isDefault()
{
    return mapDefault;
}

void VymModel::makeDefault()
{
    mapChanged=false;
    mapDefault=true;
}

bool VymModel::hasChanged()
{
    return mapChanged;
}

void VymModel::setChanged()
{
    if (!mapChanged)
	autosaveTimer->start(settings.value("/mainwindow/autosave/ms/",300000).toInt());
    mapChanged=true;
    mapDefault=false;
    mapUnsaved=true;
    findReset();
    mainWindow->updateActions();
}

QString VymModel::getObjectName (LinkableMapObj *lmo)
{
    if (!lmo || !lmo->getTreeItem() ) return QString();
    return getObjectName (lmo->getTreeItem() );
}


QString VymModel::getObjectName (TreeItem *ti)
{
    QString s;
    if (!ti) return QString("Error: NULL has no name!");
    s=ti->getHeadingPlain();
    if (s=="") s="unnamed";

    return QString ("%1 (%2)").arg(ti->getTypeName()).arg(s);
}

void VymModel::redo()
{
    // Can we undo at all?
    if (redosAvail<1) return;

    bool blockSaveStateOrg=blockSaveState;
    blockSaveState=true;
    
    redosAvail--;

    if (undosAvail<stepsTotal) undosAvail++;
    curStep++;
    if (curStep>stepsTotal) curStep=1;
    QString undoCommand=  undoSet.value (QString("/history/step-%1/undoCommand").arg(curStep));
    QString undoSelection=undoSet.value (QString("/history/step-%1/undoSelection").arg(curStep));
    QString redoCommand=  undoSet.value (QString("/history/step-%1/redoCommand").arg(curStep));
    QString redoSelection=undoSet.value (QString("/history/step-%1/redoSelection").arg(curStep));
    QString comment=undoSet.value (QString("/history/step-%1/comment").arg(curStep));
    QString version=undoSet.value ("/history/version");

    /* TODO Maybe check for version, if we save the history
    if (!checkVersion(version))
	QMessageBox::warning(0,tr("Warning"),
	    tr("Version %1 of saved undo/redo data\ndoes not match current vym version %2.").arg(version).arg(vymVersion));
    */ 

    // Find out current undo directory
    QString bakMapDir(QString(tmpMapDir+"/undo-%1").arg(curStep));

    if (debug)
    {
	qDebug() << "VymModel::redo() begin\n";
	qDebug() << "    undosAvail="<<undosAvail;
	qDebug() << "    redosAvail="<<redosAvail;
	qDebug() << "       curStep="<<curStep;
	qDebug() << "    ---------------------------";
	qDebug() << "    comment="<<comment;
	qDebug() << "    undoCom="<<undoCommand;
	qDebug() << "    undoSel="<<undoSelection;
	qDebug() << "    redoCom="<<redoCommand;
	qDebug() << "    redoSel="<<redoSelection;
	qDebug() << "    ---------------------------";
    }

    // select  object before redo
    if (!redoSelection.isEmpty())
	select (redoSelection);

    bool noErr;
    QString errMsg;
    parseAtom (redoCommand,noErr,errMsg);
    if (!noErr) 
    {
	if (!options.isOn("batch") )
	    QMessageBox::warning(0,tr("Warning"),tr("Redo failed:\n%1").arg(errMsg));
	qWarning()<< "VM::redo aborted:\n"<<errMsg;
    }

    blockSaveState=blockSaveStateOrg;

    undoSet.setValue ("/history/undosAvail",QString::number(undosAvail));
    undoSet.setValue ("/history/redosAvail",QString::number(redosAvail));
    undoSet.setValue ("/history/curStep",QString::number(curStep));
    undoSet.writeSettings(histPath);

    mainWindow->updateHistory (undoSet);
    updateActions();

    /* TODO remove testing
    qDebug() << "ME::redo() end\n";
    qDebug() << "    undosAvail="<<undosAvail;
    qDebug() << "    redosAvail="<<redosAvail;
    qDebug() << "       curStep="<<curStep;
    qDebug() << "    ---------------------------";
    */
}

bool VymModel::isRedoAvailable()
{
    if (undoSet.readNumValue("/history/redosAvail",0)>0)
	return true;
    return false;
}

void VymModel::undo()	
{
    // Can we undo at all?
    if (undosAvail<1) return;

    mainWindow->statusMessage (tr("Autosave disabled during undo."));

    bool blockSaveStateOrg=blockSaveState;
    blockSaveState=true;
    
    QString undoCommand=  undoSet.value (QString("/history/step-%1/undoCommand").arg(curStep));
    QString undoSelection=undoSet.value (QString("/history/step-%1/undoSelection").arg(curStep));
    QString redoCommand=  undoSet.value (QString("/history/step-%1/redoCommand").arg(curStep));
    QString redoSelection=undoSet.value (QString("/history/step-%1/redoSelection").arg(curStep));
    QString comment=undoSet.value (QString("/history/step-%1/comment").arg(curStep));
    QString version=undoSet.value ("/history/version");

    /* TODO Maybe check for version, if we save the history
    if (!checkVersion(version))
	QMessageBox::warning(0,tr("Warning"),
	    tr("Version %1 of saved undo/redo data\ndoes not match current vym version %2.").arg(version).arg(vymVersion));
    */

    // Find out current undo directory
    QString bakMapDir(QString(tmpMapDir+"/undo-%1").arg(curStep));

    // select  object before undo
    if (!undoSelection.isEmpty() && !select (undoSelection))
    {
	qWarning ("VymModel::undo()  Could not select object for undo");
	return;
    }

    if (debug)
    {
	qDebug() << "VymModel::undo() begin\n";
	qDebug() << "    undosAvail="<<undosAvail;
	qDebug() << "    redosAvail="<<redosAvail;
	qDebug() << "       curStep="<<curStep;
	qDebug() << "    ---------------------------";
	qDebug() << "    comment="<<comment;
	qDebug() << "    undoCom="<<undoCommand;
	qDebug() << "    undoSel="<<undoSelection;
	qDebug() << "    redoCom="<<redoCommand;
	qDebug() << "    redoSel="<<redoSelection;
	qDebug() << "    ---------------------------";
    }	

    bool noErr;
    QString errMsg;
    parseAtom (undoCommand,noErr,errMsg);
    if (!noErr)
    {
	if (!options.isOn("batch") )
	    QMessageBox::warning(0,tr("Warning"),tr("Undo failed:\n%1").arg(errMsg));
	qWarning()<< "VM::undo failed:\n"<<errMsg;
    }


    undosAvail--;
    curStep--; 
    if (curStep<1) curStep=stepsTotal;

    redosAvail++;

    blockSaveState=blockSaveStateOrg;
/* testing only
    qDebug() << "VymModel::undo() end\n";
    qDebug() << "    undosAvail="<<undosAvail;
    qDebug() << "    redosAvail="<<redosAvail;
    qDebug() << "       curStep="<<curStep;
    qDebug() << "    ---------------------------";
*/

    undoSet.setValue ("/history/undosAvail",QString::number(undosAvail));
    undoSet.setValue ("/history/redosAvail",QString::number(redosAvail));
    undoSet.setValue ("/history/curStep",QString::number(curStep));
    undoSet.writeSettings(histPath);

    mainWindow->updateHistory (undoSet);
    updateActions();
    //emitSelectionChanged();
}

bool VymModel::isUndoAvailable()
{
    if (undoSet.readNumValue("/history/undosAvail",0)>0)
	return true;
    return false;
}

void VymModel::gotoHistoryStep (int i)
{
    // Restore variables
    int undosAvail=undoSet.readNumValue (QString("/history/undosAvail"));
    int redosAvail=undoSet.readNumValue (QString("/history/redosAvail"));

    if (i<0) i=undosAvail+redosAvail;

    // Clicking above current step makes us undo things
    if (i<undosAvail) 
    {	
	for (int j=0; j<undosAvail-i; j++) undo();
	return;
    }	
    // Clicking below current step makes us redo things
    if (i>undosAvail) 
	for (int j=undosAvail; j<i; j++) 
	{
	    if (debug) qDebug() << "VymModel::gotoHistoryStep redo "<<j<<"/"<<undosAvail<<" i="<<i;
	    redo();
	}

    // And ignore clicking the current row ;-)	
}


QString VymModel::getHistoryPath()
{
    QString histName(QString("history-%1").arg(curStep));
    return (tmpMapDir+"/"+histName);
}

void VymModel::resetHistory()
{
    curStep=0;
    redosAvail=0;
    undosAvail=0;

    stepsTotal=settings.value("/history/stepsTotal",100).toInt();
    undoSet.setValue ("/history/stepsTotal",QString::number(stepsTotal));
    mainWindow->updateHistory (undoSet);
}

void VymModel::saveState(
    const SaveMode &savemode, 
    const QString &undoSelection, 
    const QString &undoCom, 
    const QString &redoSelection, 
    const QString &redoCom, 
    const QString &comment, 
    TreeItem *saveSel, 
    QString dataXML)
{
    sendData(redoCom);	//FIXME-4 testing

    // Main saveState

    if (blockSaveState) return;

    if (debug) qDebug() << "VM::saveState() for  "<<mapName;
    
    // Find out current undo directory
    if (undosAvail<stepsTotal) undosAvail++;
    curStep++;
    if (curStep>stepsTotal) curStep=1;
    
    QString histDir=getHistoryPath();
    QString bakMapPath=histDir+"/map.xml";

    // Create histDir if not available
    QDir d(histDir);
    if (!d.exists()) 
	makeSubDirs (histDir);

    // Save depending on how much needs to be saved 
    QList <Link*> tmpLinks;
    if (saveSel)
	dataXML=saveToDir (histDir,mapName+"-",false, QPointF (),saveSel);
	
    QString undoCommand=undoCom;
    QString redoCommand=redoCom;
    if (savemode==PartOfMap )
    {
	undoCommand.replace ("PATH",bakMapPath);
	redoCommand.replace ("PATH",bakMapPath);
    }

    if (!dataXML.isEmpty())
	// Write XML Data to disk
	saveStringToDisk (bakMapPath,dataXML);

    // We would have to save all actions in a tree, to keep track of 
    // possible redos after a action. Possible, but we are too lazy: forget about redos.
    redosAvail=0;

    // Write the current state to disk
    undoSet.setValue ("/history/undosAvail",QString::number(undosAvail));
    undoSet.setValue ("/history/redosAvail",QString::number(redosAvail));
    undoSet.setValue ("/history/curStep",QString::number(curStep));
    undoSet.setValue (QString("/history/step-%1/undoCommand").arg(curStep),undoCommand);
    undoSet.setValue (QString("/history/step-%1/undoSelection").arg(curStep),undoSelection);
    undoSet.setValue (QString("/history/step-%1/redoCommand").arg(curStep),redoCommand);
    undoSet.setValue (QString("/history/step-%1/redoSelection").arg(curStep),redoSelection);
    undoSet.setValue (QString("/history/step-%1/comment").arg(curStep),comment);
    undoSet.setValue (QString("/history/version"),vymVersion);
    undoSet.writeSettings(histPath);

    if (debug)
    {
	//qDebug() << "          into="<< histPath;
	qDebug() << "    stepsTotal="<<stepsTotal<<
	", undosAvail="<<undosAvail<<
	", redosAvail="<<redosAvail<<
	", curStep="<<curStep;
	qDebug() << "    ---------------------------";
	qDebug() << "    comment="<<comment;
	qDebug() << "    undoCom="<<undoCommand;
	qDebug() << "    undoSel="<<undoSelection;
	qDebug() << "    redoCom="<<redoCommand;
	qDebug() << "    redoSel="<<redoSelection;
	if (saveSel) qDebug() << "    saveSel="<<qPrintable (getSelectString(saveSel));
	qDebug() << "    ---------------------------";
    }

    mainWindow->updateHistory (undoSet);
    setChanged();
    updateActions();
}


void VymModel::saveStateChangingPart(TreeItem *undoSel, TreeItem* redoSel, const QString &rc, const QString &comment)
{
    // save the selected part of the map, Undo will replace part of map 
    QString undoSelection="";
    if (undoSel)
	undoSelection=getSelectString(undoSel);
    else
	qWarning ("VymModel::saveStateChangingPart  no undoSel given!");
    QString redoSelection="";
    if (redoSel)
	redoSelection=getSelectString(undoSel);
    else
	qWarning ("VymModel::saveStateChangingPart  no redoSel given!");
	

    saveState (PartOfMap,
	undoSelection, "addMapReplace (\"PATH\")",
	redoSelection, rc, 
	comment, 
	undoSel);
}

void VymModel::saveStateRemovingPart(TreeItem* redoSel, const QString &comment)
{
    if (!redoSel)
    {
	qWarning ("VymModel::saveStateRemovingPart  no redoSel given!");
	return;
    }
    QString undoSelection;
    QString redoSelection=getSelectString(redoSel);
    if (redoSel->isBranchLikeType() )
    {
	// save the selected branch of the map, Undo will insert part of map 
	if (redoSel->depth()>0)
	    undoSelection=getSelectString (redoSel->parent());
	saveState (PartOfMap,
	    undoSelection, QString("addMapInsert (\"PATH\",%1,%2)").arg(redoSel->num()).arg(SlideContent),
	    redoSelection, "delete ()", 
	    comment, 
	    redoSel);
    }
}

void VymModel::saveState(TreeItem *undoSel, const QString &uc, TreeItem *redoSel, const QString &rc, const QString &comment) 
{
    // "Normal" savestate: save commands, selections and comment
    // so just save commands for undo and redo
    // and use current selection, if empty parameter passed

    QString redoSelection="";
    if (redoSel) redoSelection=getSelectString(redoSel);
    QString undoSelection="";
    if (undoSel) undoSelection=getSelectString(undoSel);

    saveState (UndoCommand,
	undoSelection, uc,
	redoSelection, rc, 
	comment, 
	NULL);
}

void VymModel::saveState(const QString &undoSel, const QString &uc, const QString &redoSel, const QString &rc, const QString &comment) 
{
    // "Normal" savestate: save commands, selections and comment
    // so just save commands for undo and redo
    // and use current selection
    saveState (UndoCommand,
	undoSel, uc,
	redoSel, rc, 
	comment, 
	NULL);
}

void VymModel::saveState(const QString &uc, const QString &rc, const QString &comment) 
{
    // "Normal" savestate applied to model (no selection needed): 
    // save commands  and comment
    saveState (UndoCommand,
	NULL, uc,
	NULL, rc, 
	comment, 
	NULL);
}

void VymModel::saveStateMinimal(TreeItem *undoSel, const QString &uc, TreeItem *redoSel, const QString &rc, const QString &comment) 
{   //  Save a change in string and merge
    //  minor sequential  changes  */
    QString redoSelection="";
    if (redoSel) redoSelection=getSelectString(redoSel);
    QString undoSelection="";
    if (undoSel) undoSelection=getSelectString(undoSel);

    saveState (UndoCommand,
	undoSelection, uc,
	redoSelection, rc, 
	comment, 
	NULL);
}

void VymModel::saveStateBeforeLoad (LoadMode lmode, const QString &fname)
{ 
    BranchItem *selbi=getSelectedBranch();
    if (selbi) 
    {
	if (lmode==ImportAdd)
	    saveStateChangingPart(
		selbi,
		selbi,
		QString("addMapInsert (\"%1\")").arg(fname),
		QString("Add map %1 to %2").arg(fname).arg(getObjectName(selbi)));
	if (lmode==ImportReplace)
	{
	    BranchItem *pi=(BranchItem*)(selbi->parent());
	    saveStateChangingPart(
		pi,
		pi,
		QString("addMapReplace(%1)").arg(fname),
		QString("Add map %1 to %2").arg(fname).arg(getObjectName(selbi)));
	}
    }
}



QGraphicsScene* VymModel::getScene ()
{
    return mapEditor->getScene();
}

TreeItem* VymModel::findBySelectString(QString s)
{
    if (s.isEmpty() ) return NULL;

    // Old maps don't have multiple mapcenters and don't save full path
    if (s.left(2) !="mc") s="mc:0,"+s;

    QStringList parts=s.split (",");
    QString typ;
    int n;
    TreeItem *ti=rootItem;

    while (!parts.isEmpty() )
    {
	typ=parts.first().left(2);
	n=parts.first().right(parts.first().length() - 3).toInt();
	parts.removeFirst();
	if (typ=="mc" || typ=="bo")
	    ti=ti->getBranchNum (n);
	else if (typ=="fi")
	    ti=ti->getImageNum (n);
	else if (typ=="ai")
	    ti=ti->getAttributeNum (n);
	else if (typ=="xl")
	    ti=ti->getXLinkItemNum (n);
	if(!ti) return NULL;	    
    }
    return  ti;
}

TreeItem* VymModel::findID (const uint &id)  
{
    BranchItem *cur=NULL;
    BranchItem *prev=NULL;
    nextBranch(cur,prev);
    while (cur) 
    {
	if (id==cur->getID() ) return cur;
	int j=0;
	while (j<cur->xlinkCount() )
	{
	    XLinkItem *xli=cur->getXLinkItemNum (j);
	    if (id==xli->getID() ) return xli;
	    j++;
	}
	j=0;
	while (j<cur->imageCount() )
	{
	    ImageItem *ii=cur->getImageNum (j);
	    if (id==ii->getID() ) return ii;
	    j++;
	}
	nextBranch(cur,prev);
    }
    return NULL;
}

TreeItem* VymModel::findUuid (const QUuid &id)  
{
    BranchItem *cur=NULL;
    BranchItem *prev=NULL;
    nextBranch(cur,prev);
    while (cur) 
    {
	if (id==cur->getUuid() ) return cur;
	int j=0;
	while (j<cur->xlinkCount() )
	{
	    XLinkItem *xli=cur->getXLinkItemNum (j);
	    if (id==xli->getUuid() ) return xli;
	    j++;
	}
	j=0;
	while (j<cur->imageCount() )
	{
	    ImageItem *ii=cur->getImageNum (j);
	    if (id==ii->getUuid() ) return ii;
	    j++;
	}
	nextBranch(cur,prev);
    }
    return NULL;
}

//////////////////////////////////////////////
// Interface 
//////////////////////////////////////////////
void VymModel::setVersion (const QString &s)
{
    version=s;
}

QString VymModel::getVersion()
{
    return version;
}

void VymModel::setTitle (const QString &s)
{
    saveState (
	QString ("setMapTitle (\"%1\")").arg(title),
	QString ("setMapTitle (\"%1\")").arg(s),
	QString ("Set title of map to \"%1\"").arg(s)
    );
    title=s;
}

QString VymModel::getTitle()
{
    return title;
}

void VymModel::setAuthor (const QString &s)
{
    saveState (
	QString ("setMapAuthor (\"%1\")").arg(author),
	QString ("setMapAuthor (\"%1\")").arg(s),
	QString ("Set author of map to \"%1\"").arg(s)
    );
    author=s;
}

QString VymModel::getAuthor()
{
    return author;
}

void VymModel::setComment (const QString &s)
{
    saveState (
	QString ("setMapComment (\"%1\")").arg(comment),
	QString ("setMapComment (\"%1\")").arg(s),
	QString ("Set comment of map")
    );
    comment=s;
}

QString VymModel::getComment ()
{
    return comment;
}

QString VymModel::getDate ()
{
    return QDate::currentDate().toString ("yyyy-MM-dd");
}

int VymModel::branchCount() 
{
    int c=0;
    BranchItem *cur=NULL;
    BranchItem *prev=NULL;
    nextBranch(cur,prev);
    while (cur) 
    {
	c++;
	nextBranch(cur,prev);
    }
    return c;
}

void VymModel::setSortFilter (const QString &s)
{
    sortFilter=s;
    emit (sortFilterChanged (sortFilter));
}

QString VymModel::getSortFilter ()
{
    return sortFilter;
}

void VymModel::setHeading(const QString &s, BranchItem *bi)
{
    if (!bi) bi=getSelectedBranch();
    if (bi)
    {
	if (bi->getHeading()==s) return;
	saveState(
	    bi,
	    "setHeading (\""+bi->getHeading()+"\")", 
	    bi,
	    "setHeading (\""+s+"\")", 
	    QString("Set heading of %1 to \"%2\"").arg(getObjectName(bi)).arg(s) );
	bi->setHeading(s );
	emitDataChanged ( bi);
	emitUpdateQueries ();
	reposition();
    }
}

QString VymModel::getHeading()
{
    TreeItem *selti=getSelectedItem();
    if (selti)
	return selti->getHeading();
    else    
	return QString();
}

void VymModel::setNote(const QString &s)
{
    TreeItem *selti=getSelectedItem();
    if (selti) 
    {
	saveState(
	    selti,
	    "setNote (\"" + selti->getNote() + "\")", 
	    selti,
	    "setNote (\"" + s + "\")", 
	    QString("Set note of %1 ").arg(getObjectName(selti)) );
    }
    selti->setNote(s);
    emitNoteChanged(selti);
    emitDataChanged(selti);
}

QString VymModel::getNote()
{
    TreeItem *selti=getSelectedItem();
    if (selti)
	return selti->getNote();
    else    
	return QString();
}

void VymModel::loadNote (const QString &fn)
{
    BranchItem *selbi=getSelectedBranch();
    if (selbi)
    {
	QString n;
	if (!loadStringFromDisk (fn,n))
	    qWarning ()<<"VymModel::loadNote Couldn't load "<<fn;
	else
	    setNote (n);
    } else
	qWarning ("VymModel::loadNote no branch selected");
}

void VymModel::saveNote (const QString &fn)
{
    BranchItem *selbi=getSelectedBranch();
    if (selbi)
    {
	QString n=selbi->getNote();
	if (n.isEmpty())
	    qWarning ()<<"VymModel::saveNote  note is empty, won't save to "<<fn;
	else
	{
	    if (!saveStringToDisk (fn,n))
		qWarning ()<<"VymModel::saveNote Couldn't save "<<fn;
	    else
		selbi->setNote (n);
	}   
    } else
	qWarning ("VymModel::saveNote no branch selected");
}

void VymModel::findDuplicateURLs()  // FIXME-3 needs GUI
{
    // Generate map containing _all_ URLs and branches
    QString u;
    QMap <QString,BranchItem*> map;
    BranchItem *cur=NULL;
    BranchItem *prev=NULL;
    nextBranch(cur,prev);
    while (cur) 
    {
	u=cur->getURL();
	if (!u.isEmpty() )
	    map.insertMulti (u,cur);
	nextBranch(cur,prev);
    }

    // Extract duplicate URLs
    QMap <QString, BranchItem*>::const_iterator i=map.constBegin();
    QMap <QString, BranchItem*>::const_iterator firstdup=map.constEnd();    //invalid
    while (i != map.constEnd())
    {
	if (i!=map.constBegin() && i.key()==firstdup.key())
	{
	    if (  i-1==firstdup )
	    {
		qDebug() << firstdup.key();
		qDebug() << " - "<< firstdup.value() <<" - "<<firstdup.value()->getHeading();
	    }	
	    qDebug() << " - "<< i.value() <<" - "<<i.value()->getHeading();
	} else
	    firstdup=i;

	++i;
    }
}

bool  VymModel::findAll (FindResultModel *rmodel, QString s, Qt::CaseSensitivity cs)   
{
    rmodel->clear();
    rmodel->setSearchString (s);
    rmodel->setSearchFlags (0);	//FIXME-4 translate cs to QTextDocument::FindFlag
    bool hit=false;

    BranchItem *cur=NULL;
    BranchItem *prev=NULL;
    nextBranch(cur,prev);

    FindResultItem *lastParent=NULL;
    while (cur) 
    {
	lastParent=NULL;
	if (cur->getHeading().contains (s,cs))
	{
	    lastParent=rmodel->addItem (cur);
	    hit=true;
	}
	QString n=cur->getNoteASCII();
	int i=0;
	int j=0;
	while ( i>=0)
	{
	    i=n.indexOf (s,i,cs); 
	    if (i>=0) 
	    {
		// If not there yet, add "parent" item
		if (!lastParent)
		{
		    lastParent=rmodel->addItem (cur);
		    hit=true;
		    if (!lastParent)
			qWarning()<<"VymModel::findAll still no lastParent?!";
		    /*
		    else
			lastParent->setSelectable (false);
		    */	
		}   

		// save index of occurence
		QString e=n.mid(i-15,30);
		n.replace('\n',' ');
		rmodel->addSubItem (lastParent,QString(tr("Note","FindAll in VymModel")+": \"...%1...\"").arg(n.mid(i-8,80)),cur,j);
		j++;
		i++;
	    }
	} 
	nextBranch(cur,prev);
    }
    return hit;
}

BranchItem* VymModel::findText (QString s,Qt::CaseSensitivity cs)
{
    if (!s.isEmpty() && s!=findString)
    {
	findReset();
	findString=s;
    }

    QTextDocument::FindFlags flags=0;
    if (cs==Qt::CaseSensitive) flags=QTextDocument::FindCaseSensitively;

    if (!findCurrent) 
    {	// Nothing found or new find process
	if (EOFind)
	    // nothing found, start again
	    EOFind=false;
	findCurrent=NULL;   
	findPrevious=NULL;  
	nextBranch (findCurrent,findPrevious);
    }	
    bool searching=true;
    bool foundNote=false;
    while (searching && !EOFind)
    {
	if (findCurrent)
	{
	    // Searching in Note
	    if (findCurrent->getNote().contains(findString,cs))
	    {
		select (findCurrent);
		if (noteEditor->findText(findString,flags)) 
		{
		    searching=false;
		    foundNote=true;
		}   
	    }
	    // Searching in Heading
	    if (searching && findCurrent->getHeading().contains (findString,cs) ) 
	    {
		select(findCurrent);
		searching=false;
	    }
	}   
	if (!foundNote)
	{
	    nextBranch(findCurrent,findPrevious);
	    if (!findCurrent) EOFind=true;
	}
    }	
    if (!searching)
	return getSelectedBranch();
    else
	return NULL;
}

void VymModel::findReset()
{   // Necessary if text to find changes during a find process
    findString.clear();
    findCurrent=NULL;
    findPrevious=NULL;
    EOFind=false;
}

void VymModel::setURL(QString url) 
{
    TreeItem *selti=getSelectedItem();
    if (selti->getURL()==url) return;
 //   if (!url.isEmpty() && (! (url.startsWith("http://") || url.startsWith("https://") ) && !url.startsWith("file://") ) )   //FIXME-3 needed?
 //	url="http://"+url;
    if (selti)
    {
	QString oldurl=selti->getURL();
	selti->setURL (url);
	saveState (
	    selti,
	    QString ("setURL (\"%1\")").arg(oldurl),
	    selti,
	    QString ("setURL (\"%1\")").arg(url),
	    QString ("set URL of %1 to %2").arg(getObjectName(selti)).arg(url)
	);
	if (url.contains("bugzilla.novell.com/"))
	    getBugzillaData(false);
	emitDataChanged (selti);
	reposition();
    }
}   

QString VymModel::getURL()  
{
    TreeItem *selti=getSelectedItem();
    if (selti)
	return selti->getURL();
    else    
	return QString();
}

QStringList VymModel::getURLs(bool ignoreScrolled)  
{
    QStringList urls;
    BranchItem *selbi=getSelectedBranch();
    BranchItem *cur=NULL;
    BranchItem *prev=NULL;
    nextBranch (cur,prev,true,selbi);
    while (cur) 
    {
	if (!cur->getURL().isEmpty()  && !(ignoreScrolled && cur->hasScrolledParent() )) 
	    urls.append( cur->getURL());
	nextBranch(cur,prev,true,selbi);
    }	
    return urls;
}


void VymModel::setFrameType(const FrameObj::FrameType &t)   //FIXME-5 not saved if there is no LMO
{
    BranchItem *bi=getSelectedBranch();
    if (bi)
    {
	BranchObj *bo=(BranchObj*)(bi->getLMO());
	if (bo)
	{
	    QString s=bo->getFrameTypeName();
	    bo->setFrameType (t);
	    saveState (bi, QString("setFrameType (\"%1\")").arg(s),
		bi, QString ("setFrameType (\"%1\")").arg(bo->getFrameTypeName()),QString ("set type of frame to %1").arg(s));
	    reposition();
	    bo->updateLinkGeometry();
	}
    }
}

void VymModel::setFrameType(const QString &s)	//FIXME-5 not saved if there is no LMO
{
    BranchItem *bi=getSelectedBranch();
    if (bi)
    {
	BranchObj *bo=(BranchObj*)(bi->getLMO());
	if (bo)
	{
	    saveState (bi, QString("setFrameType (\"%1\")").arg(bo->getFrameTypeName()),
		bi, QString ("setFrameType (\"%1\")").arg(s),QString ("set type of frame to %1").arg(s));
	    bo->setFrameType (s);
	    reposition();
	    bo->updateLinkGeometry();
	}
    }
}

void VymModel::toggleFrameIncludeChildren ()	  
{
    BranchItem *bi=getSelectedBranch();
    if (bi)
    {
	bool b=bi->getFrameIncludeChildren();
	setFrameIncludeChildren (!b);
    }
}

void VymModel::setFrameIncludeChildren (bool b)	    
{
    BranchItem *bi=getSelectedBranch();
    if (bi)
    {
	QString u= b ? "false" : "true";
	QString r=!b ? "false" : "true";
	
	saveState(
	    bi,
	    QString("setFrameIncludeChildren(%1)").arg(u),
	    bi, 
	    QString("setFrameIncludeChildren(%1)").arg(r),
	    QString("Include children in %1").arg(getObjectName(bi))
	);  
	bi->setFrameIncludeChildren (b);
	emitDataChanged (bi);
	reposition();
    }
}

void VymModel::setFramePenColor(const QColor &c)    //FIXME-4 not saved if there is no LMO

{
    BranchItem *bi=getSelectedBranch();
    if (bi)
    {
	BranchObj *bo=(BranchObj*)(bi->getLMO());
	if (bo)
	{
	    saveState (bi, QString("setFramePenColor (\"%1\")").arg(bo->getFramePenColor().name() ),
		bi, QString ("setFramePenColor (\"%1\")").arg(c.name() ),QString ("set pen color of frame to %1").arg(c.name() ));
	    bo->setFramePenColor (c);
	}   
    }	
}

void VymModel::setFrameBrushColor(const QColor &c)  //FIXME-4 not saved if there is no LMO
{
    BranchItem *bi=getSelectedBranch();
    if (bi)
    {
	BranchObj *bo=(BranchObj*)(bi->getLMO());
	if (bo)
	{
	    saveState (bi, QString("setFrameBrushColor (\"%1\")").arg(bo->getFrameBrushColor().name() ),
		bi, QString ("setFrameBrushColor (\"%1\")").arg(c.name() ),QString ("set brush color of frame to %1").arg(c.name() ));
	    bo->setFrameBrushColor (c);
	    bi->setBackgroundColor (c);	//FIXME-4 redundant with above
	}   
    }	
}

void VymModel::setFramePadding (const int &i) //FIXME-4 not saved if there is no LMO
{
    BranchItem *bi=getSelectedBranch();
    if (bi)
    {
	BranchObj *bo=(BranchObj*)(bi->getLMO());
	if (bo)
	{
	    saveState (bi, QString("setFramePadding (\"%1\")").arg(bo->getFramePadding() ),
		bi, QString ("setFramePadding (\"%1\")").arg(i),QString ("set brush color of frame to %1").arg(i));
	    bo->setFramePadding (i);
	    reposition();
	    bo->updateLinkGeometry();
	}   
    }	
}

void VymModel::setFrameBorderWidth(const int &i) //FIXME-4 not saved if there is no LMO
{
    BranchItem *bi=getSelectedBranch();
    if (bi)
    {
	BranchObj *bo=(BranchObj*)(bi->getLMO());
	if (bo)
	{
	    saveState (bi, QString("setFrameBorderWidth (\"%1\")").arg(bo->getFrameBorderWidth() ),
		bi, QString ("setFrameBorderWidth (\"%1\")").arg(i),QString ("set border width of frame to %1").arg(i));
	    bo->setFrameBorderWidth (i);
	    reposition();
	    bo->updateLinkGeometry();
	}   
    }	
}

void VymModel::setIncludeImagesVer(bool b)
{
    BranchItem *bi=getSelectedBranch();
    if (bi && b!=bi->getIncludeImagesVer() )
    {
	QString u= b ? "false" : "true";
	QString r=!b ? "false" : "true";
	
	saveState(
	    bi,
	    QString("setIncludeImagesVertically (%1)").arg(u),
	    bi, 
	    QString("setIncludeImagesVertically (%1)").arg(r),
	    QString("Include images vertically in %1").arg(getObjectName(bi))
	);  
	bi->setIncludeImagesVer(b);
	emitDataChanged ( bi);   
	reposition();
    }	
}

void VymModel::setIncludeImagesHor(bool b)  
{
    BranchItem *bi=getSelectedBranch();
    if (bi && b!=bi->getIncludeImagesHor() )
    {
	QString u= b ? "false" : "true";
	QString r=!b ? "false" : "true";
	
	saveState(
	    bi,
	    QString("setIncludeImagesHorizontally (%1)").arg(u),
	    bi, 
	    QString("setIncludeImagesHorizontally (%1)").arg(r),
	    QString("Include images horizontally in %1").arg(getObjectName(bi))
	);  
	bi->setIncludeImagesHor(b);
	emitDataChanged ( bi);
	reposition();
    }	
}

void VymModel::setHideLinkUnselected (bool b) 
{
    TreeItem *ti=getSelectedItem();
    if (ti && (ti->getType()==TreeItem::Image ||ti->isBranchLikeType()))
    {
	QString u= b ? "false" : "true";
	QString r=!b ? "false" : "true";
	
	saveState(
	    ti,
	    QString("setHideLinkUnselected (%1)").arg(u),
	    ti, 
	    QString("setHideLinkUnselected (%1)").arg(r),
	    QString("Hide link of %1 if unselected").arg(getObjectName(ti))
	);  
	((MapItem*)ti)->setHideLinkUnselected(b);
    }
}

void VymModel::setHideExport(bool b, TreeItem *ti)
{
    if (!ti) ti=getSelectedItem();
    if (ti && 
	(ti->getType()==TreeItem::Image ||ti->isBranchLikeType()) &&
	ti->hideInExport() !=b
	)
    {
	ti->setHideInExport (b);
	QString u= b ? "false" : "true";
	QString r=!b ? "false" : "true";
	
	saveState(
	    ti,
	    QString ("setHideExport (%1)").arg(u),
	    ti,
	    QString ("setHideExport (%1)").arg(r),
	    QString ("Set HideExport flag of %1 to %2").arg(getObjectName(ti)).arg (r)
	);  
	    emitDataChanged(ti);
	    emitSelectionChanged();
	reposition(); 
    }
}

void VymModel::toggleHideExport()
{
    QList <TreeItem*> selItems=getSelectedItems();
    if (selItems.count()>0 )
    {
	foreach (TreeItem* ti, selItems)
	{
	    bool b=!ti->hideInExport();
	    setHideExport (b,ti );
	}
    }
}

void VymModel::toggleTask() 
{
    BranchItem *selbi=getSelectedBranch();
    if (selbi) 
    {
	saveStateChangingPart (
	    selbi,
	    selbi,
	    QString ("toggleTask()"),
	    QString ("Toggle task of %1").arg(getObjectName (selbi)) );
	Task *task=selbi->getTask();
	if (!task )
	{
	    task=taskModel->createTask (selbi);
	    taskEditor->select(task); 
	}
	else
	    taskModel->deleteTask (task);

	emitDataChanged(selbi);
	emitSelectionChanged();
	reposition();
    }
}

void VymModel::cycleTaskStatus(bool reverse)
{
    BranchItem *selbi=getSelectedBranch();
    if (selbi) 
    {
	Task *task=selbi->getTask();
	if (task) 
	{
	    saveStateChangingPart (
		selbi,
		selbi,
		QString ("cycleTask()"),
		QString ("Toggle task of %1").arg(getObjectName (selbi)) );
	    task->cycleStatus(reverse);
	    task->setDateModified();
	    
	    // make sure task is still visible
	    taskEditor->select (task);
            emitDataChanged(selbi);
            reposition();
	}
    }
}

bool VymModel::setTaskSleep(const QString &s) 
{
    BranchItem *selbi=getSelectedBranch();
    if (selbi && !s.isEmpty() ) 
    {
	Task *task=selbi->getTask();
	if (task ) 
	{
            bool ok;
            int n=s.toInt(&ok);
            if (!ok)
            {
                // Is s a date?
                QDate d=QDate::fromString(s,Qt::ISODate);
                d=QDate::fromString(s,Qt::ISODate);
                if (d.isValid())
                    // ISO date YYYY-MM-DD
                    ok=true;
                else
                {
                    d=QDate::fromString(s,Qt::DefaultLocaleShortDate);
                    if (d.isValid()) 
                        // Locale date, e.g. 24 Dec 2012
                        ok=true;
                    else
                    {
                        QRegExp re ("(\\d+).(\\d+).(\\d+)");
                        re.setMinimal(false);
                        int pos=re.indexIn(s);
                        QStringList list=re.capturedTexts();
                        if (pos>=0)
                        {
                            // German formate, e.g. 24.12.2012
                            d=QDate(list.at(3).toInt(), list.at(2).toInt(), list.at(1).toInt());
                            ok=true;
                        } else
                        {
                            re.setPattern("(\\d+).(\\d+).");
                            pos=re.indexIn(s);
                            list=re.capturedTexts();
                            if (pos>=0)
                            {
                                // Short German formate, e.g. 24.12.
                                int month=list.at(2).toInt();
                                int day=list.at(1).toInt();
                                int year=QDate::currentDate().year();
                                d=QDate(year, month, day);
                                if (QDate::currentDate().daysTo(d) < 0)
                                {
                                    year++;
                                    d=QDate(year, month, day);
                                }
                                ok=true;
                            } else
                            {
                                re.setPattern("(\\d+).(\\d+).");
                            }
                        }
                    }
                }
                if (ok) n=QDate::currentDate().daysTo(d);
            }

            if (ok)
            {
                int oldsleep=task->getDaysSleep();
                task->setDateSleep (n);
                task->setDateModified();
                saveState (
                    selbi,
                    QString("setTaskSleep (%1)").arg(oldsleep),
                    selbi,
                    QString("setTaskSleep (%1)").arg(n),
                    QString("setTaskSleep (%1)").arg(n) );
                emitDataChanged (selbi);
                reposition();
                return true;
            }
	}
    }
    return false;
}

int VymModel::taskCount()
{
    return taskModel->count (this);
}

void VymModel::addTimestamp()	//FIXME-4 new function, localize
{
    BranchItem *selbi=addNewBranch();
    if (selbi)
    {
	QDate today=QDate::currentDate();
	QChar c='0';
	selbi->setHeading (QString ("%1-%2-%3")
	    .arg(today.year(),4,10,c)
	    .arg(today.month(),2,10,c)
	    .arg(today.day(),2,10,c));
	emitDataChanged ( selbi);	
	reposition();
	select (selbi);
    }
}


void VymModel::copy()	
{
    TreeItem *selti=getSelectedItem();
    if (selti &&
	(selti->getType() == TreeItem::Branch || 
	selti->getType() == TreeItem::MapCenter  ||
	selti->getType() == TreeItem::Image ))
    {
	// Copy to global clipboard
	QString saveFile=saveToDir (clipboardDir, clipboardFile, true, QPointF(), selti);
	if (!saveStringToDisk(clipboardDir + "/" + clipboardFile,saveFile))
	    qWarning ("ME::saveStringToDisk failed!");

	clipboardEmpty=false;

	if (redosAvail == 0)
	{
	    // Copy also to history
	    QString s=getSelectString(selti);
	    saveState (PartOfMap, s, "nop ()", s, "copy ()","Copy selection to clipboard",selti  );
	    curClipboard=curStep;
	}
	updateActions();
    }	    
}

void VymModel::paste()	
{   
    BranchItem *selbi=getSelectedBranch();
    if (selbi)
    {
	saveStateChangingPart(
	    selbi,
	    selbi,
	    QString ("paste ()"),
	    QString("Paste")
	);
	bool zippedOrg=zipped;
	loadMap (clipboardDir+"/"+clipboardFile,ImportAdd, VymMap,SlideContent);
	zipped=zippedOrg;
	reposition();
    }
}

void VymModel::cut()	
{
    TreeItem *selti=getSelectedItem();
    if ( selti && (selti->isBranchLikeType() ||selti->getType()==TreeItem::Image))
    {
	copy();
	deleteSelection();
	reposition();
    }
}

bool VymModel::moveUp(BranchItem *bi)
{
    bool oldState=blockSaveState;
    blockSaveState=true;
    bool result=false;
    if (bi && bi->canMoveUp()) 
	result=relinkBranch (bi,(BranchItem*)bi->parent(),bi->num()-1,false);
    blockSaveState=oldState;
    return result;
}

void VymModel::moveUp()	
{
    BranchItem *selbi=getSelectedBranch();
    if (selbi)
    {
	QString oldsel=getSelectString(selbi);
	if (moveUp (selbi))
	{
	    saveState (
		getSelectString(selbi),"moveDown ()",
		oldsel,"moveUp ()",
		QString("Move up %1").arg(getObjectName(selbi)));
	    select (selbi);		
	}
    }
}

bool VymModel::moveDown(BranchItem *bi)	
{
    bool oldState=blockSaveState;
    blockSaveState=true;
    bool result=false;
    if (bi && bi->canMoveDown()) 
	result=relinkBranch (bi,(BranchItem*)bi->parent(),bi->num()+1,false);
    blockSaveState=oldState;
    return result;
}

void VymModel::moveDown()   
{
    BranchItem *selbi=getSelectedBranch();
    if (selbi)
    {
	QString oldsel=getSelectString(selbi);
	if ( moveDown(selbi))
	{
	    saveState (
		getSelectString(selbi),"moveUp ()",
		oldsel,"moveDown ()",
		QString("Move down %1").arg(getObjectName(selbi)));
	    select (selbi);
	}
    }
}

void VymModel::detach()	
{
    BranchItem *selbi=getSelectedBranch();
    if (selbi && selbi->depth()>0)
    {
	// if no relPos have been set before, try to use current rel positions   
	if (selbi->getLMO())
	    for (int i=0; i<selbi->branchCount();++i)
		selbi->getBranchNum(i)->getBranchObj()->setRelPos();
	
	QString oldsel=getSelectString();
	int n=selbi->num();
	QPointF p;
	BranchObj *bo=selbi->getBranchObj();
	if (bo) p=bo->getAbsPos();
	QString parsel=getSelectString(selbi->parent());
	if ( relinkBranch (selbi,rootItem,-1,true) )	
	    saveState (
		getSelectString (selbi),
		QString("relinkTo (\"%1\",%2,%3,%4)").arg(parsel).arg(n).arg(p.x()).arg(p.y()),
		oldsel,
		"detach ()",
		QString("Detach %1").arg(getObjectName(selbi))
	    );
    }
}

void VymModel::sortChildren(bool inverse) 
{
    BranchItem* selbi=getSelectedBranch();
    if (selbi)
    {
	if(selbi->branchCount()>1)
	{
	    if (!inverse)
		saveStateChangingPart(
		    selbi,selbi, "sortChildren ()",
		    QString("Sort children of %1").arg(getObjectName(selbi)));
	    else	    
		saveStateChangingPart(
		    selbi,selbi, "sortChildren (false)",
		    QString("Inverse sort children of %1").arg(getObjectName(selbi)));

	    selbi->sortChildren(inverse);
	    select(selbi);
	    reposition();
	}
    }
}

BranchItem* VymModel::createMapCenter()
{
    BranchItem *newbi=addMapCenter (QPointF (0,0) );
    return newbi;
}

BranchItem* VymModel::createBranch(BranchItem *dst) 
{
    if (dst)
	return addNewBranchInt (dst,-2);
    else
	return NULL;
}

ImageItem* VymModel::createImage(BranchItem *dst)
{
    if (dst)
    {
	QModelIndex parix;
	int n;

	QList<QVariant> cData;
	cData << "new" << "undef";

	ImageItem *newii=new ImageItem(cData) ;	
	//newii->setHeading (QApplication::translate("Heading of new image in map", "new image"));

	emit (layoutAboutToBeChanged() );

	    parix=index(dst);
	    if (!parix.isValid()) qDebug() << "VM::createII invalid index\n";
	    n=dst->getRowNumAppend(newii);
	    beginInsertRows (parix,n,n);
	    dst->appendChild (newii);	
	    endInsertRows ();

	emit (layoutChanged() );

	// save scroll state. If scrolled, automatically select
	// new branch in order to tmp unscroll parent...
	newii->createMapObj();
	latestAddedItem=newii;
	reposition();
	return newii;
    } 
    return NULL;
}

bool VymModel::createLink(Link *link)
{
    BranchItem *begin=link->getBeginBranch();
    BranchItem *end  =link->getEndBranch();

    if (!begin || !end)
    {
	qWarning ()<<"VM::createXLinkNew part of XLink is NULL";
	return false;
    }

    if (begin==end)
    {
	if (debug) qDebug()<<"VymModel::createLink begin==end, aborting";
	return false;
    }

    // check, if link already exists
    foreach (Link* l, xlinks)
    {
	if ( (l->getBeginBranch()==begin && l->getEndBranch()==end ) ||
	     (l->getBeginBranch()==end   && l->getEndBranch()==begin) )
	{
	    qWarning()<<"VymModel::createLink link exists already, aborting";
	    return false;
	}
    }

    QModelIndex parix;
    int n;

    QList<QVariant> cData;

    cData << "new Link begin"<<"undef";
    XLinkItem *newli=new XLinkItem(cData) ;	
    newli->setLink (link);
    link->setBeginLinkItem (newli);

    emit (layoutAboutToBeChanged() );

	parix=index(begin);
	n=begin->getRowNumAppend(newli);
	beginInsertRows (parix,n,n);
	begin->appendChild (newli);	
	endInsertRows ();

    cData.clear();
    cData << "new Link end"<<"undef";
    newli=new XLinkItem(cData) ;	
    newli->setLink (link);
    link->setEndLinkItem (newli);

	parix=index(end);
	n=end->getRowNumAppend(newli);
	beginInsertRows (parix,n,n);
	end->appendChild (newli);	
	endInsertRows ();

    emit (layoutChanged() );

    xlinks.append (link);
    link->activate();

    latestAddedItem=newli;

    if (!link->getMO() ) 
    {
	link->createMapObj();
	reposition();
    } else
        link->updateLink();
    return true;
}

QColor VymModel::getXLinkColor()
{
    Link *l=getSelectedXLink();
    if (l)
	return l->getPen().color();
    else
	return QColor();
}

int VymModel::getXLinkWidth()
{
    Link *l=getSelectedXLink();
    if (l)
	return l->getPen().width();
    else
	return -1;
}

Qt::PenStyle VymModel::getXLinkPenStyle()
{
    Link *l=getSelectedXLink();
    if (l)
	return l->getPen().style();
    else
	return Qt::NoPen;
}

AttributeItem* VymModel::addAttribute()	    // FIXME-5 savestate missing

{
    BranchItem* selbi=getSelectedBranch();
    if (selbi)
    {
	QList<QVariant> cData;
	cData << "new attribute" << "undef";
	AttributeItem *a=new AttributeItem (cData);
	a->setType (AttributeItem::FreeString);
	a->setKey   ("Foo Attrib");
	a->setValue ("Att val");

	if (addAttribute (selbi,a)) return a;
    }
    return NULL;
}

AttributeItem* VymModel::addAttribute(BranchItem *dst,AttributeItem *ai){
    if (dst)
    {
	emit (layoutAboutToBeChanged() );

	QModelIndex parix=index(dst);
	int n=dst->getRowNumAppend (ai);
	beginInsertRows (parix,n,n);	
	dst->appendChild (ai);	
	endInsertRows ();

	emit (layoutChanged() );

	ai->createMapObj(mapEditor->getScene() );   
	reposition();
	return ai;
    }
    return NULL;
}

BranchItem* VymModel::addMapCenter (bool saveStateFlag)
{
    if (!hasContextPos) 
    {
	// E.g. when called via keypresss:
	// Place new MCO in middle of existing ones,
	// Useful for "brainstorming" mode...
	contextPos=QPointF();
	BranchItem *bi;
	BranchObj *bo;
	for (int i=0;i<rootItem->branchCount();++i)
	{
	    bi=rootItem->getBranchNum (i);
	    bo=(BranchObj*)bi->getLMO();
	    if (bo) contextPos+=bo->getAbsPos();
	    
	}	    
	if (rootItem->branchCount()>1) 
	    contextPos*=1/(qreal)(rootItem->branchCount());
    }


    BranchItem *bi=addMapCenter (contextPos);
    updateActions();
    emitShowSelection();
    if (saveStateFlag) saveState (
	bi,
	"delete()",
	NULL,
	QString ("addMapCenter (%1,%2)").arg (contextPos.x()).arg(contextPos.y()),
	QString ("Adding MapCenter to (%1,%2)").arg (contextPos.x()).arg(contextPos.y())
    );	
    emitUpdateLayout();	
    return bi;	
}

BranchItem* VymModel::addMapCenter(QPointF absPos)  
// createMapCenter could then probably be merged with createBranch
{

    // Create TreeItem
    QModelIndex parix=index(rootItem);

    QList<QVariant> cData;
    cData << "VM:addMapCenter" << "undef";
    BranchItem *newbi=new BranchItem (cData,rootItem);
    newbi->setHeading (QApplication::translate("Heading of mapcenter in new map", "New map"));
    int n=rootItem->getRowNumAppend (newbi);

    emit (layoutAboutToBeChanged() );
    beginInsertRows (parix,n,n);

    rootItem->appendChild (newbi);

    endInsertRows();
    emit (layoutChanged() );

    // Create MapObj
    newbi->setPositionMode (MapItem::Absolute);
    BranchObj *bo=newbi->createMapObj(mapEditor->getScene() );
    if (bo) bo->move (absPos);
	
    return newbi;
}

BranchItem* VymModel::addNewBranchInt(BranchItem *dst,int pos)
{
    // Depending on pos:
    // -3	insert in children of parent  above selection 
    // -2	add branch to selection 
    // -1	insert in children of parent below selection 
    // 0..n	insert in children of parent at pos

    // Create TreeItem
    QList<QVariant> cData;
    cData << "" << "undef";

    BranchItem *parbi=dst;
    int n;
    BranchItem *newbi=new BranchItem (cData);	

    emit (layoutAboutToBeChanged() );

    if (pos==-2)
    {
	n=parbi->getRowNumAppend (newbi);
	beginInsertRows (index(parbi), n, n);	
	parbi->appendChild (newbi); 
	endInsertRows ();
    }else if (pos==-1 || pos==-3)
    {
	// insert below selection
	parbi=(BranchItem*)dst->parent();
	n=dst->childNumber() + (3+pos)/2;   //-1 |-> 1;-3 |-> 0
	beginInsertRows (index(parbi), n, n);	
	parbi->insertBranch(n,newbi);	
	endInsertRows ();
    } else  
    {	// pos >= 0
	n=parbi->getRowNumAppend (newbi) - (parbi->branchCount()-pos);
	beginInsertRows (index(parbi), n, n);	
	parbi->insertBranch(pos,newbi);	
	endInsertRows ();
    }
    emit (layoutChanged() );

    // Set color of heading to that of parent
    newbi->setHeadingColor (parbi->getHeadingColor());

    // save scroll state. If scrolled, automatically select
    // new branch in order to tmp unscroll parent...
    newbi->createMapObj(mapEditor->getScene());
    reposition();
    return newbi;
}   

BranchItem* VymModel::addNewBranch(BranchItem *bi, int pos)
{
    BranchItem *newbi=NULL;
    if (!bi) bi=getSelectedBranch();

    if (bi)
    {
	QString redosel=getSelectString(bi);
	newbi=addNewBranchInt (bi,pos);
	QString undosel=getSelectString(newbi);

	if (newbi)
	{
	    saveState(
		undosel,	
		"delete ()",
		redosel,
		QString ("addBranch (%1)").arg(pos),
		QString ("Add new branch to %1").arg(getObjectName(bi)));	

	    reposition();
	    latestAddedItem=newbi;
	    // In Network mode, the client needs to know where the new branch is,
	    // so we have to pass on this information via saveState.
	    // TODO: Get rid of this positioning workaround
	    /* FIXME-4  network problem:  QString ps=qpointfToString (newbo->getAbsPos());
	    sendData ("selectLatestAdded ()");
	    sendData (QString("move %1").arg(ps));
	    sendSelection();
	    */
	}
    }	
    return newbi;
}


BranchItem* VymModel::addNewBranchBefore()  
{
    BranchItem *newbi=NULL;
    BranchItem *selbi=getSelectedBranch();
    if (selbi && selbi->getType()==TreeItem::Branch)
	 // We accept no MapCenter here, so we _have_ a parent
    {
	// add below selection
	newbi=addNewBranchInt (selbi,-1);

	if (newbi)
	{
	    //newbi->move2RelPos (p);

	    // Move selection to new branch
	    relinkBranch (selbi,newbi,0,true);

	    // Use color of child instead of parent
	    newbi->setHeadingColor (selbi->getHeadingColor() );
	    emitDataChanged (newbi);

	    saveState (newbi, "deleteKeepChildren ()", newbi, "addBranchBefore ()", 
		QString ("Add branch before %1").arg(getObjectName(selbi)));
	}
    }	
    return newbi;
}

bool VymModel::relinkBranch (
    BranchItem *branch, 
    BranchItem *dst, 
    int pos, 
    bool updateSelection,
    QPointF orgPos)
{
    if (branch && dst)
    {
	// Check if we relink to ourselves
	if (dst->isChildOf (branch) ) return false;
	 
	if (updateSelection) unselectAll();
 
	// Do we need to update frame type?
	bool keepFrame=true;
	 
	// Save old position for savestate
	QString preSelStr=getSelectString (branch);
	QString preNum=QString::number (branch->num(),10);
	QString preParStr=getSelectString (branch->parent());

	emit (layoutAboutToBeChanged() );
	BranchItem *branchpi=(BranchItem*)branch->parent();
	// Remove at current position
	int n=branch->childNum();

	beginRemoveRows (index(branchpi),n,n);
	branchpi->removeChild (n);
	endRemoveRows();

	if (pos<0 ||pos>dst->branchCount() ) pos=dst->branchCount();

	// Append as last branch to dst
	if (dst->branchCount()==0)
	    n=0;
	else	
	    n=dst->getFirstBranch()->childNumber(); 
	beginInsertRows (index(dst),n+pos,n+pos);
	dst->insertBranch (pos,branch);
	endInsertRows();

	// Correct type if necessesary
	if ( branch->getType()==TreeItem::MapCenter && branch->depth() >0 ) 
	{
	    branch->setType(TreeItem::Branch);
	    keepFrame=false;
	}

	// reset parObj, fonts, frame, etc in related LMO or other view-objects
	branch->updateStyles(keepFrame);

	emit (layoutChanged() );
	reposition();	// both for moveUp/Down and relinking

	// Savestate
	QString postSelStr=getSelectString(branch);
	QString postNum=QString::number (branch->num(),10);

	QPointF savePos;
	LinkableMapObj *lmosel=branch->getLMO();
	if (lmosel) savePos=lmosel->getAbsPos();

	if (!blockSaveState)
	{   // Don't build strings when moving up/down
	    QString undoCom="relinkTo (\""+ 
		preParStr+ "\"," + preNum  +"," + 
		QString ("%1,%2").arg(orgPos.x()).arg(orgPos.y())+ ")";

	    QString redoCom="relinkTo (\""+ 
		getSelectString (dst)  + "\"," + postNum + "," +
		QString ("%1,%2").arg(savePos.x()).arg(savePos.y())+ ")";

	    saveState (
		postSelStr,undoCom,
		preSelStr, redoCom,
		QString("Relink %1 to %2").arg(getObjectName(branch)).arg(getObjectName(dst)) );
	}

	// New parent might be invisible
	branch->updateVisibility();

	if (dst->isScrolled() )
	{
	    if (updateSelection) select (dst);
	}
	else	
	    if (updateSelection) select (branch);
	return true;
    }
    return false;
}

bool VymModel::relinkImage (ImageItem *image, BranchItem *dst)
{
    if (image && dst)
    {
	emit (layoutAboutToBeChanged() );

	BranchItem *pi=(BranchItem*)(image->parent());
	QString oldParString=getSelectString (pi);
	// Remove at current position
	int n=image->childNum();
	beginRemoveRows (index(pi),n,n);
	pi->removeChild (n);
	endRemoveRows();

	// Add at dst
	QModelIndex dstix=index(dst);
	n=dst->getRowNumAppend (image);
	beginInsertRows (dstix,n,n+1);	
	dst->appendChild (image);   
	endInsertRows ();

	// Set new parent also for lmo
	if (image->getLMO() && dst->getLMO() )
	    image->getLMO()->setParObj (dst->getLMO() );

	emit (layoutChanged() );
	saveState(
	    image,
	    QString("relinkTo (\"%1\")").arg(oldParString), 
	    image,
	    QString ("relinkTo (\"%1\")").arg(getSelectString (dst)),
	    QString ("Relink floatimage to %1").arg(getObjectName(dst)));
	return true;	
    }
    return false;
}

void VymModel::cleanupItems()
{

    // Search for double entries (should not be necessary now) //FIXME-5
    // --> not implemented yet

    while (!deleteLaterIDs.isEmpty())
    {
	TreeItem *ti=findID (deleteLaterIDs.takeFirst());
	if (ti) deleteItem (ti);
    }
}

void VymModel::deleteLater(uint id)	
{
    if (!deleteLaterIDs.contains(id))
	deleteLaterIDs.append (id);
}

void VymModel::deleteSelection()    
{
    QList <uint> selectedIDs=getSelectedIDs();
    foreach (uint id, selectedIDs)
    {
	TreeItem *ti=findID (id);
	if (ti && ti->isBranchLikeType ())
	{   // Delete branch
	    BranchItem *selbi=(BranchItem*)ti;
	    unselectAll();
	    saveStateRemovingPart (selbi, QString ("Delete %1").arg(getObjectName(selbi)));

	    BranchItem *pi=(BranchItem*)(deleteItem (selbi));
	    if (pi)
	    {
		if (pi->isScrolled() && pi->branchCount()==0)
		    pi->unScroll();
		emitDataChanged(pi);
		select (pi);
	    } else
		emitDataChanged(rootItem); 
	    ti=NULL;		
	}

	// Delete other item
	if (ti)
	{
	    TreeItem *pi=ti->parent(); 
	    if (!pi) return;
	    if (ti->getType()==TreeItem::Image || ti->getType()==TreeItem::Attribute||ti->getType()==TreeItem::XLink)
	    {
		saveStateChangingPart(
		    pi, 
		    ti,
		    "delete ()",
		    QString("Delete %1").arg(getObjectName(ti))
		);
		unselectAll();
		deleteItem (ti);
		emitDataChanged (pi);
		select (pi);
		reposition();
	    } else
		qWarning ("VymmModel::deleteSelection()  unknown type?!");
	}
    }
}

void VymModel::deleteKeepChildren(bool saveStateFlag)
//deleteKeepChildren FIXME-3+ does not work yet for mapcenters 
//deleteKeepChildren FIXME-3+ children of scrolled branch stay invisible...
{
    BranchItem *selbi=getSelectedBranch();
    BranchItem *pi;
    if (selbi)
    {
	// Don't use this on mapcenter
	if (selbi->depth()<1) return;

	pi=(BranchItem*)(selbi->parent());
	// Check if we have children at all to keep
	if (selbi->branchCount()==0) 
	{
	    deleteSelection();
	    return;
	}

	QPointF p;
	if (selbi->getLMO()) p=selbi->getLMO()->getRelPos();
	if (saveStateFlag) saveStateChangingPart(
	    pi,
	    pi,
	    "deleteKeepChildren ()",
	    QString("Remove %1 and keep its children").arg(getObjectName(selbi))
	);

	QString sel=getSelectString(selbi);
	unselectAll();
	bool oldSaveState=blockSaveState;
	blockSaveState=true;
	int pos=selbi->num();
	BranchItem *bi=selbi->getFirstBranch();
	while (bi)
	{
	    relinkBranch (bi,pi,pos,true);
	    bi=selbi->getFirstBranch();
	    pos++;
	}
	deleteItem (selbi);
	reposition();
	emitDataChanged(pi);
	select (sel);
	BranchObj *bo=getSelectedBranchObj();
	if (bo) 
	{
	    bo->move2RelPos (p);
	    reposition();
	}
	blockSaveState=oldSaveState;
    }	
}

void VymModel::deleteChildren()	    

{
    BranchItem *selbi=getSelectedBranch();
    if (selbi)
    {	    
	saveStateChangingPart(
	    selbi, 
	    selbi,
	    "deleteChildren ()",
	    QString( "Remove children of branch %1").arg(getObjectName(selbi))
	);
	emit (layoutAboutToBeChanged() );

	QModelIndex ix=index (selbi);
	int n=selbi->branchCount()-1;
	beginRemoveRows (ix,0,n);
	removeRows (0,n+1,ix);
	endRemoveRows();
	if (selbi->isScrolled()) unscrollBranch (selbi);
	emit (layoutChanged() );
	reposition();
    }	
}

TreeItem* VymModel::deleteItem (TreeItem *ti)
{
    if (ti)
    {
	TreeItem *pi=ti->parent();
	//qDebug()<<"VM::deleteItem  start ti="<<ti<<"  "<<ti->getHeading()<<"  pi="<<pi<<"="<<pi->getHeading();

	TreeItem::Type t=ti->getType();
	
	QModelIndex parentIndex=index(pi);

	emit (layoutAboutToBeChanged() );

	int n=ti->childNum();
	beginRemoveRows (parentIndex,n,n);
	removeRows (n,1,parentIndex);
	endRemoveRows();

	// Size of parent branch might change when deleting images
	if (t==TreeItem::Image)
	{
	    BranchObj *bo=(BranchObj*) ( ((BranchItem*)pi)->getMO() );
	    if (bo) bo->calcBBoxSize();
	}

	reposition();

	emit (layoutChanged() );
	emitUpdateQueries ();
	if (!cleaningUpLinks) cleanupItems();

	//qDebug()<<"VM::deleteItem  end   ti="<<ti;
	if (pi->depth()>=0) return pi;
    }	
    return NULL;
}

void VymModel::deleteLink(Link* l)  
{
    if (xlinks.removeOne (l)) delete (l);
}

void VymModel::clearItem (TreeItem *ti)
{
    if (ti)
    {
	// Clear task (or other data in item itself)
	ti->clear();

	QModelIndex parentIndex=index(ti);
	if (!parentIndex.isValid()) return;

	int n=ti->childCount();
	if (n==0) return;

	emit (layoutAboutToBeChanged() );

	beginRemoveRows (parentIndex,0,n-1);
	removeRows (0,n,parentIndex);
	endRemoveRows();


	reposition();

	emit (layoutChanged() );

    }	
    return ;
}

bool VymModel::scrollBranch(BranchItem *bi)
{
    if (bi) 
    {
	if (bi->isScrolled()) return false;
	if (bi->branchCount()==0) return false;
	if (bi->depth()==0) return false;
	if (bi->toggleScroll())
	{
	    QString u,r;
	    r="scroll";
	    u="unscroll";
	    saveState(
		bi,
		QString ("%1 ()").arg(u),
		bi,
		QString ("%1 ()").arg(r),
		QString ("%1 %2").arg(r).arg(getObjectName(bi))
	    );
	    emitDataChanged(bi);
	    emitSelectionChanged();
	    reposition();
	    mapEditor->getScene()->update(); //Needed for _quick_ update,  even in 1.13.x 
	    return true;
	}
    }	
    return false;
}

bool VymModel::unscrollBranch(BranchItem *bi)
{
    if (bi)
    {
	if (!bi->isScrolled()) return false;
	if (bi->toggleScroll())
	{
	    QString u,r;
	    u="scroll";
	    r="unscroll";
	    saveState(
		bi,
		QString ("%1 ()").arg(u),
		bi,
		QString ("%1 ()").arg(r),
		QString ("%1 %2").arg(r).arg(getObjectName(bi))
	    );
	    emitDataChanged(bi);
	    emitSelectionChanged();
	    reposition();
	    mapEditor->getScene()->update(); //Needed for _quick_ update,  even in 1.13.x 
	    return true;
	}   
    }	
    return false;
}

void VymModel::toggleScroll()	
{
    BranchItem *selbi=getSelectedBranch();
    if (selbi)
    {
	if (selbi->isScrolled())
	    unscrollBranch (selbi);
	else
	    scrollBranch (selbi);
	// Note: saveState & reposition are called in above functions
    }
}

void VymModel::unscrollChildren() 
{
    BranchItem *selbi=getSelectedBranch();
    if (selbi)
    {
	saveStateChangingPart(
	    selbi,
	    selbi,
	    QString ("unscrollChildren ()"),
	    QString ("unscroll all children of %1").arg(getObjectName(selbi))
	);  
        BranchItem *prev=NULL;
        BranchItem *cur=NULL;
        nextBranch (cur,prev,true,selbi);
	while (cur) 
	{
	    if (cur->isScrolled())
	    {
		cur->toggleScroll(); 
		emitDataChanged (cur);
            }
	    nextBranch (cur,prev,true,selbi);
	}   
	updateActions();
	reposition();
	// Would this help??? emitSelectionChanged();	
    }	
}

void VymModel::setScale(qreal xn, qreal yn) 
{
    ImageItem *selii=getSelectedImage();
    if (selii)
    {
	qreal sx=selii->getScaleX();
	qreal sy=selii->getScaleY();
	selii->setScale (xn,yn);
	saveState ( 
	    selii,
	    QString ("setScale(%1,%2)").arg(sx).arg(sy),
	    selii,
	    QString ("setScale(%1,%2)").arg(xn).arg(yn),
	    QString ("Scale %1").arg(getObjectName(selii))
	);  
	reposition();
    }	
}

void VymModel::growSelectionSize()  //FIXME-3 Also for heading in BranchItem?
{
    ImageItem *selii=getSelectedImage();
    if (selii)
    {
	qreal f=0.05;
	qreal sx=selii->getScaleX();
	qreal sy=selii->getScaleY();
	setScale (sx+f,sy+f);
    }	
}

void VymModel::shrinkSelectionSize() 
{
    ImageItem *selii=getSelectedImage();
    if (selii)
    {
	qreal f=0.05;
	qreal sx=selii->getScaleX();
	qreal sy=selii->getScaleY();
	setScale (sx-f,sy-f);
    }	
}

void VymModel::resetSelectionSize() 
{
    ImageItem *selii=getSelectedImage();
    if (selii) setScale (1,1);
}

void VymModel::emitExpandAll()	
{
    emit (expandAll() );
}

void VymModel::emitExpandOneLevel() 
{
    emit (expandOneLevel () );
}

void VymModel::emitCollapseOneLevel()	
{
    emit (collapseOneLevel () );
}

void VymModel::emitCollapseUnselected()	
{
    emit (collapseUnselected() );
}

void VymModel::toggleTarget()	
{
    BranchItem *selbi=getSelectedBranch();
    if (selbi)
    {
	selbi->toggleTarget(); 
	reposition();
	saveState ( 
	    selbi,
	    "toggleTarget()",
	    selbi,
	    "toggleTarget",
	    "Toggle target");
    }
}

ItemList VymModel::getTargets()	
{
    ItemList targets;
    
    //rmodel->setSearchString (s);

    BranchItem *cur=NULL;
    BranchItem *prev=NULL;
    nextBranch(cur,prev);

    while (cur) 
    {
	if (cur->hasActiveSystemFlag("system-target"))
	    targets[cur->getID()]=cur->getHeading();
	nextBranch(cur,prev);
    }
    return targets; 
}

void VymModel::toggleStandardFlag (const QString &name, FlagRow *master)
{
    BranchItem *bi=getSelectedBranch();
    if (bi) 
    {
	QString u,r;
	if (bi->hasActiveStandardFlag(name))
	{
	    r="unsetFlag";
	    u="setFlag";
	}   
	else
	{
	    u="unsetFlag";
	    r="setFlag";
	}   
	saveState(
	    bi,
	    QString("%1 (\"%2\")").arg(u).arg(name), 
	    bi,
	    QString("%1 (\"%2\")").arg(r).arg(name),
	    QString("Toggling standard flag \"%1\" of %2").arg(name).arg(getObjectName(bi)));
	    bi->toggleStandardFlag (name, master);
	emitDataChanged (bi);
	reposition();
    }
}

void VymModel::addFloatImage (const QImage &img) 
{
    BranchItem *selbi=getSelectedBranch();
    if (selbi)
    {
	ImageItem *ii=createImage (selbi);
	ii->load(img);
	ii->setOriginalFilename("No original filename (image added by dropevent)"); 
	QString s=getSelectString(selbi);
	saveState (PartOfMap, s, "nop ()", s, "copy ()","Copy dropped image to clipboard",ii  );
	saveState (ii,"delete ()", selbi,QString("paste(%1)").arg(curStep),"Pasting dropped image");
	reposition();
    }
}


void VymModel::colorBranch (QColor c)	
{
    QList <BranchItem*> selbis=getSelectedBranches();
    foreach (BranchItem* selbi, selbis)
    {
	saveState(
	    selbi, 
	    QString ("colorBranch (\"%1\")").arg(selbi->getHeadingColor().name()),
	    selbi,
	    QString ("colorBranch (\"%1\")").arg(c.name()),
	    QString("Set color of %1 to %2").arg(getObjectName(selbi)).arg(c.name())
	);  
	selbi->setHeadingColor(c); // color branch
	emitDataChanged (selbi);
	taskEditor->showSelection();
    }
    mapEditor->getScene()->update();    
}

void VymModel::colorSubtree (QColor c, BranchItem *b) 
{
    QList <BranchItem*> selbis;
    if (b) 
	selbis.append (b);
    else
	selbis=getSelectedBranches();
    foreach (BranchItem *bi,selbis)
    {
	saveStateChangingPart(
	    bi,
	    bi,
	    QString ("colorSubtree (\"%1\")").arg(c.name()),
	    QString ("Set color of %1 and children to %2").arg(getObjectName(bi)).arg(c.name())
	);  
	BranchItem *prev=NULL;
	BranchItem *cur=NULL;
        nextBranch (cur,prev,true,bi);
	while (cur) 
	{
	    cur->setHeadingColor(c); // color links, color children
	    emitDataChanged (cur);
            nextBranch (cur,prev,true,bi);
	}   
    }
    taskEditor->showSelection();
    mapEditor->getScene()->update();
}

QColor VymModel::getCurrentHeadingColor()   
{
    BranchItem *selbi=getSelectedBranch();
    if (selbi)	return selbi->getHeadingColor();
	
    QMessageBox::warning(0,"Warning","Can't get color of heading,\nthere's no branch selected");
    return Qt::black;
}

void VymModel::note2URLs()    
{
    BranchItem *selbi=getSelectedBranch();
    if (selbi)
    {	    
	saveStateChangingPart(
	    selbi,
	    selbi,
	    QString ("note2URLs()"),
	    QString ("Extract URLs from note of %1").arg(getObjectName(selbi))
	);  

	QString n=selbi->getNote();
	if (n.isEmpty()) return;
	QRegExp re ("(http.*)(\\s|\"|')");
	re.setMinimal (true);

	BranchItem *bi;
	int pos = 0;
	while ((pos = re.indexIn(n, pos)) != -1) 
	{
	    bi=createBranch (selbi);
	    bi->setHeading (re.cap(1));
	    bi->setURL (re.cap(1));
	    emitDataChanged (bi);
	    pos += re.matchedLength();
	}
	
    }
}

void VymModel::editHeading2URL() 
{
    TreeItem *selti=getSelectedItem();
    if (selti)
	setURL (selti->getHeading());
}   

void VymModel::editBugzilla2URL()   
{
    TreeItem *selti=getSelectedItem();
    if (selti)
    {	    
	QString h=selti->getHeading();
	QRegExp rx("(\\d+)");
	if (rx.indexIn(h) !=-1)
	    setURL ("https://bugzilla.novell.com/show_bug.cgi?id="+rx.cap(1) );
    }
}   

void VymModel::getBugzillaData(bool subtree)	
{
    if (!bugzillaClientAvailable)
    {
	WarningDialog dia;
	dia.setText(
	    QObject::tr("No Bugzilla client found. "
	    " For openSUSE you can install by (running as root):\n\n","VymModel, how to install Bugzilla client module")+
	    "  zypper ar http://download.opensuse.org/repositories/openSUSE:/Tools/openSUSE_XX.Y/ openSUSE:Tools_XX.Y\n"+
	    "  zypper in perl-SUSE-BugzillaClient\n\n"+
	    "  and replace XX.Y with your version of openSUSE, e.g. 11.4\n\n"+
	    QObject::tr("Alternatively you can also add the repository\n"
	    "and install the perl module for Bugzilla access using YaST","VymModel, how to install Bugzilla client module")
	);
	dia.setWindowTitle(QObject::tr("Warning: Couldn't find Bugzilla client","VymModel"));
	dia.setShowAgainName("/BugzillaClient/missing");
	dia.exec();
	return;
    }
    
    BranchItem *selbi=getSelectedBranch();
    if (selbi)
    {	    
	QString url;
	BranchItem *prev=NULL;
	BranchItem *cur=NULL;
        nextBranch (cur,prev,true,selbi);
	while (cur) 
	{
	    url=cur->getURL();
	    if (!url.isEmpty())
	    {
		// Don't run query again if we are in update mode
		if (!subtree || ! url.contains("buglist.cgi") )
		{
		    new BugAgent (cur,url);
		    mainWindow->statusMessage (tr("Contacting Bugzilla...","VymModel"));
		}
	    }
	    if (subtree) 
		nextBranch (cur,prev,true,selbi);
	    else
		cur=NULL;
	}   
    }
}   

void VymModel::editFATE2URL()
{
    TreeItem *selti=getSelectedItem();
    if (selti)
    {	    
	QString url= "http://keeper.suse.de:8080/webfate/match/id?value=ID"+selti->getHeading();
	saveState(
	    selti,
	    "setURL (\""+selti->getURL()+"\")",
	    selti,
	    "setURL (\""+url+"\")",
	    QString("Use heading of %1 as link to FATE").arg(getObjectName(selti))
	);  
	selti->setURL (url);
	// FIXME-4 updateActions();
    }
}   

void VymModel::setVymLink (const QString &s)	//FIXME-4 fail, if s does not exist
{
    BranchItem *bi=getSelectedBranch();
    if (bi)
    {
	saveState(
	    bi,
	    "setVymLink (\""+bi->getVymLink()+"\")", 
	    bi,
	    "setVymLink (\""+s+"\")", 
	    QString("Set vymlink of %1 to %2").arg(getObjectName(bi)).arg(s)
	);  
	bi->setVymLink(s);
	emitDataChanged (bi);
	reposition();
    }
}

void VymModel::deleteVymLink()
{
    BranchItem *bi=getSelectedBranch();
    if (bi)
    {	    
	saveState(
	    bi,
	    "setVymLink (\""+bi->getVymLink()+"\")", 
	    bi,
	    "setVymLink (\"\")",
	    QString("Unset vymlink of %1").arg(getObjectName(bi))
	);  
	bi->setVymLink ("");
	emitDataChanged (bi);
	reposition();
	updateActions();
    }
}

QString VymModel::getVymLink()
{
    BranchItem *bi=getSelectedBranch();
    if (bi)
	return bi->getVymLink();
    else    
	return "";
    
}

QStringList VymModel::getVymLinks() 
{
    QStringList links;
    BranchItem *selbi=getSelectedBranch();
    BranchItem *cur=NULL;
    BranchItem *prev=NULL;
    nextBranch (cur,prev,true,selbi);
    while (cur) 
    {
	if (!cur->getVymLink().isEmpty()) links.append( cur->getVymLink());
	nextBranch (cur,prev,true,selbi);
    }	
    return links;
}


void VymModel::followXLink(int i)   
{
    BranchItem *selbi=getSelectedBranch();
    if (selbi)
    {
	selbi=selbi->getXLinkItemNum(i)->getPartnerBranch();
	if (selbi) select (selbi);
    }
}

void VymModel::editXLink()
{
    Link *l=getSelectedXLink();
    if (l) 
    {
	EditXLinkDialog dia;
	dia.setLink (l);
	if (dia.exec() == QDialog::Accepted)
	{
	    if (dia.useSettingsGlobal() )
            {
		setMapDefXLinkPen( l->getPen() );
                // FIXME-0 set defxlink begin style
                // FIXME-0 set defxlink end   style
                // and also read/write in map header
            }
	}
    }   
}

//////////////////////////////////////////////
// Scripting
//////////////////////////////////////////////

QVariant VymModel::parseAtom(const QString &atom, bool &noErr, QString &errorMsg)
{
    TreeItem* selti=getSelectedItem();
    BranchItem *selbi=getSelectedBranch();
    QString s,t;
    double x,y;
    int n;
    bool b,ok;
    QVariant returnValue="";

    // Split string s into command and parameters
    parser.parseAtom (atom);

    // Check set of parameters
    if (parser.errorLevel()==NoError && parser.checkParameters(selti) )
    {
	QString com=parser.getCommand();
	/////////////////////////////////////////////////////////////////////
	if (com=="addBranch")  
	{
	    if (parser.parCount()==0)
		addNewBranch ();
	    else
		addNewBranch ( selbi,parser.parInt (ok,0) );
	/////////////////////////////////////////////////////////////////////
	} else if (com=="addBranchBefore")
	{
	    addNewBranchBefore ();
	/////////////////////////////////////////////////////////////////////
	} else if (com==QString("addMapCenter"))
	{
	    x=parser.parDouble (ok,0);
	    y=parser.parDouble (ok,1);
	/////////////////////////////////////////////////////////////////////
	} else if (com==QString("addMapInsert"))
	{
	    t=parser.parString (ok,0);  // path to map
	    int contentFilter=0x0000;

	    int pc=parser.parCount();
	    int pos=-1;	
	    // Get position
	    if (pc>1)
	    {
		pos=parser.parInt(ok,1);	    // position
		if (!ok)
		    parser.setError (Aborted,"Couldn't read position");
	    }

	    // Get contentFilter (to filter e.g. slides)
	    if (pc>2)
	    {
		contentFilter=parser.parInt (ok,2);
		if (!ok)
		    parser.setError (Aborted,"Couldn't read content Filter");
	    }
	    
	    if (parser.errorLevel() == NoError)
	    {
		if (QDir::isRelativePath(t)) 
		    t=(QDir::currentPath() + "/"+t);
		saveStateBeforeLoad (ImportAdd, t);
		if (File::Aborted==loadMap (t,ImportAdd,VymMap,contentFilter,pos) )
		    parser.setError (Aborted,QString("Couldn't load %1").arg(t) );
	    }	
	/////////////////////////////////////////////////////////////////////
	} else if (com==QString("addMapReplace"))
	{
	    t=parser.parString (ok,0);	// path to map
	    if (QDir::isRelativePath(t)) 
		t=(QDir::currentPath() + "/"+t);
	    saveStateBeforeLoad (ImportReplace, t);
	    if (File::Aborted==loadMap (t,ImportReplace,VymMap) )
		parser.setError (Aborted,QString("Couldn't load %1").arg(t) );
	/////////////////////////////////////////////////////////////////////
	} else if (com==QString("addSlide"))
	{
	    addSlide();
	/////////////////////////////////////////////////////////////////////
	} else if (com==QString("addXLink")) 
	{
	    s=parser.parString (ok,0);	// begin
	    t=parser.parString (ok,1);	// end
	    BranchItem *begin=(BranchItem*)findBySelectString(s);
	    BranchItem *end=(BranchItem*)findBySelectString(t);
	    if (begin && end)
	    {
		if (begin->isBranchLikeType() && end->isBranchLikeType())
		{
		    Link *li=new Link (this);
		    li->setBeginBranch ( (BranchItem*)begin );
		    li->setEndBranch ( (BranchItem*)end);

		    createLink (li);
		    QPen pen=li->getPen();
		    if (parser.parCount()>2)
		    {
			int w=parser.parInt (ok,2); 
			if (ok) pen.setWidth(w);
		    }
		    if (parser.parCount()>3)
		    {
			QColor col=parser.parColor (ok,3);
			if (ok) pen.setColor (col);
		    }
		    if (parser.parCount()>4)
		    {
			QString st0=parser.parString (ok,4);
			if (ok)
			{
			    Qt::PenStyle st1=penStyle (st0,ok);
			    if (ok) 
				pen.setStyle (st1);
			    else	
				parser.setError (Aborted, "Couldn't read penstyle");
			}
		    }
		    if (ok) li->setPen(pen);	
		}
		else
		    parser.setError (Aborted,"begin or end of xLink are not branch or mapcenter");
		
	    } else
		parser.setError (Aborted,"Couldn't find begin or end of xLink");
	/////////////////////////////////////////////////////////////////////
	} else if (com=="branchCount")
	{ 
	    returnValue=selti->branchCount();
	/////////////////////////////////////////////////////////////////////
	} else if (com=="centerCount")
	{ 
	    returnValue=rootItem->branchCount();
	/////////////////////////////////////////////////////////////////////
	} else if (com=="centerOnID")
	{
	    s=parser.parString(ok,0);
	    TreeItem *ti=findUuid(QUuid(s));
	    if (ti)
            {
		LinkableMapObj *lmo=((MapItem*)ti)->getLMO();
		if (zoomFactor>0 && lmo)
		    mapEditor->setViewCenterTarget (
			lmo->getBBox().center(),
			zoomFactor,
			rotationAngle,
			animDuration,
			animCurve);
                else
                    qWarning()<<"VymModel::centerOnID failed!";
	    } else
		parser.setError(Aborted,QString("Could not find ID: \"%1\"").arg(s));
	/////////////////////////////////////////////////////////////////////
	} else if (com=="clearFlags")   
	{
	    selbi->deactivateAllStandardFlags();
	    reposition();
	    emitDataChanged(selbi);
	    setChanged();
	/////////////////////////////////////////////////////////////////////
	} else if (com=="colorBranch")
	{
	    QColor c=parser.parColor (ok,0);
	    colorBranch (c);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="colorSubtree")
	{
	    QColor c=parser.parColor (ok,0);
	    colorSubtree (c);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="copy")
	{
	    copy();
	/////////////////////////////////////////////////////////////////////
	} else if (com=="cut")
	{
		cut();
	/////////////////////////////////////////////////////////////////////
	} else if (com=="cycleTask")
	{
	    ok=true;
	    if (parser.parCount()==0) b=false;
	    if (parser.parCount()==1) b=parser.parBool(ok,0);
	    if (ok) cycleTaskStatus (b);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="delete")
	{
	    deleteSelection();
	/////////////////////////////////////////////////////////////////////
	} else if (com=="deleteKeepChildren")
	{
	    deleteKeepChildren();
	/////////////////////////////////////////////////////////////////////
	} else if (com=="deleteChildren")
	{
	    deleteChildren();
	/////////////////////////////////////////////////////////////////////
	} else if (com=="deleteSlide")   
	{
	    if (ok) deleteSlide(n);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="exportAO")
	{
	    QString fname=parser.parString(ok,0); 
	    exportAO (fname,false);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="exportASCII")
	{
	   QString fname=parser.parString(ok,0); 
	   exportASCII (fname,false);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="exportCSV")
	{
	   QString fname=parser.parString(ok,0); 
	   exportCSV (fname,false);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="exportHTML")
	{
	    QString path=parser.parString(ok,0); 
	    QString fname=parser.parString(ok,1); 
	    exportHTML (path,fname,false);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="exportImage")
	{
	    QString fname=parser.parString(ok,0); 
	    QString format="PNG";
	    if (parser.parCount()>=2)
		format=parser.parString(ok,1);
	    exportImage (fname,false,format);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="exportImpress")
	{
	    QString fn=parser.parString(ok,0); 
	    QString cf=parser.parString(ok,1); 
	    exportImpress (fn,cf);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="exportLast")
	{
	    exportLast ();
	/////////////////////////////////////////////////////////////////////
	} else if (com=="exportLaTeX")
	{
	    QString fname=parser.parString(ok,0); 
	    exportLaTeX (fname,false);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="exportOrgMode")
	{
	    QString fname=parser.parString(ok,0); 
	    exportOrgMode (fname,false);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="exportPDF")
	{
	    QString fname=parser.parString(ok,0); 
	    exportPDF(fname,false);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="exportSVG")
	{
	    QString fname=parser.parString(ok,0); 
	    exportSVG(fname,false);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="exportXML")
	{
	    QString dpath=parser.parString(ok,0); 
	    QString fpath=parser.parString(ok,1); 
	    exportXML (dpath,fpath,false);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="getDestPath")
	{ 
	    returnValue=getDestPath();
	/////////////////////////////////////////////////////////////////////
	} else if (com=="getFileDir")
	{ 
	    returnValue=getFileDir();
	/////////////////////////////////////////////////////////////////////
	} else if (com=="getFrameType")
	{ 
	    BranchObj *bo=(BranchObj*)(selbi->getLMO());
	    if (!bo)
		parser.setError (Aborted,"No BranchObj");
	    else
		returnValue=bo->getFrame()->getFrameTypeName();
	/////////////////////////////////////////////////////////////////////
	} else if (com=="getHeading")
	{ 
	    returnValue=getHeading();
	/////////////////////////////////////////////////////////////////////
	} else if (com=="getMapAuthor")
	{ 
	    returnValue=author;
	/////////////////////////////////////////////////////////////////////
	} else if (com=="getMapComment")
	{ 
	    returnValue=comment;
	/////////////////////////////////////////////////////////////////////
	} else if (com=="getMapTitle")
	{ 
	    returnValue=title;
	/////////////////////////////////////////////////////////////////////
	} else if (com=="getNote")
	{ 
	    returnValue=getNote();
	/////////////////////////////////////////////////////////////////////
	} else if (com=="getSelectString")
	{ 
	    returnValue=getSelectString();
	/////////////////////////////////////////////////////////////////////
	} else if (com=="getTaskSleepDays")
	{ 
      Task *task=selbi->getTask();
      if (task)
        returnValue=task->getDaysSleep();
      else
        parser.setError (Aborted,"Branch has no task set");
	/////////////////////////////////////////////////////////////////////
	} else if (com=="getURL")
	{ 
	    returnValue=selti->getURL();
	/////////////////////////////////////////////////////////////////////
	} else if (com=="getVymLink")
	{ 
	    returnValue=selti->getVymLink();
	/////////////////////////////////////////////////////////////////////
	} else if (com=="getXLinkColor")
	{ 
	    returnValue=getXLinkColor().name();
	/////////////////////////////////////////////////////////////////////
	} else if (com=="getXLinkWidth")
	{ 
	    returnValue=getXLinkWidth();
	/////////////////////////////////////////////////////////////////////
	} else if (com=="getXLinkPenStyle")
	{ 
	    returnValue=penStyleToString( getXLinkPenStyle() );
	/////////////////////////////////////////////////////////////////////
	} else if (com=="hasActiveFlag")
	{ 
	    s=parser.parString(ok,0);
	    returnValue=selti->hasActiveStandardFlag(s);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="hasTask")
	{ 
      if (selbi && selbi->getTask() )
        returnValue=true;
      else
        returnValue=false;
	/////////////////////////////////////////////////////////////////////
	} else if (com=="importDir")
	{
	    s=parser.parString(ok,0);
	    importDirInt(s);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="isScrolled")
	{
	    returnValue=selbi->isScrolled();
	/////////////////////////////////////////////////////////////////////
	} else if (com=="loadImage")
	{
	    s=parser.parString(ok,0);
	    loadImage (selbi,s);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="loadNote")
	{
	    s=parser.parString(ok,0);
	    loadNote (s);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="moveDown")
	{
	    moveDown();
	/////////////////////////////////////////////////////////////////////
	} else if (com=="moveUp")
	{
	    moveUp();
	/////////////////////////////////////////////////////////////////////
	} else if (com=="moveSlideUp")
	{
	    n=parser.parInt (ok,0);
	    if (n>=slideModel->count())
		parser.setError (Aborted,"Index out of range");
	    else    
		moveSlideUp(n);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="moveSlideDown")
	{
	    n=parser.parInt (ok,0);
	    if (n>=slideModel->count()-1)
		parser.setError (Aborted,"Index out of range");
	    else    
		moveSlideDown(n);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="move")
	{
	    x=parser.parDouble (ok,0);
	    y=parser.parDouble (ok,1);
	    move (x,y);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="moveRel")
	{
	    x=parser.parDouble (ok,0);
	    y=parser.parDouble (ok,1);
	    moveRel (x,y);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="nop")
	{
	/////////////////////////////////////////////////////////////////////
	} else if (com=="note2URLs")
	{
	    note2URLs();
	/////////////////////////////////////////////////////////////////////
	} else if (com=="paste")
	{
	    paste();
	/////////////////////////////////////////////////////////////////////
	} else if (com=="redo")
	{
	    redo();
	/////////////////////////////////////////////////////////////////////
	} else if (com=="relinkTo")
	{
	    if (!selti)
	    {
		parser.setError (Aborted,"Nothing selected");
	    } else if ( selbi)
	    {
		if (parser.checkParCount(4))
		{
		    // 0	selectstring of parent
		    // 1	num in parent (for branches)
		    // 2,3	x,y of mainbranch or mapcenter (for images)
		    s=parser.parString(ok,0);
		    TreeItem *dst=findBySelectString (s);
		    if (dst)
		    {   
			if (dst->getType()==TreeItem::Branch ) 
			{
			    // Get number in parent
			    n=parser.parInt (ok,1);
			    if (ok)
			    {
				if (relinkBranch (selbi,(BranchItem*)dst,n,true))
				    emitSelectionChanged();
				else
				    parser.setError(Aborted,"Relinking failed");
			    }   
			} else if (dst->getType()==TreeItem::MapCenter) 
			{
			    if (relinkBranch (selbi,(BranchItem*)dst,-1,true))
			    {
				// Get coordinates of mainbranch
				x=parser.parDouble(ok,2);
				if (ok)
				{
				    y=parser.parDouble(ok,3);
				    if (ok) 
				    {
					if (selbi->getLMO()) 
					{
					    ((BranchObj*)selbi->getLMO())->move (x,y);
					    ((BranchObj*)selbi->getLMO())->setRelPos();
					}
				    }
				}
				reposition();
				emitSelectionChanged();
			    } else
				parser.setError(Aborted,"Relinking failed");
			}	
		    } else
			parser.setError (Aborted,"Couldn't find destination branch");
		}	
	    } else if ( selti->getType() == TreeItem::Image) 
	    {
		if (parser.checkParCount(1))
		{
		    // 0	selectstring of parent
		    s=parser.parString(ok,0);
		    TreeItem *dst=findBySelectString (s);
		    if (dst)
		    {   
			if (dst->isBranchLikeType())
			    if (!relinkImage ( ((ImageItem*)selti),(BranchItem*)dst))
				parser.setError(Aborted,"Relinking failed");

		    } else	
			parser.setError (Aborted,"Destination is not a branch");
		}	    
	    } else
		parser.setError (Aborted,"Type of selection is not a floatimage or branch");
	/////////////////////////////////////////////////////////////////////
	} else if (com=="saveImage")
	{
	    ImageItem *ii=getSelectedImage();
	    s=parser.parString(ok,0);
	    t=parser.parString(ok,1);
	    saveImage (ii,t,s);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="saveNote")
	{
	    s=parser.parString(ok,0);
	    saveNote (s);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="scroll")
	{
	    if (!scrollBranch (selbi))	
		parser.setError (Aborted,"Could not scroll branch");
	/////////////////////////////////////////////////////////////////////
	} else if (com=="select")
	{
	    s=parser.parString(ok,0);
	    if (!select (s))
		parser.setError(Aborted,QString("Could not select \"%1\"").arg(s));
	/////////////////////////////////////////////////////////////////////
	} else if (com=="selectID")
	{
	    s=parser.parString(ok,0);
	    if (!selectID (s))
		parser.setError(Aborted,QString("Could not select ID: \"%1\"").arg(s));
	/////////////////////////////////////////////////////////////////////
	} else if (com=="selectLastBranch")
	{
	    BranchItem *bi=selbi->getLastBranch();
	    if (!bi)
		parser.setError (Aborted,"Could not select last branch");
	    select (bi);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="selectLastImage")
	{
	    ImageItem *ii=selbi->getLastImage();
	    if (!ii)
		parser.setError (Aborted,"Could not select last image");
	    select (ii);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="selectParent")
	{
	    selectParent ();
	/////////////////////////////////////////////////////////////////////
	} else if (com=="selectLatestAdded")
	{
	    if (!latestAddedItem)
	    {
		parser.setError (Aborted,"No latest added object");
	    } else
	    {   
		if (!select (latestAddedItem))
		    parser.setError (Aborted,"Could not select latest added object ");
	    }   
	/////////////////////////////////////////////////////////////////////
	} else if (com=="setFlag")
	{
	    s=parser.parString(ok,0);
	    selbi->activateStandardFlag(s);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="setTaskSleep")
	{
	    s=parser.parString(ok,0);
	    returnValue=setTaskSleep (s);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="setFrameIncludeChildren")
	{
	    b=parser.parBool(ok,0);
	    setFrameIncludeChildren(b);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="setFrameType")
	{
	    s=parser.parString(ok,0);
	    setFrameType (s);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="setFramePenColor")
	{
	    QColor c=parser.parColor(ok,0);
	    setFramePenColor (c);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="setFrameBrushColor")
	{
	    QColor c=parser.parColor(ok,0);
	    setFrameBrushColor (c);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="setFramePadding")
	{
	    n=parser.parInt(ok,0);
	    setFramePadding(n);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="setFrameBorderWidth")
	{
	    n=parser.parInt(ok,0);
	    setFrameBorderWidth (n);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="setHeading")
	{
	    s=parser.parString (ok,0);
	    setHeading (s);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="setHideExport")
	{
	    b=parser.parBool(ok,0);
	    setHideExport (b);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="setIncludeImagesHorizontally")
	{ 
	    b=parser.parBool(ok,0);
	    setIncludeImagesHor(b);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="setIncludeImagesVertically")
	{
	    b=parser.parBool(ok,0);
	    if (ok) setIncludeImagesVer(b);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="setHideLinkUnselected")
	{
	    b=parser.parBool(ok,0);
	    setHideLinkUnselected(b);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="setMapAnimCurve")
	{
	    n=parser.parInt(ok,0);
	    if (n<0 || n>QEasingCurve::OutInBounce)
		parser.setError (Aborted,"Unknown link style");
	    else
	    {
		QEasingCurve c;
		c.setType ( (QEasingCurve::Type) n);
		setMapAnimCurve(c);
	    }
	/////////////////////////////////////////////////////////////////////
	} else if (com=="setMapAnimDuration")
	{
	    n=parser.parInt(ok,0);
	    setMapAnimDuration(n);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="setMapAuthor")
	{
	    s=parser.parString(ok,0);
	    setAuthor (s);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="setMapComment")
	{
	    s=parser.parString(ok,0);
	    if (ok) setComment(s);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="setMapTitle")
	{
	    s=parser.parString(ok,0);
	    if (ok) setTitle(s);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="setMapBackgroundColor")
	{
	    QColor c=parser.parColor (ok,0);
	    setMapBackgroundColor (c);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="setMapDefLinkColor")
	{
	    QColor c=parser.parColor (ok,0);
	    setMapDefLinkColor (c);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="setMapLinkStyle")
	{
	    s=parser.parString (ok,0);
	    if (!setMapLinkStyle(s) )
		parser.setError (Aborted,"Unknown link style");
	/////////////////////////////////////////////////////////////////////
	} else if (com=="setMapRotation")
	{
	    x=parser.parDouble (ok,0);
	    setMapRotationAngle(x);
	    mapEditor->setAngleTarget(x);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="setMapZoom")
	{
	    x=parser.parDouble (ok,0);
	    setMapZoomFactor(x);
	    mapEditor->setZoomFactorTarget(x);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="setNote")
	{
	    s=parser.parString (ok,0);
	    setNote (s);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="setScale")
	{
	    x=parser.parDouble (ok,0);
	    y=parser.parDouble (ok,1);
	    setScale (x,y);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="setSelectionColor")
	{
	    QColor c=parser.parColor (ok,0);
	    setSelectionColorInt (c);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="setURL")
	{
	    s=parser.parString (ok,0);
	    setURL(s);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="setVymLink")
	{
	    s=parser.parString (ok,0);
	    setVymLink(s);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="sleep")
	{
	    n=parser.parInt (ok,0);
	    sleep (n);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="sortChildren")
	{
	    b=false;
	    if (parser.parCount()==1)
		b=parser.parBool(ok,0);
	    sortChildren(b);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="toggleFlag")
	{
	    s=parser.parString(ok,0);
            toggleStandardFlag (s);
	/////////////////////////////////////////////////////////////////////
	} else if (com=="toggleFrameIncludeChildren")
	{
	    toggleFrameIncludeChildren();
	/////////////////////////////////////////////////////////////////////
	} else if (com=="toggleScroll")
	{
	    toggleScroll();	
	/////////////////////////////////////////////////////////////////////
	} else if (com=="toggleTarget")
	{
	    toggleTarget();	
	/////////////////////////////////////////////////////////////////////
	} else if (com=="toggleTask")
	{
	    toggleTask();	
	/////////////////////////////////////////////////////////////////////
	} else if (com=="undo")
	{
	    undo();
	/////////////////////////////////////////////////////////////////////
	} else  if (com=="unscroll")
	{
	    if (!unscrollBranch (selbi))    
		parser.setError (Aborted,"Could not unscroll branch");
	/////////////////////////////////////////////////////////////////////
	} else if (com=="unscrollChildren")
	{
	    unscrollChildren ();
	/////////////////////////////////////////////////////////////////////
	} else if (com=="unselectAll")
	{
	    unselectAll();
	/////////////////////////////////////////////////////////////////////
	} else if (com=="unsetFlag")
	{
	    s=parser.parString(ok,0);
	    selbi->deactivateStandardFlag(s);
	/////////////////////////////////////////////////////////////////////
	} else 
	    parser.setError (Aborted,"Unknown command");
    } // End of iterating over commands

    // Any errors?
    if (parser.errorLevel()==NoError)
    {
	reposition();
	errorMsg.clear();
	noErr=true;
    }	
    else    
    {
	// TODO Error handling
	noErr=false;
	errorMsg=parser.errorMessage();
	returnValue=errorMsg;
    } 
    return returnValue;
}

QVariant VymModel::execute (const QString &script)
{
    parser.setScript (script);
    parser.execute ();
    QVariant r;
    bool noErr=true;
    QString errMsg;
    while (parser.next() && noErr) 
    {
	r=parseAtom(parser.getAtom(),noErr,errMsg);
	if (!noErr)
	{
	    if (!options.isOn("batch") && !testmode )
		QMessageBox::warning(0,tr("Warning"),tr("Script aborted:\n%1").arg(errMsg));
	    qWarning()<< QString("VM::execute aborted: "+errMsg + "\n" + script);
	}
    }	
    return r;
}

void VymModel::setExportMode (bool b)
{
    // should be called before and after exports
    // depending on the settings
    if (b && settings.value("/export/useHideExport","true")=="true")
	setHideTmpMode (TreeItem::HideExport);
    else    
	setHideTmpMode (TreeItem::HideNone);
}

QPointF VymModel::exportImage(QString fname, bool askName, QString format)  
{
    if (fname=="")
    {
        if (!askName) 
        {
            qWarning("VymModel::exportImage called without filename (and askName==false)");
            return QPointF();
        }

	fname=lastImageDir.absolutePath() + "/" + getMapName()+".png";
	format="PNG";
    }	

    if (askName)
    {
        fname=QFileDialog::getSaveFileName ( 
                mainWindow, 
                tr("Export map as image"),
                fname,
                imageIO.getFilters().join(";;")
                );
        lastImageDir=dirname(fname);
    }

    if (fname.isEmpty()) return QPointF();

    setExportMode (true);
    QPointF offset;
    QImage img (mapEditor->getImage(offset));
    if (!img.save(fname, format.toAscii()))
	QMessageBox::critical (0,tr("Critical Error"),tr("Couldn't save QImage %1 in format %2").arg(fname).arg(format));
    setExportMode (false);

    QString cmd= QString("exportImage(\"%1\",\"PNG\")").arg(fname);
    settings.setLocalValue ( filePath, "/export/last/exportPath",fname);
    settings.setLocalValue ( filePath, "/export/last/command",cmd);
    settings.setLocalValue ( filePath, "/export/last/description","Image");
    setChanged();
    mainWindow->statusMessage(tr("Exported: ","Export confirmation") + fname);

    return offset;
}

QPointF VymModel::exportPDF (QString fname, bool askName)
{
    if (fname=="")
    {
        if (!askName) 
        {
            qWarning("VymModel::exportPDF called without filename (and askName==false)");
            return QPointF();
        }

	fname=lastImageDir.absolutePath() + "/" + getMapName()+".pdf";
    }	

    if (askName)
    {
        fname=QFileDialog::getSaveFileName ( 
                mainWindow, 
                tr("Export map as PDF"),
                fname,
                "PDF (*.pdf);;All (* *.*)"
                );
        lastImageDir=dirname(fname);
    }

    if (fname.isEmpty()) return QPointF();

    setExportMode (true);
    QPointF offset;

    // To PDF
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fname);
    printer.setPageSize(QPrinter::A3);

    QRectF bbox=mapEditor->getTotalBBox();
    if (bbox.width()>bbox.height())
	// recommend landscape
	printer.setOrientation (QPrinter::Landscape);
    else    
	// recommend portrait
	printer.setOrientation (QPrinter::Portrait);

    QPainter *pdfPainter = new QPainter(&printer);
    getScene()->render(pdfPainter);
    pdfPainter->end();
    delete pdfPainter;

    setExportMode (false);

    QString cmd= QString("exportPDF(\"%1\")").arg(fname);
    settings.setLocalValue ( filePath, "/export/last/exportPath",fname);
    settings.setLocalValue ( filePath, "/export/last/command",cmd);
    settings.setLocalValue ( filePath, "/export/last/description","PDF");
    setChanged();
    mainWindow->statusMessage(tr("Exported: ","Export confirmation") + fname);

    return offset;
}

QPointF VymModel::exportSVG (QString fname, bool askName) 
{
    if (fname=="")
    {
        if (!askName) 
        {
            qWarning("VymModel::exportSVG called without filename (and askName==false)");
            return QPointF();
        }

	fname=lastImageDir.absolutePath() + "/" + getMapName()+".png";
    }	

    if (askName)
    {
        fname=QFileDialog::getSaveFileName ( 
                mainWindow, 
                tr("Export map as scalable vector graphic"),
                fname,
                "SVG (*.svg);;All (* *.*)"
                );
        lastImageDir=dirname(fname);
    }

    if (fname.isEmpty()) return QPointF();

    setExportMode (true);
    QPointF offset;

    QSvgGenerator generator;
    generator.setFileName(fname);
    QSize sceneSize = getScene()->sceneRect().size().toSize();
    generator.setSize(sceneSize);
    generator.setViewBox(QRect(0, 0, sceneSize.width(), sceneSize.height()));
    QPainter *svgPainter = new QPainter(&generator);
    getScene()->render(svgPainter);
    svgPainter->end();
    delete svgPainter;

    setExportMode (false);

    QString cmd= QString("exportSVG(\"%1\")").arg(fname);
    settings.setLocalValue ( filePath, "/export/last/exportPath",fname);
    settings.setLocalValue ( filePath, "/export/last/command",cmd);
    settings.setLocalValue ( filePath, "/export/last/description","SVG");
    setChanged();
    mainWindow->statusMessage(tr("Exported: ","Export confirmation") + fname);
    return offset;
}

void VymModel::exportXML (QString dpath, QString fpath, bool useDialog)
{
    if (useDialog)
    {
    	QFileDialog fd;
	fd.setWindowTitle (vymName+ " - " + tr("Export XML to directory"));
	fd.setFileMode (QFileDialog::DirectoryOnly);
	QStringList filters;
	filters <<"XML data (*.xml)";
	fd.setFilters (filters);
	fd.setConfirmOverwrite (false);
	fd.setAcceptMode (QFileDialog::AcceptSave);

	QString fn;
	if (fd.exec() != QDialog::Accepted || fd.selectedFiles().isEmpty() )
        {
            qDebug()<<"exportXML returning 1"; //FIXME-2
            return;
        }

	if (dpath=="" && !reallyWriteDirectory(dpath) )
	{
	    qDebug()<<"exportXML returning 2"; //FIXME-2
	    return;
	}
	setChanged();

	dpath=fd.selectedFiles().first();
        dpath=dpath.left(dpath.lastIndexOf("/"));
	fpath=dpath + "/" + mapName + ".xml";
    }

    QString mname=basename(fpath);

    // Hide stuff during export, if settings want this
    setExportMode (true);

    // Create subdirectories
    makeSubDirs (dpath);

    // write image and calculate offset
    QPointF offset=exportImage (dpath + "/images/" + mname + ".png",false,"PNG");

    // write to directory   //FIXME-3 check totalBBox here...
    QString saveFile=saveToDir (dpath , mname + "-", true, offset, NULL); 
    QFile file;

    file.setFileName (fpath);
    if ( !file.open( QIODevice::WriteOnly ) )
    {
	// This should neverever happen
	QMessageBox::critical (
                0,
                tr("Critical Export Error"),
                QString("VymModel::exportXML couldn't open %1").arg(file.fileName())
        );
	return;
    }	

    // Write it finally, and write in UTF8, no matter what 
    QTextStream ts( &file );
    ts << saveFile;
    file.close();

    setExportMode (false);

    QString cmd=QString("exportXML(\"%1\",\"%2\")")
        .arg(dpath)
        .arg(fpath);
    settings.setLocalValue ( filePath, "/export/last/exportPath",dpath);
    settings.setLocalValue ( filePath, "/export/last/command",cmd);
    settings.setLocalValue ( filePath, "/export/last/description","XML");
}

void VymModel::exportAO (QString fname,bool askName)
{
    ExportAO ex;
    ex.setModel (this);
    if (fname=="") 
	ex.setFilePath (mapName+".txt");	
    else
	ex.setFilePath (fname);

    if (askName)
    {
	ex.setDirPath (lastExportDir.absolutePath());
	ex.execDialog();
    } 
    if (!ex.canceled())
    {
	setExportMode(true);
	ex.doExport();
	setExportMode(false);
    }
}

void VymModel::exportASCII(const QString &fname, bool askName)
{
    ExportASCII ex;
    ex.setModel (this);
    if (fname=="") 
	ex.setFilePath (mapName+".txt");	
    else
	ex.setFilePath (fname);

    if (askName) 
    {
	ex.setDirPath (lastExportDir.absolutePath());
        ex.execDialog() ; 
    }

    if (!ex.canceled())
    {
	setExportMode(true);
	ex.doExport();
	setExportMode(false);
    }
}

void VymModel::exportCSV(const QString &fname, bool askName)
{
    ExportCSV ex;
    ex.setModel (this);
    if (fname=="") 
	ex.setFilePath (mapName+".csv");	
    else
	ex.setFilePath (fname);

    if (askName) 
    {
	ex.addFilter ("CSV (*.csb);;All (* *.*)");
	ex.setDirPath (lastExportDir.absolutePath());
	ex.setWindowTitle(vymName+ " -" +tr("Export as csv")+" "+tr("(still experimental)"));
        ex.execDialog() ; 
    }

    if (!ex.canceled())
    {
	setExportMode(true);
	ex.doExport();
	setExportMode(false);
    }
}

void VymModel::exportHTML (const QString &dpath, const QString &fpath,bool useDialog)
{
    ExportHTML ex (this);
    if (!dpath.isEmpty()) ex.setDirPath (dpath);
    if (!fpath.isEmpty()) ex.setFilePath (fpath);
    setExportMode(true);
    ex.doExport(useDialog);
    setExportMode(false);
}

void VymModel::exportImpress(const QString &fn, const QString &cf) 
{
    ExportOO ex;
    ex.setFilePath (fn);
    ex.setModel (this);
    if (ex.setConfigFile(cf)) 
    {
	setExportMode (true);
	ex.exportPresentation();
	setExportMode (false);
    }
}

bool VymModel::exportLastAvailable(QString &description, QString &command, QString &path, QString &configFile)
{
    command=settings.localValue(filePath,"/export/last/command","").toString();
    description=settings.localValue(filePath,"/export/last/description","").toString();
    path=settings.localValue(filePath,"/export/last/exportPath","").toString();
    configFile=settings.localValue(filePath,"/export/last/configFile","").toString();
    if (!command.isEmpty() && command.startsWith("export")) 
	return true;
    else
	return false;
}   

void VymModel::exportLast()
{
    QString desc, command, path, configFile;  //FIXME-2 better integrate configFIle into command
    if (exportLastAvailable(desc, command, path, configFile) )
    {
        execute (command);
        /*
	if (!configFile.isEmpty() && command=="exportImpress")
	    execute (QString ("%1 (\"%2\",\"%3\")").arg(command).arg(path).arg(configFile) );
	else    
	    execute (QString ("%1 (\"%2\")").arg(command).arg(path) );
        */
    }	    
}

void VymModel::exportLaTeX (const QString &fname,bool askName)
{
    ExportLaTeX ex;
    ex.setModel (this);
    if (fname=="") 
	ex.setFilePath (mapName+".tex");	
    else
	ex.setFilePath (fname);

    if (askName) ex.execDialog() ; 
    if (!ex.canceled())
    {
	setExportMode(true);
	ex.doExport();
	setExportMode(false);
    }
}

void VymModel::exportOrgMode (const QString &fname, bool askName)
{
    ExportOrgMode ex;
    ex.setModel (this);
    if (fname=="") 
	ex.setFilePath (mapName+".org");	
    else
	ex.setFilePath (fname);

    if (askName) 
    {
	ex.setDirPath (lastExportDir.absolutePath());
        ex.execDialog();
    }

    if (!ex.canceled())
    {
	setExportMode(true);
	ex.doExport();
	setExportMode(false);
    }
}


//////////////////////////////////////////////
// View related
//////////////////////////////////////////////

void VymModel::registerEditor(QWidget *me)
{
    mapEditor=(MapEditor*)me;
}

void VymModel::unregisterEditor(QWidget *)
{
    mapEditor=NULL;
}

void VymModel::setMapZoomFactor (const double &d)
{
    zoomFactor=d;
}

void VymModel::setMapRotationAngle(const double &d)
{
    rotationAngle=d;
}

void VymModel::setMapAnimDuration(const int &d)
{
    animDuration=d;
}

void VymModel::setMapAnimCurve(const QEasingCurve &c)
{
    animCurve=c;
}

void VymModel::setContextPos(QPointF p)
{
    contextPos=p;
    hasContextPos=true;
}

void VymModel::unsetContextPos()
{
    contextPos=QPointF();
    hasContextPos=false;
}

void VymModel::updateNoteFlag()
{
    TreeItem *selti=getSelectedItem();
    if (selti)
    {
	if (!mapChanged)
	{
	    setChanged();
	    updateActions();
	}

	if (noteEditor->isEmpty()) 
	    selti->clearNote();
	else
	    selti->setNoteObj (noteEditor->getNote());
	emitDataChanged(selti);	
        reposition();
    }
}

void VymModel::reposition() //FIXME-4 VM should have no need to reposition, but the views...
{
    if (blockReposition) return;

    BranchObj *bo;
    for (int i=0;i<rootItem->branchCount(); i++)
    {
	bo=rootItem->getBranchObjNum(i);
	if (bo)
	    bo->reposition();	//  for positioning heading
	else
	    qDebug()<<"VM::reposition bo=0";
    }	
    mapEditor->getTotalBBox();	
    emitSelectionChanged();
}


bool VymModel::setMapLinkStyle (const QString & s)
{
    QString snow;
    switch (linkstyle)
    {
	case LinkableMapObj::Line :
	    snow="StyleLine";
	    break;
	case LinkableMapObj::Parabel:
	    snow="StyleParabel";
	    break;
	case LinkableMapObj::PolyLine:
	    snow="StylePolyLine";
	    break;
	case LinkableMapObj::PolyParabel:
	    snow="StylePolyParabel";
	    break;
	default:    
	    return false;
	    break;
    }

    saveState (
	QString("setMapLinkStyle (\"%1\")").arg(s),
	QString("setMapLinkStyle (\"%1\")").arg(snow),
	QString("Set map link style (\"%1\")").arg(s)
    );	

    if (s=="StyleLine")
	linkstyle=LinkableMapObj::Line;
    else if (s=="StyleParabel")
	linkstyle=LinkableMapObj::Parabel;
    else if (s=="StylePolyLine")
	linkstyle=LinkableMapObj::PolyLine;
    else if (s=="StylePolyParabel") 
	linkstyle=LinkableMapObj::PolyParabel;
    else
	linkstyle=LinkableMapObj::UndefinedStyle;

    BranchItem *cur=NULL;
    BranchItem *prev=NULL;
    BranchObj *bo;
    nextBranch (cur,prev);
    while (cur) 
    {
	bo=(BranchObj*)(cur->getLMO() );
	bo->setLinkStyle(bo->getDefLinkStyle(cur->parent() ));	//FIXME-4 better emit dataCHanged and leave the changes to View
	nextBranch(cur,prev);
    }
    reposition();
    return true;
}

LinkableMapObj::Style VymModel::getMapLinkStyle ()
{
    return linkstyle;
}   

uint VymModel::getModelID()
{
    return modelID;
}

void VymModel::setView (VymView *vv)
{
    vymView=vv;
}

void VymModel::setMapDefLinkColor(QColor col)
{
    if ( !col.isValid() ) return;
    saveState (
	QString("setMapDefLinkColor (\"%1\")").arg(getMapDefLinkColor().name()),
	QString("setMapDefLinkColor (\"%1\")").arg(col.name()),
	QString("Set map link color to %1").arg(col.name())
    );

    defLinkColor=col;
    BranchItem *cur=NULL;
    BranchItem *prev=NULL;
    BranchObj *bo;
    nextBranch(cur,prev);
    while (cur) 
    {
	bo=(BranchObj*)(cur->getLMO() );
	bo->setLinkColor();
	nextBranch(cur,prev);
    }
    updateActions();
}

void VymModel::setMapLinkColorHintInt()
{
    // called from setMapLinkColorHint(lch) or at end of parse
    BranchItem *cur=NULL;
    BranchItem *prev=NULL;
    BranchObj *bo;
    nextBranch(cur,prev);
    while (cur) 
    {
	bo=(BranchObj*)(cur->getLMO() );
	bo->setLinkColor();
	nextBranch(cur,prev);
    }
}

void VymModel::setMapLinkColorHint(LinkableMapObj::ColorHint lch)
{
    linkcolorhint=lch;
    setMapLinkColorHintInt();
}

void VymModel::toggleMapLinkColorHint()
{
    if (linkcolorhint==LinkableMapObj::HeadingColor)
	linkcolorhint=LinkableMapObj::DefaultColor;
    else    
	linkcolorhint=LinkableMapObj::HeadingColor;
    BranchItem *cur=NULL;
    BranchItem *prev=NULL;
    BranchObj *bo;
    nextBranch(cur,prev);
    while (cur) 
    {
	bo=(BranchObj*)(cur->getLMO() );
	bo->setLinkColor();
	nextBranch(cur,prev);
    }
}

void VymModel::selectMapBackgroundImage ()  // FIXME-5 for using background image: view.setCacheMode(QGraphicsView::CacheBackground);  Also this belongs into ME
{
    QStringList filters;
    filters<< tr("Images") + " (*.png *.bmp *.xbm *.jpg *.png *.xpm *.gif *.pnm)";
    QFileDialog fd;
    fd.setFileMode (QFileDialog::ExistingFile);
    fd.setWindowTitle(vymName+" - " +tr("Load background image"));
    fd.setDirectory (lastImageDir);
    fd.setAcceptMode (QFileDialog::AcceptOpen);

    if ( fd.exec() == QDialog::Accepted &&!fd.selectedFiles().isEmpty())
    {
	// TODO selectMapBackgroundImg in QT4 use:  lastImageDir=fd.directory();
	lastImageDir=QDir (fd.directory().path());
	setMapBackgroundImage (fd.selectedFiles().first());
    }
}   

void VymModel::setMapBackgroundImage (const QString &fn)    //FIXME-5 missing savestate, move to ME
{
    /*
    QColor oldcol=mapEditor->getScene()->backgroundBrush().color();
    saveState(
	selection,
	QString ("setMapBackgroundImage (%1)").arg(oldcol.name()),
	selection,
	QString ("setMapBackgroundImage (%1)").arg(col.name()),
	QString("Set background color of map to %1").arg(col.name()));
    */	
    QBrush brush;
    brush.setTextureImage (QImage (fn));
    mapEditor->getScene()->setBackgroundBrush(brush);
}

void VymModel::selectMapBackgroundColor()   // FIXME-4 move to ME
{
    QColor col = QColorDialog::getColor( mapEditor->getScene()->backgroundBrush().color(), NULL);
    if ( !col.isValid() ) return;
    setMapBackgroundColor( col );
}


void VymModel::setMapBackgroundColor(QColor col)    // FIXME-4 move to ME
{
    QColor oldcol=mapEditor->getScene()->backgroundBrush().color();
    saveState(
	QString ("setMapBackgroundColor (\"%1\")").arg(oldcol.name()),
	QString ("setMapBackgroundColor (\"%1\")").arg(col.name()),
	QString("Set background color of map to %1").arg(col.name()));
    mapEditor->getScene()->setBackgroundBrush(col);
}

QColor VymModel::getMapBackgroundColor()    // FIXME-4 move to ME
{
    return mapEditor->getScene()->backgroundBrush().color();
}

QFont VymModel::getMapDefaultFont ()  
{
    return defaultFont;
}

void VymModel::setMapDefaultFont (const QFont &f)  
{
    defaultFont=f;
}

LinkableMapObj::ColorHint VymModel::getMapLinkColorHint()   // FIXME-4 move to ME
{
    return linkcolorhint;
}

QColor VymModel::getMapDefLinkColor()	// FIXME-4 move to ME
{
    return defLinkColor;
}

void VymModel::setMapDefXLinkPen (const QPen &p)  // FIXME-4 move to ME
{
    defXLinkPen=p;
}

QPen VymModel::getMapDefXLinkPen()	// FIXME-4 move to ME
{
    return defXLinkPen;
}

void VymModel::setMapDefXLinkStyleBegin( const QString &s)
{
    defXLinkStyleBegin = s;
}

QString VymModel::getMapDefXLinkStyleBegin()
{
    return defXLinkStyleBegin;
}

void VymModel::setMapDefXLinkStyleEnd( const QString &s)
{
    defXLinkStyleEnd = s;
}

QString VymModel::getMapDefXLinkStyleEnd()
{
    return defXLinkStyleEnd;
}

void VymModel::move(const double &x, const double &y)
{
    MapItem *seli = (MapItem*)getSelectedItem();
    if (seli && (seli->isBranchLikeType() || seli->getType()==TreeItem::Image))
    {
	LinkableMapObj *lmo=seli->getLMO();
	if (lmo)
	{
	    QPointF ap(lmo->getAbsPos());
	    QPointF to(x, y);
	    if (ap != to)
	    {
		QString ps=qpointFToString(ap);
		QString s=getSelectString(seli);
		saveState(
		    s, "move "+ps, 
		    s, "move "+qpointFToString(to), 
		    QString("Move %1 to %2").arg(getObjectName(seli)).arg(ps));
		lmo->move(x,y);
		reposition();
		emitSelectionChanged();
	    }
	}
    }
}

void VymModel::moveRel (const double &x, const double &y)   
{
    MapItem *seli = (MapItem*)getSelectedItem();
    if (seli && (seli->isBranchLikeType() || seli->getType()==TreeItem::Image))
    {
	LinkableMapObj *lmo=seli->getLMO();
	if (lmo)
	{
	    QPointF rp(lmo->getRelPos());
	    QPointF to(x, y);
	    if (rp != to)
	    {
		QString ps=qpointFToString (lmo->getRelPos());
		QString s=getSelectString(seli);
		saveState(
		    s, "moveRel "+ps, 
		    s, "moveRel "+qpointFToString(to), 
		    QString("Move %1 to relative position %2").arg(getObjectName(seli)).arg(ps));
		((OrnamentedObj*)lmo)->move2RelPos (x,y);
		reposition();
		lmo->updateLinkGeometry();
		emitSelectionChanged();
	    }
	}   
    }
}


void VymModel::animate()   
{
    animationTimer->stop();
    BranchObj *bo;
    int i=0;
    while (i<animObjList.size() )
    {
	bo=(BranchObj*)animObjList.at(i);
	if (!bo->animate())
	{
	    if (i>=0) 
	    {	
		animObjList.removeAt(i);
		i--;
	    }
	}
	bo->reposition();
	i++;
    } 
    emitSelectionChanged();

    if (!animObjList.isEmpty()) animationTimer->start(animationInterval);
}


void VymModel::startAnimation(BranchObj *bo, const QPointF &v)
{
    if (!bo) return;

    if (bo->getUseRelPos())
	startAnimation (bo,bo->getRelPos(),bo->getRelPos()+v);
    else
	startAnimation (bo,bo->getAbsPos(),bo->getAbsPos()+v);
}

void VymModel::startAnimation(BranchObj *bo, const QPointF &start, const QPointF &dest)
{
    if (start==dest) return;
    if (bo && bo->getTreeItem()->depth()>=0) 
    {
	AnimPoint ap;
	ap.setStart (start);
	ap.setDest  (dest);
	ap.setTicks (animationTicks);
	ap.setAnimated (true);
	bo->setAnimation (ap);
	if (!animObjList.contains(bo))
	    animObjList.append( bo );
	animationTimer->setSingleShot (true);
	animationTimer->start(animationInterval);
    }
}

void VymModel::stopAnimation (MapObj *mo)
{
    int i=animObjList.indexOf(mo);
    if (i>=0)
	animObjList.removeAt (i);
}

void VymModel::stopAllAnimation ()
{
    BranchObj *bo;
    int i=0;
    while (i<animObjList.size() )
    {
	bo=(BranchObj*)animObjList.at(i);
	bo->stopAnimation();
	bo->requestReposition();
	i++;
    } 
    reposition();
}

void VymModel::sendSelection()
{
    if (netstate!=Server) return;
    sendData (QString("select (\"%1\")").arg(getSelectString()) );
}

void VymModel::newServer()
{
    port=54321;
    sendCounter=0;
    tcpServer = new QTcpServer(this);
    if (!tcpServer->listen(QHostAddress::Any,port)) {
        QMessageBox::critical(NULL, "vym server",
                              QString("Unable to start the server: %1.").arg(tcpServer->errorString()));
        //FIXME-3 needed? we are no widget any longer... close();
        return;
    }
    connect(tcpServer, SIGNAL(newConnection()), this, SLOT(newClient()));
    netstate=Server;
    qDebug()<<"Server is running on port "<<tcpServer->serverPort();
}

void VymModel::connectToServer()
{
    port=54321;
    server="salam.suse.de";
    server="localhost";
    clientSocket = new QTcpSocket (this);
    clientSocket->abort();
    clientSocket->connectToHost(server ,port);
    connect(clientSocket, SIGNAL(readyRead()), this, SLOT(readData()));
    connect(clientSocket, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(displayNetworkError(QAbstractSocket::SocketError)));
    netstate=Client;	    
    qDebug()<<"connected to "<<qPrintable (server)<<" port "<<port;

    
}

void VymModel::newClient()
{
    QTcpSocket *newClient = tcpServer->nextPendingConnection();
    connect(newClient, SIGNAL(disconnected()),
            newClient, SLOT(deleteLater()));

    qDebug() <<"ME::newClient  at "<<qPrintable( newClient->peerAddress().toString() );

    clientList.append (newClient);
}


void VymModel::sendData(const QString &s)
{
    if (clientList.size()==0) return;

    // Create bytearray to send
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_0);

    // Reserve some space for blocksize
    out << (quint16)0;

    // Write sendCounter
    out << sendCounter++;

    // Write data
    out << s;

    // Go back and write blocksize so far
    out.device()->seek(0);
    quint16 bs=(quint16)(block.size() - 2*sizeof(quint16));
    out << bs;

    if (debug)
	qDebug() << "ME::sendData  bs="<<bs<<"  counter="<<sendCounter<<"  s="<<qPrintable(s);

    for (int i=0; i<clientList.size(); ++i)
    {
	//qDebug() << "Sending \""<<qPrintable (s)<<"\" to "<<qPrintable (clientList.at(i)->peerAddress().toString());
	clientList.at(i)->write (block);
    }
}

void VymModel::readData ()
{
    while (clientSocket->bytesAvailable() >=(int)sizeof(quint16) )
    {
	if (debug)
	    qDebug() <<"readData  bytesAvail="<<clientSocket->bytesAvailable();
	quint16 recCounter;
	quint16 blockSize;

	QDataStream in(clientSocket);
	in.setVersion(QDataStream::Qt_4_0);

	in >> blockSize;
	in >> recCounter;
	
	QString t;
	in >>t;
	if (debug)
	    qDebug() << "VymModel::readData  command="<<qPrintable (t);
	bool noErr;
	QString errMsg;
	parseAtom (t,noErr,errMsg);

    }
    return;
}

void VymModel::displayNetworkError(QAbstractSocket::SocketError socketError)
{
    switch (socketError) {
    case QAbstractSocket::RemoteHostClosedError:
        break;
    case QAbstractSocket::HostNotFoundError:
        QMessageBox::information(NULL, vymName +" Network client",
                                 "The host was not found. Please check the "
                                    "host name and port settings.");
        break;
    case QAbstractSocket::ConnectionRefusedError:
        QMessageBox::information(NULL, vymName + " Network client",
                                 "The connection was refused by the peer. "
                                    "Make sure the fortune server is running, "
                                    "and check that the host name and port "
                                    "settings are correct.");
        break;
    default:
        QMessageBox::information(NULL, vymName + " Network client",
                                 QString("The following error occurred: %1.")
                                 .arg(clientSocket->errorString()));
    }
}

void VymModel::download (const QUrl &url, BranchItem *bi) 
{
    //qDebug()<<"VM::download "<<url; 
    if (!bi) bi=getSelectedBranch();
    if (!bi) 
    {
	qWarning ("VM::download bi==NULL");
	return;
    }

    new DownloadAgent (url,bi);
}

void VymModel::selectMapSelectionColor()
{
    QColor col = QColorDialog::getColor( defLinkColor, NULL);
    setSelectionColor (col);
}

void VymModel::setSelectionColorInt (QColor col)
{
    if ( !col.isValid() ) return;
    saveState (
	QString("setSelectionColor (%1)").arg(mapEditor->getSelectionColor().name()),
	QString("setSelectionColor (%1)").arg(col.name()),
	QString("Set color of selection box to %1").arg(col.name())
    );

    mapEditor->setSelectionColor (col);
}

void VymModel::emitSelectionChanged(const QItemSelection &newsel)
{
    emit (selectionChanged(newsel,newsel)); // needed e.g. to update geometry in editor
    sendSelection();
}

void VymModel::emitSelectionChanged()
{
    QItemSelection newsel=selModel->selection();
    emitSelectionChanged (newsel);
}

void VymModel::setSelectionColor(QColor col)
{
    if ( !col.isValid() ) return;
    saveState (
	QString("setSelectionColor (%1)").arg(mapEditor->getSelectionColor().name()),
	QString("setSelectionColor (%1)").arg(col.name()),
	QString("Set color of selection box to %1").arg(col.name())
    );
    setSelectionColorInt (col);
}

QColor VymModel::getSelectionColor()
{
    return mapEditor->getSelectionColor();
}

void VymModel::setHideTmpMode (TreeItem::HideTmpMode mode)  
{
    hidemode=mode;
    for (int i=0;i<rootItem->branchCount();i++)
	rootItem->getBranchNum(i)->setHideTmp (mode);
    reposition();
    if (mode==TreeItem::HideExport)
	unselectAll();
    else
	reselect();

    qApp->processEvents();
}

//////////////////////////////////////////////
// Selection related
//////////////////////////////////////////////

void VymModel::updateSelection(QItemSelection newsel,QItemSelection dsel)	
{
    QModelIndex ix;
    MapItem *mi;
    BranchItem *bi;
    bool do_reposition=false;
    foreach (ix, dsel.indexes() )
    {
	mi = static_cast<MapItem*>(ix.internalPointer());
	if (mi->isBranchLikeType() )
	    do_reposition=do_reposition || ((BranchItem*)mi)->resetTmpUnscroll();
	if (mi->getType()==TreeItem::XLink)
	{
	    Link *li=((XLinkItem*)mi)->getLink();
	    XLinkObj *xlo=li->getXLinkObj();
	    if (xlo) 
		xlo->setSelection (XLinkObj::Unselected);

	    do_reposition=do_reposition || li->getBeginBranch()->resetTmpUnscroll();
	    do_reposition=do_reposition || li->getEndBranch()->resetTmpUnscroll();
	}
    }    

    foreach (ix, newsel.indexes() )
    {
	mi = static_cast<MapItem*>(ix.internalPointer());
	if (mi->isBranchLikeType() )
	{
	    bi=(BranchItem*)mi;
	    if (bi->hasScrolledParent() )
	    {
		bi->tmpUnscroll();
		do_reposition=true;
	    }
	}
	if (mi->getType()==TreeItem::XLink)
	{
	    ((XLinkItem*)mi)->setSelection();

	    // begin/end branches need to be tmp unscrolled
	    Link *li=((XLinkItem*)mi)->getLink();
	    bi=li->getBeginBranch();
	    if (bi->hasScrolledParent() )
	    {
		bi->tmpUnscroll();
		do_reposition=true;
	    }
	    bi=li->getEndBranch();
	    if (bi->hasScrolledParent() )
	    {
		bi->tmpUnscroll();
		do_reposition=true;
	    }
	}
    }    
    if ( do_reposition ) reposition();
}

void VymModel::setSelectionModel (QItemSelectionModel *sm)
{
    selModel=sm;
}

QItemSelectionModel* VymModel::getSelectionModel()
{
    return selModel;
}

void VymModel::setSelectionBlocked (bool b)
{
    selectionBlocked=b;
}

bool VymModel::isSelectionBlocked()
{
    return selectionBlocked;
}

bool VymModel::select (const QString &s)
{
    if (s.isEmpty()) return false;
    TreeItem *ti=findBySelectString(s);
    if (ti) return select (index(ti));
    return false;
}

bool VymModel::selectID (const QString &s)
{
    if (s.isEmpty()) return false;
    TreeItem *ti=findUuid(QUuid(s));
    if (ti) return select (index(ti));
    return false;
}

bool VymModel::select (LinkableMapObj *lmo)
{
    QItemSelection oldsel=selModel->selection();

    if (lmo)
	return select (lmo->getTreeItem() );
    else    
	return false;
}

bool VymModel::selectToggle (TreeItem *ti)
{
    if (ti) 
    { 
	selModel->select ( index(ti), QItemSelectionModel::Toggle);
	//appendSelection();	// FIXME-4 selection history not implemented yet for multiselections 
	return true;
    }
    return false;
}

bool VymModel::select (TreeItem *ti)
{
    if (ti) 
	return select (index(ti));
    else
	return false;
}

bool VymModel::select (const QModelIndex &index) 
{
    if (index.isValid() )
    {
	TreeItem *ti=getItem (index);
	if (ti->isBranchLikeType() )
	    ((BranchItem*)ti)->tmpUnscroll(); 
	reposition();
	selModel->select (index,QItemSelectionModel::ClearAndSelect  );
	appendSelection();
	return true;
    }
    return false;
}

void VymModel::unselectAll ()    
{
    unselect (selModel->selection() );
}

void VymModel::unselect(QItemSelection desel)  
{
    if (!desel.isEmpty())
    {
	lastSelectString=getSelectString();
	selModel->clearSelection(); 
    }
}   

bool VymModel::reselect()
{
    bool b=select (lastSelectString);
    return b;
}   

bool VymModel::canSelectPrevious()
{
    if (currentSelection>0)
	return true;
    else
	return false;
}

bool VymModel::selectPrevious()
{
    keepSelectionHistory=true;
    bool result=false;
    while (currentSelection>0)
    {
	currentSelection--;
	TreeItem *ti=findID (selectionHistory.at(currentSelection));
	if (ti) 
	{
	    result=select (ti);
	    break;
	} else
	    selectionHistory.removeAt (currentSelection);
    } 
    keepSelectionHistory=false;
    return result;
}   

bool VymModel::canSelectNext()
{
    if (currentSelection < selectionHistory.count()-1 )
	return true;
    else
	return false;
}

bool VymModel::selectNext()
{
    keepSelectionHistory=true;
    bool result=false;
    while (currentSelection<selectionHistory.count()-1)
    {
	currentSelection++;
	TreeItem *ti=findID (selectionHistory.at(currentSelection));
	if (ti) 
	{
	    result=select (ti);
	    break;
	} else
	    selectionHistory.removeAt (currentSelection);
    } 
    keepSelectionHistory=false;
    return result;
}   

void VymModel::resetSelectionHistory()
{
    selectionHistory.clear();
    currentSelection=-1;
    keepSelectionHistory=false;
    appendSelection();
}

void VymModel::appendSelection()    // FIXME-4 history unable to cope with multiple selections
{
    uint id=0;
    TreeItem *ti=getSelectedItem();
    if (ti && !keepSelectionHistory) 
    {
	if (ti->isBranchLikeType())
	    ((BranchItem*)ti)->setLastSelectedBranch();
	id=ti->getID();	
	selectionHistory.append (id);
	currentSelection=selectionHistory.count()-1;
	mainWindow->updateActions();
    }
}

void VymModel::emitShowSelection()  
{
    if (!blockReposition) emit (showSelection() );
}

void VymModel::emitNoteChanged (TreeItem *ti)
{
    QModelIndex ix=index(ti);
    emit (noteChanged (ix) );
}

void VymModel::emitDataChanged (TreeItem *ti)    
{
    QModelIndex ix=index(ti);
    emit ( dataChanged (ix,ix) );
    if (!blockReposition && ti->isBranchLikeType() && ((BranchItem*)ti)->getTask()  )
    {
	taskModel->emitDataChanged ( ((BranchItem*)ti)->getTask() );
	taskModel->recalcPriorities();
    }
}

void VymModel::emitUpdateQueries ()
{
    // Used to tell MainWindow to update query results
    if (blockReposition) return; 
    emit (updateQueries (this) );
}
void VymModel::emitUpdateLayout()
{
    if (settings.value("/mainwindow/autoLayout/use","true")=="true")
	emit (updateLayout());
}

bool VymModel::selectFirstBranch()
{
    TreeItem *ti=getSelectedBranch();
    if (ti)
    {
	TreeItem *par=ti->parent();
	if (par) 
	{
	    TreeItem *ti2=par->getFirstBranch();
	    if (ti2) return  select(ti2);
	}
    }	    
    return false;
}

bool VymModel::selectLastBranch()
{
    TreeItem *ti=getSelectedBranch();
    if (ti)
    {
	TreeItem *par=ti->parent();
	if (par) 
	{
	    TreeItem *ti2=par->getLastBranch();
	    if (ti2) return select(ti2);
	}
    }	    
    return false;
}

bool VymModel::selectLastSelectedBranch()
{
    BranchItem *bi=getSelectedBranch();
    if (bi)
    {
	bi=bi->getLastSelectedBranch();
	if (bi) return select (bi);
    }	    
    return false;
}

bool VymModel::selectLastImage()    
{
    TreeItem *ti=getSelectedBranch();
    if (ti)
    {
	TreeItem *par=ti->parent();
	if (par) 
	{
	    TreeItem *ti2=par->getLastImage();
	    if (ti2) return select(ti2);
	}
    }	    
    return false;
}

bool VymModel::selectParent()
{
    TreeItem *ti=getSelectedItem();
    TreeItem *par;
    if (ti)
    {
	par=ti->parent();
	if (par) 
	    return select(par);
    }	    
    return false;
}

TreeItem::Type VymModel::selectionType()
{
    TreeItem *ti = getSelectedItem();
    if (ti)
	return ti->getType();
    else
	return TreeItem::Undefined;
}

LinkableMapObj* VymModel::getSelectedLMO()
{
    QModelIndexList list=selModel->selectedIndexes();
    if (list.count()==1 )
    {
	TreeItem *ti = getItem (list.first() );
	TreeItem::Type type=ti->getType();
	if (type ==TreeItem::Branch || type==TreeItem::MapCenter || type==TreeItem::Image)
	    return ((MapItem*)ti)->getLMO();
    }
    return NULL;
}

BranchObj* VymModel::getSelectedBranchObj() // convenience function
{
    TreeItem *ti = getSelectedBranch();
    if (ti)
	return (BranchObj*)(  ((MapItem*)ti)->getLMO());
    else    
	return NULL;
}

BranchItem* VymModel::getSelectedBranch()
{
    TreeItem *ti=getSelectedItem();
    if (ti)
    {
	TreeItem::Type type=ti->getType();
	if (type ==TreeItem::Branch || type==TreeItem::MapCenter)
	    return (BranchItem*)ti;
    }
    return NULL;
}

QList <BranchItem*> VymModel::getSelectedBranches()
{
    QList <BranchItem*> bis;
    foreach (TreeItem *ti,getSelectedItems() )
    {
	TreeItem::Type type=ti->getType();
	if (type ==TreeItem::Branch || type==TreeItem::MapCenter)
	    bis.append ( (BranchItem*)ti );
    }
    return bis;
}

ImageItem* VymModel::getSelectedImage()
{
    TreeItem *ti=getSelectedItem();
    if (ti && ti->getType()==TreeItem::Image)
	return (ImageItem*)ti;
    else
	return NULL;
}

Task* VymModel::getSelectedTask()
{
    BranchItem *selbi=getSelectedBranch();
    if (selbi)
	return selbi->getTask();
    else
	return NULL;
}

Link* VymModel::getSelectedXLink()
{
    XLinkItem *xli=getSelectedXLinkItem();
    if (xli) return xli->getLink();
    return NULL;
}

XLinkItem* VymModel::getSelectedXLinkItem()
{
    TreeItem *ti=getSelectedItem();
    if (ti && ti->getType()==TreeItem::XLink)
	return (XLinkItem*)ti;
    else
	return NULL;
}

AttributeItem* VymModel::getSelectedAttribute()	
{
    TreeItem *ti=getSelectedItem();
    if (ti && ti->getType()==TreeItem::Attribute)
	return (AttributeItem*)ti;
    else
	return NULL;
}

TreeItem* VymModel::getSelectedItem()	
{
    if (!selModel) return NULL;
    QModelIndexList list=selModel->selectedIndexes();
    if (list.count()==1 )
	return getItem (list.first() );
    else    
	return NULL;
}

QList <TreeItem*> VymModel::getSelectedItems()	
{
    QList <TreeItem*> l;
    if (!selModel) return l;
    QModelIndexList list=selModel->selectedIndexes();
    foreach (QModelIndex ix,list)
	l.append (getItem (ix) );
    return l;
}

QModelIndex VymModel::getSelectedIndex()
{
    QModelIndexList list=selModel->selectedIndexes();
    if (list.count()==1 )
	return list.first();
    else
	return QModelIndex();
}

QList <uint> VymModel::getSelectedIDs()
{
    QList <uint> uids;
    foreach (TreeItem* ti,getSelectedItems() )
	uids.append (ti->getID() );
    return uids;	
}

bool VymModel::isSelected(TreeItem *ti)
{
    return getSelectedItems().contains (ti);
}

QString VymModel::getSelectString ()
{
    return getSelectString (getSelectedItem());
}

QString VymModel::getSelectString (LinkableMapObj *lmo)	// only for convenience. Used in MapEditor
{
    if (!lmo) return QString();
    return getSelectString (lmo->getTreeItem() );
}

QString VymModel::getSelectString (TreeItem *ti) 
{
    QString s;
    if (!ti || ti->depth()<0) return s;    
    switch (ti->getType())
    {
	case TreeItem::MapCenter: s="mc:"; break;
	case TreeItem::Branch: s="bo:";break;
	case TreeItem::Image: s="fi:";break;
	case TreeItem::Attribute: s="ai:";break;
	case TreeItem::XLink: s="xl:";break;
	default:
	    s="unknown type in VymModel::getSelectString()";
	    break;
    }
    s=  s + QString("%1").arg(ti->num());
    if (ti->depth() >0)
	// call myself recursively
	s= getSelectString(ti->parent()) +","+s;
    return s;
}

QString VymModel::getSelectString (BranchItem *bi) 
{
    return getSelectString ((TreeItem*)bi);
}

QString VymModel::getSelectString (const uint &i)
{
    return getSelectString ( findID (i) );
}

SlideModel* VymModel::getSlideModel()
{
    return slideModel;
}

int VymModel::slideCount() 
{
    return slideModel->count();
}

SlideItem* VymModel::addSlide()  
{
    SlideItem *si=slideModel->getSelectedItem();  
    if (si)
	si=slideModel->addSlide (NULL,si->childNumber()+1 );
    else
	si=slideModel->addSlide();
    
    TreeItem *seli=getSelectedItem();

    if (si && seli)
    {
	QString inScript;
        if (!loadStringFromDisk(vymBaseDir.path() + "/macros/slideeditor-snapshot.vys", inScript) )
        {
            qWarning()<<"VymModel::addSlide couldn't load template for taking snapshot";
            return NULL;
        }

        inScript.replace("CURRENT_ZOOM", QString().setNum(getMapEditor()->getZoomFactorTarget()) );
        inScript.replace("CURRENT_ANGLE", QString().setNum(getMapEditor()->getAngleTarget()) );
        inScript.replace("CURRENT_ID", "\"" + seli->getUuid().toString() + "\"");

	si->setInScript(inScript);
	slideModel->setData ( slideModel->index(si), seli->getHeading() );
    }
    QString s="<vymmap>" + si->saveToDir() + "</vymmap>";
    int pos=si->childNumber();
    saveState (
	PartOfMap,
	getSelectString(), QString("deleteSlide (%1)").arg(pos),
	getSelectString(), QString("addMapInsert (\"PATH\",%1)").arg(pos),
	"Add slide",
	NULL,
	s );
    return si;
}

void VymModel::deleteSlide(SlideItem *si)  
{
    if (si)
    {
	QString s="<vymmap>" + si->saveToDir() + "</vymmap>";
	int pos=si->childNumber();
	saveState (
	    PartOfMap,
	    getSelectString(), QString("addMapInsert (\"PATH\",%1)").arg(pos),
	    getSelectString(), QString("deleteSlide (%1)").arg(pos),
	    "Delete slide",
	    NULL,
	    s );
	slideModel->deleteSlide (si);
    }
}

void VymModel::deleteSlide(int n)  
{
    deleteSlide (slideModel->getSlide (n));
}

void VymModel::relinkSlide(SlideItem *si, int pos)
{
    if (si && pos>=0) 
	slideModel->relinkSlide (si, si->parent(), pos);
}

void VymModel::moveSlideUp(int n)  
{
    SlideItem *si=NULL;
    if (n<0) // default if called without parameters
    {
	si=slideModel->getSelectedItem();
	if (si) n=si->childNumber();
    } else
	si=slideModel->getSlide(n);
    if (si && n>0 && n<slideModel->count())
    {
	blockSlideSelection=true;
	slideModel->relinkSlide (si, si->parent(), n-1);
	blockSlideSelection=false;
	saveState (
	    getSelectString(),QString("moveSlideDown (%1)").arg(n-1),
	    getSelectString(),QString("moveSlideUp (%1)").arg(n),
	    QString("Move slide %1 up").arg(n));
    }
}

void VymModel::moveSlideDown(int n)   
{
    SlideItem *si=NULL;
    if (n<0) // default if called without parameters
    {
	si=slideModel->getSelectedItem();
	if (si) n=si->childNumber();
    } else
	si=slideModel->getSlide(n);
    if (si && n>=0 && n < slideModel->count()-1)
    {
	blockSlideSelection=true;
	slideModel->relinkSlide (si, si->parent(), n+1);
	blockSlideSelection=false;
	saveState (
	    getSelectString(),QString("moveSlideUp (%1)").arg(n+1),
	    getSelectString(),QString("moveSlideDown (%1)").arg(n),
	    QString("Move slide %1 down").arg(n));
    }
}

void VymModel::updateSlideSelection (QItemSelection newsel,QItemSelection)
{
    if (blockSlideSelection) return;
    QModelIndex ix;
    foreach (ix,newsel.indexes() )
    {
	SlideItem *si= static_cast<SlideItem*>(ix.internalPointer());
	QString inScript=si->getInScript();

	// show inScript in ScriptEditor
	scriptEditor->setSlideScript(modelID, si->getID(), inScript );

	// Execute inScript 
	execute (inScript);
    }
}


