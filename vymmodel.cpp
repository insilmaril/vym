#include <QApplication>
#include <typeinfo>

#include "vymmodel.h"

#include "attributeitem.h"
#include "treeitem.h"
#include "branchitem.h"
#include "bugagent.h"
#include "editxlinkdialog.h"
#include "exports.h"
#include "exporthtmldialog.h"
#include "file.h"
#include "findresultmodel.h"
#include "mainwindow.h"
#include "misc.h"
#include "noteeditor.h"
#include "parser.h"
#include "process.h"
#include "warningdialog.h"
#include "xlinkitem.h"
#include "xlinkobj.h"
#include "xml-freemind.h"
#include "xmlobj.h"
#include "xml-vym.h"


extern bool debug;
extern Main *mainWindow;
extern QDBusConnection dbusConnection;

extern Settings settings;
extern QString tmpVymDir;

extern NoteEditor *noteEditor;
extern FlagRow *standardFlagsMaster;

extern QString clipboardDir;
extern QString clipboardFile;
extern bool clipboardEmpty;

extern ImageIO imageIO;

extern QString vymName;
extern QString vymVersion;
extern QDir vymBaseDir;

extern QDir lastImageDir;
extern QDir lastFileDir;

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
    //qDebug() << "Destr VymModel begin this="<<this;
    autosaveTimer->stop();
    fileChangedTimer->stop();
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
    mapID=idLast;
    mapChanged=false;
    mapDefault=true;
    mapUnsaved=false;

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

    fileChangedTimer=new QTimer (this);	//FIXME-4 use QFileSystemWatcher
    fileChangedTimer->start(3000);
    connect(fileChangedTimer, SIGNAL(timeout()), this, SLOT(fileChanged()));


    // selections
    selModel=NULL;
    selectionBlocked=false;

    // find routine
    findReset();

    // animations   // FIXME-4 switch to new animation system 
    animationUse=settings.readBoolEntry("/animation/use",false);    // FIXME-4 add options to control _what_ is animated
    animationTicks=settings.readNumEntry("/animation/ticks",10);
    animationInterval=settings.readNumEntry("/animation/interval",50);
    animObjList.clear();    
    animationTimer=new QTimer (this);
    connect(animationTimer, SIGNAL(timeout()), this, SLOT(animate()));

    // View - map
    defLinkColor=QColor (0,0,255);
    defXLinkColor=QColor (180,180,180);
    linkcolorhint=LinkableMapObj::DefaultColor;
    linkstyle=LinkableMapObj::PolyParabel;
    defXLinkWidth=1;
    defXLinkColor=QColor (230,230,230);
    zoomFactor=1;

    hidemode=TreeItem::HideNone;

    // Avoid recursions later
    cleaningUpLinks=false;

    // Network
    netstate=Offline;

    //Initialize DBUS object
    adaptorModel=new AdaptorModel(this);    // Created and not deleted as documented in Qt
    if (!dbusConnection.registerObject (QString("/vymmodel_%1").arg(mapID),this))
	qWarning ("VymModel: Couldn't register DBUS object!");
}

void VymModel::makeTmpDirectories()
{
    // Create unique temporary directories
    tmpMapDir = tmpVymDir+QString("/model-%1").arg(mapID);
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

void VymModel::updateActions()	// FIXME-4  maybe don't update if blockReposition is set
{
    //cout << "VM::updateActions \n";
    // Tell mainwindow to update states of actions
    mainWindow->updateActions();
}



QString VymModel::saveToDir(const QString &tmpdir, const QString &prefix, bool writeflags, const QPointF &offset, TreeItem *saveSel)
{
    // tmpdir	    temporary directory to which data will be written
    // prefix	    mapname, which will be appended to images etc.
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
		  xml.attribut("comment",comment) +
		  xml.attribut("date",getDate()) +
		  xml.attribut("branchCount", QString().number(branchCount())) +
		  xml.attribut("backgroundColor", mapEditor->getScene()->backgroundBrush().color().name() ) +
		  xml.attribut("selectionColor", mapEditor->getSelectionColor().name() ) +
		  xml.attribut("linkStyle", ls ) +
		  xml.attribut("linkColor", defLinkColor.name() ) +
		  xml.attribut("defXLinkColor", defXLinkColor.name() ) +
		  xml.attribut("defXLinkWidth", QString().setNum(defXLinkWidth,10) ) +
		  xml.attribut("mapZoomFactor", QString().setNum(mapEditor->getZoomFactorTarget()) ) +
		  colhint; 
    s+=xml.beginElement("vymmap",mapAttr);
    xml.incIndent();

    // Find the used flags while traversing the tree	// FIXME-4 this can be done local to vymmodel maybe...
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
	filePath=QDir(fpath).absPath();
	fileDir=filePath.left (1+filePath.findRev ("/"));

	// Set short name, too. Search from behind:
	int i=fileName.findRev("/");
	if (i>=0) fileName=fileName.remove (0,i+1);

	// Forget the .vym (or .xml) for name of map
	mapName=fileName.left(fileName.findRev(".",-1,true) );
    }
}

void VymModel::setFilePath(QString fpath)
{
    setFilePath (fpath,fpath);
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

ErrorCode VymModel::loadMap (
    QString fname, 
    const LoadMode &lmode, 
    bool saveStateFlag, 
    const FileType &ftype,
    int pos)
{
    ErrorCode err=success;

    // Get updated zoomFactor, before applying one read from file in the end
    if (mapEditor) zoomFactor=mapEditor->getZoomFactorTarget();

    // For ImportReplace let's insert a new branch and replace that
    BranchItem *selbi=getSelectedBranch();
    BranchItem *newbi=NULL;

    parseBaseHandler *handler;
    fileType=ftype;
    switch (fileType)
    {
	case VymMap: handler=new parseVYMHandler; break;
	case FreemindMap : handler=new parseFreemindHandler; break;
	default: 
	    QMessageBox::critical( 0, tr( "Critical Parse Error" ),
		   "Unknown FileType in VymModel::load()");
	return aborted;	
    }

    bool zipped_org=zipped;

    if (lmode==NewMap)
    {
	// Reset timestamp to check for later updates of file
	fileChangedTime=QFileInfo (destPath).lastModified();

	selModel->clearSelection();
    } else
    {
	if (!selbi) return aborted;
	if (lmode==ImportAdd)
	    if (saveStateFlag) saveStateChangingPart(
		selbi,
		selbi,
		QString("addMapInsert (%1)").arg(fname),
		QString("Add map %1 to %2").arg(fname).arg(getObjectName(selbi)));
	if (lmode==ImportReplace)
	{
	    if (saveStateFlag) saveStateChangingPart(
		selbi,
		selbi,
		QString("addMapReplace(%1)").arg(fname),
		QString("Add map %1 to %2").arg(fname).arg(getObjectName(selbi)));
	    newbi=addNewBranchInt (selbi,-1);	// Add below selection	
	    select (newbi);
	}
    }	
    

    // Create temporary directory for packing
    bool ok;
    QString tmpZipDir=makeTmpDir (ok,"vym-pack");
    if (!ok)
    {
	QMessageBox::critical( 0, tr( "Critical Load Error" ),
	   tr("Couldn't create temporary directory before load\n"));
	return aborted; 
    }

    // Try to unzip file
    err=unzipDir (tmpZipDir,fname);
    QString xmlfile;
    if (err==nozip)
    {
	xmlfile=fname;
	zipped=false;
    } else
    {
	zipped=true;
	
	// Look for mapname.xml
	xmlfile= fname.left(fname.findRev(".",-1,true));
	xmlfile=xmlfile.section( '/', -1 );
	QFile mfile( tmpZipDir + "/" + xmlfile + ".xml");
	if (!mfile.exists() )
	{
	    // mapname.xml does not exist, well, 
	    // maybe someone renamed the mapname.vym file...
	    // Try to find any .xml in the toplevel 
	    // directory of the .vym file
	    QStringList flist=QDir (tmpZipDir).entryList("*.xml");
	    if (flist.count()==1) 
	    {
		// Only one entry, take this one
		xmlfile=tmpZipDir + "/"+flist.first();
	    } else
	    {
		for ( QStringList::Iterator it = flist.begin(); it != flist.end(); ++it ) 
		    *it=tmpZipDir + "/" + *it;
		// TODO Multiple entries, load all (but only the first one into this ME)
		//mainWindow->fileLoadFromTmp (flist);
		//returnCode=1;	// Silently forget this attempt to load
		qWarning ("MainWindow::load (fn)  multimap found...");
	    }	
		
	    if (flist.isEmpty() )
	    {
		QMessageBox::critical( 0, tr( "Critical Load Error" ),
			   tr("Couldn't find a map (*.xml) in .vym archive.\n"));
		err=aborted;		       
	    }	
	} //file doesn't exist	
	else
	    xmlfile=mfile.name();
    }

    QFile file( xmlfile);

    // I am paranoid: file should exist anyway
    // according to check in mainwindow.
    if (!file.exists() )
    {
	QMessageBox::critical( 0, tr( "Critical Parse Error" ),
		   tr(QString("Couldn't open map %1").arg(file.name())));
	err=aborted;	
    } else
    {
	bool blockSaveStateOrg=blockSaveState;
	blockReposition=true;
	blockSaveState=true;
	mapEditor->setViewportUpdateMode (QGraphicsView::NoViewportUpdate);
	QXmlInputSource source( file);
	QXmlSimpleReader reader;
	reader.setContentHandler( handler );
	reader.setErrorHandler( handler );
	handler->setModel ( this);


	// We need to set the tmpDir in order  to load files with rel. path
	QString tmpdir;
	if (zipped)
	    tmpdir=tmpZipDir;
	else
	    tmpdir=fname.left(fname.findRev("/",-1));	
	handler->setTmpDir (tmpdir);
	handler->setInputFile (file.name());
	if (lmode==ImportReplace)
	    handler->setLoadMode (ImportAdd,pos);
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
	    }
    
	    if (lmode==ImportReplace)
	    {
		deleteItem (selbi);
		select (newbi);
		deleteKeepChildren (false);
	    }

	    if (debug) qDebug()<<"VM::load reposition first";	//FIXME-3 why calling reposition twice?
	    reposition();   // to generate bbox sizes
	    //qDebug()<<"VM::load reposition second";
	    //reposition();   // to also position totalBBoxes
	    emitSelectionChanged();
	} else 
	{
	    QMessageBox::critical( 0, tr( "Critical Parse Error" ),
		       tr( handler->errorProtocol() ) );
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

    if (mapEditor) mapEditor->setZoomFactorTarget (zoomFactor);

    //Update view (scene()->update() is not enough)
    qApp->processEvents();  // Update view (scene()->update() is not enough)
    return err;
}

ErrorCode VymModel::save (const SaveMode &savemode)
{
    QString tmpZipDir;
    QString mapFileName;
    QString safeFilePath;

    ErrorCode err=success;

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
		return aborted;
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
	    return aborted; 
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
	    saveFloatImage();
	else	
	    saveFile=saveToDir (fileDir,mapName+"-",true,QPointF(),getSelectedBranch());    
	// TODO take care of multiselections
    }	

    if (!saveStringToDisk(fileDir+mapFileName,saveFile))
    {
	err=aborted;
	qWarning ("ME::saveStringToDisk failed!");
    }

    if (zipped)
    {
	// zip
	if (err==success) err=zipDir (tmpZipDir,destPath);

	// Delete tmpDir
	removeDir (QDir(tmpZipDir));

	// Restore original filepath outside of tmp zip dir
	setFilePath (safeFilePath);
    }

    updateActions();
    fileChangedTime=QFileInfo (destPath).lastModified();
    return err;
}

ImageItem* VymModel::loadFloatImageInt (BranchItem *dst,QString fn)
{
    ImageItem *ii=createImage(dst);
    if (ii)
    {
	ii->load (fn);
	reposition();
	return ii;
    }
    return NULL;
}   

void VymModel::loadFloatImage ()
{
    BranchItem *selbi=getSelectedBranch();
    if (selbi)
    {

	Q3FileDialog *fd=new Q3FileDialog( NULL);   // FIXME-4 get rid of Q3FileDialog
	fd->setMode (Q3FileDialog::ExistingFiles);
	fd->addFilter (QString (tr("Images") + " (*.png *.bmp *.xbm *.jpg *.png *.xpm *.gif *.pnm)"));
	ImagePreview *p =new ImagePreview (fd);
	fd->setContentsPreviewEnabled( TRUE );
	fd->setContentsPreview( p, p );
	fd->setPreviewMode( Q3FileDialog::Contents );
	fd->setCaption(vymName+" - " +tr("Load image"));
	fd->setDir (lastImageDir);
	fd->show();

	if ( fd->exec() == QDialog::Accepted )
	{
	    // TODO loadFIO in QT4 use:	lastImageDir=fd->directory();
	    lastImageDir=QDir (fd->dirPath());
	    QString s;
	    ImageItem *ii;
	    for (int j=0; j<fd->selectedFiles().count(); j++)
	    {
		s=fd->selectedFiles().at(j);
		ii=loadFloatImageInt (selbi,s);
		//FIXME-3 check savestate for loadImage 
		if (ii)
		    saveState(
			(TreeItem*)ii,
			"delete ()",
			selbi, 
			QString ("loadImage (\"%1\")").arg(s ),
			QString("Add image %1 to %2").arg(s).arg(getObjectName(selbi))
		    );
		else
		    // TODO loadFIO error handling
		    qWarning ("Failed to load "+s);
	    }
	}
	delete (p);
	delete (fd);
    }
}

void VymModel::saveFloatImageInt  (ImageItem *ii, const QString &type, const QString &fn)
{
    ii->save (fn,type);
}

void VymModel::saveFloatImage ()
{
    ImageItem *ii=getSelectedImage();
    if (ii)
    {
	QFileDialog *fd=new QFileDialog( NULL);
	fd->setFilters (imageIO.getFilters());
	fd->setCaption(vymName+" - " +tr("Save image"));
	fd->setFileMode( QFileDialog::AnyFile );
	fd->setDirectory (lastImageDir);
//	fd->setSelection (fio->getOriginalFilename());
	fd->show();

	QString fn;
	if ( fd->exec() == QDialog::Accepted && fd->selectedFiles().count()==1)
	{
	    fn=fd->selectedFiles().at(0);
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
			delete (fd);
			return;
			break;
		}
	    }
	    saveFloatImageInt (ii,fd->selectedFilter(),fn );
	}
	delete (fd);
    }
}


void VymModel::importDirInt(BranchItem *dst, QDir d)
{
    BranchItem *selbi=getSelectedBranch();
    BranchItem *bi;
    if (selbi)
    {
	int beginDepth=selbi->depth();

	// Traverse directories
	d.setFilter( QDir::Dirs| QDir::Hidden | QDir::NoSymLinks );
	QFileInfoList list = d.entryInfoList();
	QFileInfo fi;

	for (int i = 0; i < list.size(); ++i) 
	{
	    fi=list.at(i);
	    if (fi.fileName() != "." && fi.fileName() != ".." )
	    {
		bi=addNewBranchInt(dst,-2);
		bi->setHeading (fi.fileName() );    // FIXME-3 check this
		bi->setHeadingColor (QColor("blue"));
		if ( !d.cd(fi.fileName()) ) 
		    QMessageBox::critical (0,tr("Critical Import Error"),tr("Cannot find the directory %1").arg(fi.fileName()));
		else 
		{
		    // Recursively add subdirs
		    importDirInt (bi,d);
		    d.cdUp();
		}
		emitDataHasChanged(bi);
	    }	
	}	
	// Traverse files
	d.setFilter( QDir::Files| QDir::Hidden | QDir::NoSymLinks );
	list = d.entryInfoList();

	for (int i = 0; i < list.size(); ++i) 
	{
	    fi=list.at(i);
	    bi=addNewBranchInt (dst,-2);
	    bi->setHeading (fi.fileName() );
	    bi->setHeadingColor (QColor("black"));
	    if (fi.fileName().right(4) == ".vym" )
		bi->setVymLink (fi.filePath());
	    emitDataHasChanged(bi);
	}   

	// Scroll at least some stuff
	if (dst->branchCount()>1 && dst->depth()-beginDepth>2)
	    dst->toggleScroll();
    }	    
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

void VymModel::importDir()  //FIXME-3 check me... (not tested yet)
{
    BranchItem *selbi=getSelectedBranch();
    if (selbi)
    {
	QStringList filters;
	filters <<"VYM map (*.vym)";
	QFileDialog *fd=new QFileDialog( NULL,vymName+ " - " +tr("Choose directory structure to import"));
	fd->setMode (QFileDialog::DirectoryOnly);
	fd->setFilters (filters);
	fd->setCaption(vymName+" - " +tr("Choose directory structure to import"));
	fd->show();

	QString fn;
	if ( fd->exec() == QDialog::Accepted )
	{
	    importDirInt (fd->selectedFile() );
	    reposition();
	}
    }	
}


void VymModel::autosave()
{
    if (filePath=="") 
    {
	if (debug)
	    cout << "VymModel::autosave rejected due to missing filePath\n";
    }

    QDateTime now=QDateTime().currentDateTime();

    // Disable autosave, while we have gone back in history
    int redosAvail=undoSet.readNumEntry (QString("/history/redosAvail"));
    if (redosAvail>0) return;

    // Also disable autosave for new map without filename
    if (filePath.isEmpty()) return;


    if (mapUnsaved &&mapChanged && settings.value ("/mainwindow/autosave/use",true).toBool() )
    {
	if (QFileInfo(filePath).lastModified()<=fileChangedTime) 
	    mainWindow->fileSave (this);
	else
	    if (debug)
		cout <<"  ME::autosave  rejected, file on disk is newer than last save.\n"; 

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
	    // FIXME-3 VM switch to current mapeditor and finish lineedits...
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
		    loadMap (filePath);
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
    latestAddedItem=NULL;
    findReset();
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
    QString undoCommand=  undoSet.readEntry (QString("/history/step-%1/undoCommand").arg(curStep));
    QString undoSelection=undoSet.readEntry (QString("/history/step-%1/undoSelection").arg(curStep));
    QString redoCommand=  undoSet.readEntry (QString("/history/step-%1/redoCommand").arg(curStep));
    QString redoSelection=undoSet.readEntry (QString("/history/step-%1/redoSelection").arg(curStep));
    QString comment=undoSet.readEntry (QString("/history/step-%1/comment").arg(curStep));
    QString version=undoSet.readEntry ("/history/version");

    /* TODO Maybe check for version, if we save the history
    if (!checkVersion(version))
	QMessageBox::warning(0,tr("Warning"),
	    tr("Version %1 of saved undo/redo data\ndoes not match current vym version %2.").arg(version).arg(vymVersion));
    */ 

    // Find out current undo directory
    QString bakMapDir(QString(tmpMapDir+"/undo-%1").arg(curStep));

    if (debug)
    {
	cout << "VymModel::redo() begin\n";
	cout << "    undosAvail="<<undosAvail<<endl;
	cout << "    redosAvail="<<redosAvail<<endl;
	cout << "       curStep="<<curStep<<endl;
	cout << "    ---------------------------"<<endl;
	cout << "    comment="<<comment.toStdString()<<endl;
	cout << "    undoCom="<<undoCommand.toStdString()<<endl;
	cout << "    undoSel="<<undoSelection.toStdString()<<endl;
	cout << "    redoCom="<<redoCommand.toStdString()<<endl;
	cout << "    redoSel="<<redoSelection.toStdString()<<endl;
	cout << "    ---------------------------"<<endl<<endl;
    }

    // select  object before redo
    if (!redoSelection.isEmpty())
	select (redoSelection);


    bool noErr;
    QString errMsg;
    parseAtom (redoCommand,noErr,errMsg);

    blockSaveState=blockSaveStateOrg;

    undoSet.setEntry ("/history/undosAvail",QString::number(undosAvail));
    undoSet.setEntry ("/history/redosAvail",QString::number(redosAvail));
    undoSet.setEntry ("/history/curStep",QString::number(curStep));
    undoSet.writeSettings(histPath);

    mainWindow->updateHistory (undoSet);
    updateActions();

    /* TODO remove testing
    cout << "ME::redo() end\n";
    cout << "    undosAvail="<<undosAvail<<endl;
    cout << "    redosAvail="<<redosAvail<<endl;
    cout << "       curStep="<<curStep<<endl;
    cout << "    ---------------------------"<<endl<<endl;
    */


}

bool VymModel::isRedoAvailable()
{
    if (undoSet.readNumEntry("/history/redosAvail",0)>0)
	return true;
    else    
	return false;
}

void VymModel::undo()	
{
    // Can we undo at all?
    if (undosAvail<1) return;

    mainWindow->statusMessage (tr("Autosave disabled during undo."));

    bool blockSaveStateOrg=blockSaveState;
    blockSaveState=true;
    
    QString undoCommand=  undoSet.readEntry (QString("/history/step-%1/undoCommand").arg(curStep));
    QString undoSelection=undoSet.readEntry (QString("/history/step-%1/undoSelection").arg(curStep));
    QString redoCommand=  undoSet.readEntry (QString("/history/step-%1/redoCommand").arg(curStep));
    QString redoSelection=undoSet.readEntry (QString("/history/step-%1/redoSelection").arg(curStep));
    QString comment=undoSet.readEntry (QString("/history/step-%1/comment").arg(curStep));
    QString version=undoSet.readEntry ("/history/version");

    /* TODO Maybe check for version, if we save the history
    if (!checkVersion(version))
	QMessageBox::warning(0,tr("Warning"),
	    tr("Version %1 of saved undo/redo data\ndoes not match current vym version %2.").arg(version).arg(vymVersion));
    */

    // Find out current undo directory
    QString bakMapDir(QString(tmpMapDir+"/undo-%1").arg(curStep));

    // select  object before undo
    if (!select (undoSelection))
    {
	qWarning ("VymModel::undo()  Could not select object for undo");
	return;
    }


    if (debug)
    {
	cout << "VymModel::undo() begin\n";
	cout << "    undosAvail="<<undosAvail<<endl;
	cout << "    redosAvail="<<redosAvail<<endl;
	cout << "       curStep="<<curStep<<endl;
	cout << "    ---------------------------"<<endl;
	cout << "    comment="<<comment.toStdString()<<endl;
	cout << "    undoCom="<<undoCommand.toStdString()<<endl;
	cout << "    undoSel="<<undoSelection.toStdString()<<endl;
	cout << "    redoCom="<<redoCommand.toStdString()<<endl;
	cout << "    redoSel="<<redoSelection.toStdString()<<endl;
	cout << "    ---------------------------"<<endl<<endl;
    }	

    bool noErr;
    QString errMsg;
    parseAtom (undoCommand,noErr,errMsg);

    undosAvail--;
    curStep--; 
    if (curStep<1) curStep=stepsTotal;

    redosAvail++;

    blockSaveState=blockSaveStateOrg;
/* testing only
    cout << "VymModel::undo() end\n";
    cout << "    undosAvail="<<undosAvail<<endl;
    cout << "    redosAvail="<<redosAvail<<endl;
    cout << "       curStep="<<curStep<<endl;
    cout << "    ---------------------------"<<endl<<endl;
*/

    undoSet.setEntry ("/history/undosAvail",QString::number(undosAvail));
    undoSet.setEntry ("/history/redosAvail",QString::number(redosAvail));
    undoSet.setEntry ("/history/curStep",QString::number(curStep));
    undoSet.writeSettings(histPath);

    mainWindow->updateHistory (undoSet);
    updateActions();
    //emitSelectionChanged();
}

bool VymModel::isUndoAvailable()
{
    if (undoSet.readNumEntry("/history/undosAvail",0)>0)
	return true;
    else    
	return false;
}

void VymModel::gotoHistoryStep (int i)
{
    // Restore variables
    int undosAvail=undoSet.readNumEntry (QString("/history/undosAvail"));
    int redosAvail=undoSet.readNumEntry (QString("/history/redosAvail"));

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
	    if (debug) cout << "VymModel::gotoHistoryStep redo "<<j<<"/"<<undosAvail<<" i="<<i<<endl;
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

    stepsTotal=settings.readNumEntry("/history/stepsTotal",100);
    undoSet.setEntry ("/history/stepsTotal",QString::number(stepsTotal));
    mainWindow->updateHistory (undoSet);
}

void VymModel::saveState(const SaveMode &savemode, const QString &undoSelection, const QString &undoCom, const QString &redoSelection, const QString &redoCom, const QString &comment, TreeItem *saveSel)
{
    sendData(redoCom);	//FIXME-4 testing

    // Main saveState


    if (blockSaveState) return;

    if (debug) cout << "VM::saveState() for  "<<qPrintable (mapName)<<endl;
    
    // Find out current undo directory
    if (undosAvail<stepsTotal) undosAvail++;
    curStep++;
    if (curStep>stepsTotal) curStep=1;
    
    QString backupXML="";
    QString histDir=getHistoryPath();
    QString bakMapPath=histDir+"/map.xml";

    // Create histDir if not available
    QDir d(histDir);
    if (!d.exists()) 
	makeSubDirs (histDir);

    // Save depending on how much needs to be saved 
    QList <Link*> tmpLinks;
    if (saveSel)
	backupXML=saveToDir (histDir,mapName+"-",false, QPointF (),saveSel);
	
    QString undoCommand="";
    if (savemode==UndoCommand)
    {
	undoCommand=undoCom;
    }	
    else if (savemode==PartOfMap )
    {
	undoCommand=undoCom;
	undoCommand.replace ("PATH",bakMapPath);
    }


    if (!backupXML.isEmpty())
	// Write XML Data to disk
	saveStringToDisk (bakMapPath,backupXML);

    // We would have to save all actions in a tree, to keep track of 
    // possible redos after a action. Possible, but we are too lazy: forget about redos.
    redosAvail=0;

    // Write the current state to disk
    undoSet.setEntry ("/history/undosAvail",QString::number(undosAvail));
    undoSet.setEntry ("/history/redosAvail",QString::number(redosAvail));
    undoSet.setEntry ("/history/curStep",QString::number(curStep));
    undoSet.setEntry (QString("/history/step-%1/undoCommand").arg(curStep),undoCommand);
    undoSet.setEntry (QString("/history/step-%1/undoSelection").arg(curStep),undoSelection);
    undoSet.setEntry (QString("/history/step-%1/redoCommand").arg(curStep),redoCom);
    undoSet.setEntry (QString("/history/step-%1/redoSelection").arg(curStep),redoSelection);
    undoSet.setEntry (QString("/history/step-%1/comment").arg(curStep),comment);
    undoSet.setEntry (QString("/history/version"),vymVersion);
    undoSet.writeSettings(histPath);

    if (debug)
    {
	// TODO remove after testing
	//cout << "          into="<< histPath.toStdString()<<endl;
	cout << "    stepsTotal="<<stepsTotal<<
	", undosAvail="<<undosAvail<<
	", redosAvail="<<redosAvail<<
	", curStep="<<curStep<<endl;
	cout << "    ---------------------------"<<endl;
	cout << "    comment="<<comment.toStdString()<<endl;
	cout << "    undoCom="<<undoCommand.toStdString()<<endl;
	cout << "    undoSel="<<undoSelection.toStdString()<<endl;
	cout << "    redoCom="<<redoCom.toStdString()<<endl;
	cout << "    redoSel="<<redoSelection.toStdString()<<endl;
	if (saveSel) cout << "    saveSel="<<qPrintable (getSelectString(saveSel))<<endl;
	cout << "    ---------------------------"<<endl;
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
    if (redoSel->getType()==TreeItem::Branch) 
    {
	undoSelection=getSelectString (redoSel->parent());
	// save the selected branch of the map, Undo will insert part of map 
	saveState (PartOfMap,
	    undoSelection, QString("addMapInsert (\"PATH\",%1)").arg(redoSel->num()),
	    redoSelection, "delete ()", 
	    comment, 
	    redoSel);
    }
    if (redoSel->getType()==TreeItem::MapCenter) 
    {
	// save the selected branch of the map, Undo will insert part of map 
	saveState (PartOfMap,
	    undoSelection, QString("addMapInsert (\"PATH\")"),
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



QGraphicsScene* VymModel::getScene ()
{
    return mapEditor->getScene();
}

TreeItem* VymModel::findBySelectString(QString s)
{
    if (s.isEmpty() ) return NULL;

    // Old maps don't have multiple mapcenters and don't save full path
    if (s.left(2) !="mc")
	s="mc:0,"+s;

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

TreeItem* VymModel::findID (const uint &i)  //FIXME-3 Search also other types...
{
    BranchItem *cur=NULL;
    BranchItem *prev=NULL;
    nextBranch(cur,prev);
    while (cur) 
    {
	if (i==cur->getID() ) return cur;
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

void VymModel::setHeading(const QString &s) 
{
    BranchItem *selbi=getSelectedBranch();
    if (selbi->getHeading()==s) return;
    if (selbi)
    {
	saveState(
	    selbi,
	    "setHeading (\""+selbi->getHeading()+"\")", 
	    selbi,
	    "setHeading (\""+s+"\")", 
	    QString("Set heading of %1 to \"%2\"").arg(getObjectName(selbi)).arg(s) );
	selbi->setHeading(s );
	emitDataHasChanged ( selbi);	//FIXME-4 maybe emit signal from TreeItem?   //FIXME-2 called 2x ???
	emitUpdateQueries ();
	reposition();
	emitSelectionChanged();
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
	    "setNote (\""+selti->getNote()+"\")", 
	    selti,
	    "setNote (\""+s+"\")", 
	    QString("Set note of %1 ").arg(getObjectName(selti)) );
    }
    selti->setNote(s);
    emitNoteHasChanged(selti);
    emitDataHasChanged(selti);
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
		cout << firstdup.key().toStdString() << endl;
		cout << " - "<< firstdup.value() <<" - "<<firstdup.value()->getHeadingStd()<<endl;
	    }	
	    cout << " - "<< i.value() <<" - "<<i.value()->getHeadingStd()<<endl;
	} else
	    firstdup=i;

	++i;
    }
}

void  VymModel::findAll (FindResultModel *rmodel, QString s, Qt::CaseSensitivity cs)   
{
    rmodel->clear();
    rmodel->setSearchString (s);
    rmodel->setSearchFlags (0);	//FIXME-3 translate cs to QTextDocument::FindFlag
    BranchItem *cur=NULL;
    BranchItem *prev=NULL;
    nextBranch(cur,prev);

    FindResultItem *lastParent=NULL;
    while (cur) 
    {
	lastParent=NULL;
	if (cur->getHeading().contains (s,cs))
	    lastParent=rmodel->addItem (cur);
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
	    if (!nextBranch(findCurrent,findPrevious) )
		EOFind=true;
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

void VymModel::setURL(const QString &url) 
{
    TreeItem *selti=getSelectedItem();
    if (selti->getURL()==url) return;
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
	emitDataHasChanged (selti);
	reposition();
	emitSelectionChanged();
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
    BranchItem *cur=selbi;
    BranchItem *prev=NULL;
    while (cur) 
    {
	if (!cur->getURL().isEmpty()  && !(ignoreScrolled && cur->hasScrolledParent(cur) )) 
	    urls.append( cur->getURL());
	cur=nextBranch (cur,prev,true,selbi);
    }	
    return urls;
}


void VymModel::setFrameType(const FrameObj::FrameType &t)   //FIXME-4 not saved if there is no LMO
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

void VymModel::setFrameType(const QString &s)	//FIXME-4 not saved if there is no LMO
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

void VymModel::setFrameIncludeChildren (bool b)	    //FIXME-2 no savestate
{
    BranchItem *bi=getSelectedBranch();
    if (bi)
    {
	bi->setFrameIncludeChildren (b);
	emitDataHasChanged (bi);
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
	    emitSelectionChanged();
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
    if (bi)
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
	emitDataHasChanged ( bi);   
	reposition();
    }	
}

void VymModel::setIncludeImagesHor(bool b)  
{
    BranchItem *bi=getSelectedBranch();
    if (bi)
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
	emitDataHasChanged ( bi);
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

void VymModel::setHideExport(bool b)
{
    TreeItem *ti=getSelectedItem();
    if (ti && 
	(ti->getType()==TreeItem::Image ||ti->isBranchLikeType()))
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
	    emitDataHasChanged(ti);
	    emitSelectionChanged();
	updateActions();
	reposition();
	// emitSelectionChanged();
	// FIXME-3 VM needed? scene()->update();
    }
}

void VymModel::toggleHideExport()
{
    TreeItem *selti=getSelectedItem();
    if (selti)
	setHideExport ( !selti->hideInExport() );
}

void VymModel::addTimestamp()	//FIXME-3 new function, localize
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
	emitDataHasChanged ( selbi);	//FIXME-3 maybe emit signal from TreeItem? 
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
	if (redosAvail == 0)
	{
	    // Copy to history
	    QString s=getSelectString(selti);
	    saveState (PartOfMap, s, "nop ()", s, "copy ()","Copy selection to clipboard",selti  );
	    curClipboard=curStep;
	}

	// Copy also to global clipboard, because we are at last step in history
	QString bakMapName(QString("history-%1").arg(curStep));
	QString bakMapDir(tmpMapDir +"/"+bakMapName);
	copyDir (bakMapDir,clipboardDir );

	clipboardEmpty=false;
	updateActions();
    }	    
}


void VymModel::pasteNoSave(const int &n)
{
    bool zippedOrg=zipped;
    if (redosAvail > 0 || n!=0)
    {
	// Use the "historical" buffer
	QString bakMapName(QString("history-%1").arg(n));
	QString bakMapDir(tmpMapDir +"/"+bakMapName);
	loadMap (bakMapDir+"/"+clipboardFile,ImportAdd, false);
    } else
	// Use the global buffer
	loadMap (clipboardDir+"/"+clipboardFile,ImportAdd, false);
    zipped=zippedOrg;
}

void VymModel::paste()	
{   
    BranchItem *selbi=getSelectedBranch();
    if (selbi)
    {
	saveStateChangingPart(
	    selbi,
	    selbi,
	    QString ("paste (%1)").arg(curClipboard),
	    QString("Paste to %1").arg( getObjectName(selbi))
	);
	pasteNoSave(0);
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
    if (bi && bi->canMoveUp()) 
	return relinkBranch (bi,(BranchItem*)bi->parent(),bi->num()-1);
    else    
	return false;
}

void VymModel::moveUp()	
{
    BranchItem *selbi=getSelectedBranch();
    if (selbi)
    {
	QString oldsel=getSelectString();
	if (moveUp (selbi))
	    saveState (
		getSelectString(),"moveDown ()",
		oldsel,"moveUp ()",
		QString("Move up %1").arg(getObjectName(selbi)));
    }
}

bool VymModel::moveDown(BranchItem *bi)	
{
    if (bi && bi->canMoveDown())
	return relinkBranch (bi,(BranchItem*)bi->parent(),bi->num()+1);
    else
	return false;
}

void VymModel::moveDown()   
{
    BranchItem *selbi=getSelectedBranch();
    if (selbi)
    {
	QString oldsel=getSelectString();
	if ( moveDown(selbi))
	    saveState (
		getSelectString(),"moveUp ()",
		oldsel,"moveDown ()",
		QString("Move down %1").arg(getObjectName(selbi)));
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
	if ( relinkBranch (selbi,rootItem,-1) )	//FIXME-1 undo does not work for detach branch???
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
	    {
	    saveStateChangingPart(
		selbi,selbi, "sortChildren ()",
		QString("Sort children of %1").arg(getObjectName(selbi)));
	    }
	    selbi->sortChildren(inverse);
	    reposition();
	    emitShowSelection();
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
	    if (!parix.isValid()) cout << "VM::createII invalid index\n";
	    n=dst->getRowNumAppend(newii);
	    beginInsertRows (parix,n,n);
	    dst->appendChild (newii);	
	    endInsertRows ();

	emit (layoutChanged() );

	// save scroll state. If scrolled, automatically select
	// new branch in order to tmp unscroll parent...
	newii->createMapObj(mapEditor->getScene());
	reposition();
	return newii;
    } 
    return NULL;
}

bool VymModel::createLink(Link *link, bool createMO)
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
		if (debug) qDebug()<<"VymModel::createLink link exists already, aborting";
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

	if (createMO) 
	{
	    link->createMapObj(mapEditor->getScene() );
	    reposition();
	}
//  } 
    //return newli;
    return true;
}

AttributeItem* VymModel::addAttribute()	    // FIXME-3 savestate missing

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

BranchItem* VymModel::addNewBranchInt(BranchItem *dst,int num)
{
    // Depending on pos:
    // -3	insert in children of parent  above selection 
    // -2	add branch to selection 
    // -1	insert in children of parent below selection 
    // 0..n	insert in children of parent at pos

    // Create TreeItem
    QList<QVariant> cData;
    cData << "" << "undef";

    BranchItem *parbi;
    QModelIndex parix;
    int n;
    BranchItem *newbi=new BranchItem (cData);	
    //newbi->setHeading (QApplication::translate("Heading of new branch in map", "new"));

    emit (layoutAboutToBeChanged() );

    if (num==-2)
    {
	parbi=dst;
	parix=index(parbi);
	n=parbi->getRowNumAppend (newbi);
	beginInsertRows (parix,n,n);	
	parbi->appendChild (newbi); 
	endInsertRows ();
    }else if (num==-1 || num==-3)
    {
	// insert below selection
	parbi=(BranchItem*)dst->parent();
	parix=index(parbi);  
	
	n=dst->childNumber() + (3+num)/2;   //-1 |-> 1;-3 |-> 0
	beginInsertRows (parix,n,n);	
	parbi->insertBranch(n,newbi);	
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

BranchItem* VymModel::addNewBranch(int pos)
{
    // Different meaning than num in addNewBranchInt!
    // -1   add above
    //  0   add as child
    // +1   add below
    BranchItem *newbi=NULL;
    BranchItem *selbi=getSelectedBranch();

    if (selbi)
    {
	// FIXME-3 setCursor (Qt::ArrowCursor);  //Still needed?

	QString redosel=getSelectString(selbi);
	newbi=addNewBranchInt (selbi,pos-2);
	QString undosel=getSelectString(newbi);

	if (newbi)
	{
	    saveState(
		undosel,	
		"delete ()",
		redosel,
		QString ("addBranch (%1)").arg(pos),
		QString ("Add new branch to %1").arg(getObjectName(selbi)));	

	    reposition();
	    // emitSelectionChanged(); FIXME-3
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
	//QPointF p=bo->getRelPos();


	// add below selection
	newbi=addNewBranchInt (selbi,-1);

	if (newbi)
	{
	    //newbi->move2RelPos (p);

	    // Move selection to new branch
	    relinkBranch (selbi,newbi,0);

	    // Use color of child instead of parent
	    newbi->setHeadingColor (selbi->getHeadingColor() );
	    emitDataHasChanged (newbi);

	    saveState (newbi, "deleteKeepChildren ()", newbi, "addBranchBefore ()", 
		QString ("Add branch before %1").arg(getObjectName(selbi)));

	    // FIXME-3 needed? reposition();
	    // emitSelectionChanged(); FIXME-3 
	}
    }	
    return newbi;
}

bool VymModel::relinkBranch (BranchItem *branch, BranchItem *dst, int pos)
{
    if (branch && dst)
    {
	unselect();
 
	// Do we need to update frame type?
	bool keepFrame=false;
	 

	emit (layoutAboutToBeChanged() );
	BranchItem *branchpi=(BranchItem*)branch->parent();
	// Remove at current position
	int n=branch->childNum();

/* FIXME-4 seg since 20091207, if ModelTest active. strange.
	// error occured if relinking branch to empty mainbranch
	cout<<"VM::relinkBranch:\n";
	cout<<"    b="<<branch->getHeadingStd()<<endl;
	cout<<"  dst="<<dst->getHeadingStd()<<endl;
	cout<<"  pos="<<pos<<endl;
	cout<<"   n1="<<n<<endl;
*/	
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
	if (branch->getType()==TreeItem::MapCenter) 
	    branch->setType(TreeItem::Branch);

	// reset parObj, fonts, frame, etc in related LMO or other view-objects
	branch->updateStyles(keepFrame);

	emit (layoutChanged() );
	reposition();	// both for moveUp/Down and relinking
	if (dst->isScrolled() )
	{
	    select (dst);   
	    branch->updateVisibility();
	}
	else	
	    select (branch);
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

    // Search for double entries (should not be necessary now) //FIXME-3
    // --> not implemented yet

    while (!deleteLaterIDs.isEmpty())
    {
	TreeItem *ti=rootItem->findID (deleteLaterIDs.takeFirst());
	if (ti) delete (ti);
    }
}

void VymModel::deleteLater(uint id)	
{
    if (!deleteLaterIDs.contains(id))
	deleteLaterIDs.append (id);
}

void VymModel::deleteSelection()    //FIXME-2 no undo for deleting MC
{
    BranchItem *selbi=getSelectedBranch();

    if (selbi)
    {	// Delete branch
	unselect();
	saveStateRemovingPart (selbi, QString ("Delete %1").arg(getObjectName(selbi)));

	BranchItem *pi=(BranchItem*)(deleteItem (selbi));
	if (pi)
	{
	    if (pi->isScrolled() && pi->branchCount()==0)
		pi->unScroll();
	    emitDataHasChanged(pi);
	    select (pi);
	    emitShowSelection();
	} else
	    emitDataHasChanged(rootItem); //FIXME-1 crash...
	return;
    }

    TreeItem *ti=getSelectedItem();

    if (ti)
    {	// Delete other item
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
	    unselect();
	    deleteItem (ti);
	    emitDataHasChanged (pi);
	    select (pi);
	    reposition();
	    emitShowSelection();
	} else
	    qWarning ("VymmModel::deleteSelection()  unknown type?!");
    }
}

void VymModel::deleteKeepChildren(bool saveStateFlag)	//FIXME-3 does not work yet for mapcenters 
//deleteKeePChilderen FIXME-3 children of scrolled branch stay invisible...

{
    BranchItem *selbi=getSelectedBranch();
    BranchItem *pi;
    if (selbi)
    {
	// Don't use this on mapcenter
	if (selbi->depth()<2) return;

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
	    selbi,
	    "deleteKeepChildren ()",
	    QString("Remove %1 and keep its children").arg(getObjectName(selbi))
	);

	QString sel=getSelectString(selbi);
	unselect();
	int pos=selbi->num();
	BranchItem *bi=selbi->getFirstBranch();
	while (bi)
	{
	    relinkBranch (bi,pi,pos);
	    bi=selbi->getFirstBranch();
	    pos++;
	}
	deleteItem (selbi);
	reposition();
	select (sel);
	BranchObj *bo=getSelectedBranchObj();
	if (bo) 
	{
	    bo->move2RelPos (p);
	    reposition();
	}
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
	QModelIndex parentIndex=index(pi);

	emit (layoutAboutToBeChanged() );

	int n=ti->childNum();
	beginRemoveRows (parentIndex,n,n);
	removeRows (n,1,parentIndex);
	endRemoveRows();
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
	    reposition();
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
	    emitDataHasChanged(bi);
	    emitSelectionChanged();
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
	if (bi->depth()==0) return false;
	if (bi->toggleScroll())
	{
	    reposition();
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
	    emitDataHasChanged(bi);
	    emitSelectionChanged();
	    mapEditor->getScene()->update(); //Needed for _quick_ update,  even in 1.13.x 
	    return true;
	}   
    }	
    return false;
}

void VymModel::toggleScroll()	
{
    BranchItem *bi=(BranchItem*)getSelectedBranch();
    if (bi && bi->isBranchLikeType() )
    {
	if (bi->isScrolled())
	    unscrollBranch (bi);
	else
	    scrollBranch (bi);
    }
    // saveState & reposition are called in above functions
}

void VymModel::unscrollChildren() 
{
    BranchItem *selbi=getSelectedBranch();
    BranchItem *prev=NULL;
    BranchItem *cur=selbi;
    if (selbi)
    {
	saveStateChangingPart(
	    selbi,
	    selbi,
	    QString ("unscrollChildren ()"),
	    QString ("unscroll all children of %1").arg(getObjectName(selbi))
	);  
	while (cur) 
	{
	    if (cur->isScrolled())
	    {
		cur->toggleScroll(); 
		emitDataHasChanged (cur);
	}
	    cur=nextBranch (cur,prev,true,selbi);
	}   
	updateActions();
	reposition();
    }	
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

void VymModel::toggleStandardFlag (const QString &name, FlagRow *master)
{
    BranchItem *bi=getSelectedBranch();
    if (bi) 
    {
	QString u,r;
	if (bi->isActiveStandardFlag(name))
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
	reposition();
	emitSelectionChanged();	
    }
}

void VymModel::addFloatImage (const QPixmap &img) 
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
	// FIXME-3 VM needed? scene()->update();
    }
}


void VymModel::colorBranch (QColor c)	
{
    BranchItem *selbi=getSelectedBranch();
    if (selbi)
    {
	saveState(
	    selbi, 
	    QString ("colorBranch (\"%1\")").arg(selbi->getHeadingColor().name()),
	    selbi,
	    QString ("colorBranch (\"%1\")").arg(c.name()),
	    QString("Set color of %1 to %2").arg(getObjectName(selbi)).arg(c.name())
	);  
	selbi->setHeadingColor(c); // color branch
	emitDataHasChanged (selbi);
	mapEditor->getScene()->update();
    }
}

void VymModel::colorSubtree (QColor c) 
{
    BranchItem *selbi=getSelectedBranch();
    if (selbi)
    {
	saveStateChangingPart(
	    selbi,
	    selbi,
	    QString ("colorSubtree (\"%1\")").arg(c.name()),
	    QString ("Set color of %1 and children to %2").arg(getObjectName(selbi)).arg(c.name())
	);  
	BranchItem *prev=NULL;
	BranchItem *cur=selbi;
	while (cur) 
	{
	    cur->setHeadingColor(c); // color links, color children
	    emitDataHasChanged (cur);
	    cur=nextBranch (cur,prev,true,selbi);
	}   
    mapEditor->getScene()->update();
    }
}

QColor VymModel::getCurrentHeadingColor()   
{
    BranchItem *selbi=getSelectedBranch();
    if (selbi)	return selbi->getHeadingColor();
	
    QMessageBox::warning(0,"Warning","Can't get color of heading,\nthere's no branch selected");
    return Qt::black;
}



void VymModel::editURL()    
{
    TreeItem *selti=getSelectedItem();
    if (selti)
    {	    
	bool ok;
	QString text = QInputDialog::getText(
		"VYM", tr("Enter URL:"), QLineEdit::Normal,
		selti->getURL(), &ok, NULL);
	if ( ok) 
	    // user entered something and pressed OK
	    setURL(text);
    }
}

void VymModel::editLocalURL()
{
    TreeItem *selti=getSelectedItem();
    if (selti)
    {	    
	QStringList filters;
	filters <<"All files (*)";
	filters << tr("Text","Filedialog") + " (*.txt)";
	filters << tr("Spreadsheet","Filedialog") + " (*.odp,*.sxc)";
	filters << tr("Textdocument","Filedialog") +" (*.odw,*.sxw)";
	filters << tr("Images","Filedialog") + " (*.png *.bmp *.xbm *.jpg *.png *.xpm *.gif *.pnm)";
	QFileDialog *fd=new QFileDialog( NULL,vymName+" - " +tr("Set URL to a local file"));
	fd->setFilters (filters);
	fd->setCaption(vymName+" - " +tr("Set URL to a local file"));
	fd->setDirectory (lastFileDir);
	if (! selti->getVymLink().isEmpty() )
	    fd->selectFile( selti->getURL() );
	fd->show();

	if ( fd->exec() == QDialog::Accepted )
	{
	    lastFileDir=QDir (fd->directory().path());
	    setURL (fd->selectedFile() );
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
	QRegExp rx("^(\\d+)");
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
	    "  zypper ar http://download.opensuse.org/repositories/openSUSE:/Tools/openSUSE_11.2/ openSUSE:Tools_11.2\n"+
	    "  zypper in perl-SUSE-BugzillaClient\n\n"+
	    QObject::tr("Alternatively you can also add the repository\n"
	    "and install the perl module for Bugzilla access using YaST","VymModel, how to install Bugzilla client module")
	);
	dia.setCaption(QObject::tr("Warning: Couldn't find Bugzilla client","VymModel"));
	dia.setShowAgainName("/BugzillaClient/missing");
	dia.exec();
	return;
    }
    
    BranchItem *selbi=getSelectedBranch();
    if (selbi)
    {	    
	QString url;
	BranchItem *prev=NULL;
	BranchItem *cur=selbi;
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
		cur=nextBranch (cur,prev,true,selbi);
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

void VymModel::editVymLink()
{
    BranchItem *bi=getSelectedBranch();
    if (bi)
    {	    
	QStringList filters;
	filters <<"VYM map (*.vym)";
	QFileDialog *fd=new QFileDialog( NULL,vymName+" - " +tr("Link to another map"));
	fd->setFilters (filters);
	fd->setCaption(vymName+" - " +tr("Link to another map"));
	fd->setDirectory (lastFileDir);
	if (! bi->getVymLink().isEmpty() )
	    fd->selectFile( bi->getVymLink() );
	fd->show();

	QString fn;
	if ( fd->exec() == QDialog::Accepted )
	{
	    lastFileDir=QDir (fd->directory().path());
	    saveState(
		bi,
		"setVymLink (\""+bi->getVymLink()+"\")",
		bi,
		"setVymLink (\""+fd->selectedFile()+"\")",
		QString("Set vymlink of %1 to %2").arg(getObjectName(bi)).arg(fd->selectedFile())
	    );	
	    setVymLink (fd->selectedFile() );	
	}
    }
}

void VymModel::setVymLink (const QString &s)
{
    // Internal function, no saveState needed
    TreeItem *selti=getSelectedItem();
    if (selti)
    {
	selti->setVymLink(s);
	emitDataHasChanged (selti);
	reposition();
	emitSelectionChanged();
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
	emitDataHasChanged (bi);
	reposition();
	emitSelectionChanged();
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
    BranchItem *cur=selbi;
    BranchItem *prev=NULL;
    while (cur) 
    {
	if (!cur->getVymLink().isEmpty()) links.append( cur->getVymLink());
	cur=nextBranch (cur,prev,true,selbi);
    }	
    return links;
}


void VymModel::followXLink(int i)   
{
    i=0;
    BranchItem *selbi=getSelectedBranch();
    if (selbi)
    {
	selbi=selbi->getXLinkItemNum(i)->getPartnerBranch();
	if (selbi) select (selbi);
    }
}

void VymModel::editXLink(int i)
{
    BranchItem *selbi=getSelectedBranch();
    if (selbi)
    {
	Link *l=selbi->getXLinkItemNum(i)->getLink();
	if (l) 
	{
	    EditXLinkDialog dia;
	    dia.setLink (l);
	    dia.setSelection(selbi);
	    if (dia.exec() == QDialog::Accepted)
	    {
		if (dia.useSettingsGlobal() )
		{
		    setMapDefXLinkColor (l->getColor() );
		    setMapDefXLinkWidth (l->getWidth() );
		}
		if (dia.deleteXLink()) delete (l);
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
    QString com=parser.getCommand();
    
    // External commands
    /////////////////////////////////////////////////////////////////////
    if (com=="addBranch")   
    {
	if (!selti)
	{
	    parser.setError (Aborted,"Nothing selected");
	} else if (! selbi )
	{		  
	    parser.setError (Aborted,"Type of selection is not a branch");
	} else 
	{   
	    QList <int> pl;
	    pl << 0 <<1;
	    if (parser.checkParCount(pl))
	    {
		if (parser.parCount()==0)
		    addNewBranch (0);
		else
		{
		    n=parser.parInt (ok,0);
		    if (ok ) addNewBranch (n);
		}
	    }
	}
    /////////////////////////////////////////////////////////////////////
    } else if (com=="addBranchBefore")
    {
	if (!selti)
	{
	    parser.setError (Aborted,"Nothing selected");
	} else if (! selbi )
	{		  
	    parser.setError (Aborted,"Type of selection is not a branch");
	} else 
	{   
	    if (parser.parCount()==0)
	    {
		addNewBranchBefore ();
	    }	
	}
    /////////////////////////////////////////////////////////////////////
    } else if (com==QString("addMapCenter"))
    {
	if (parser.checkParCount(2))
	{
	    x=parser.parDouble (ok,0);
	    if (ok)
	    {
		y=parser.parDouble (ok,1);
		if (ok) addMapCenter (QPointF(x,y));
	    }
	}   
    /////////////////////////////////////////////////////////////////////
    } else if (com==QString("addMapReplace"))
    {
	if (!selti)
	{
	    parser.setError (Aborted,"Nothing selected");
	} else if (! selbi )
	{		  
	    parser.setError (Aborted,"Type of selection is not a branch");
	} else if (parser.checkParCount(1))
	{
	    //s=parser.parString (ok,0);    // selection
	    t=parser.parString (ok,0);	// path to map
	    if (QDir::isRelativePath(t)) t=(tmpMapDir + "/"+t);
	    loadMap (t,ImportReplace,false,VymMap,n);
	}
    /////////////////////////////////////////////////////////////////////
    } else if (com==QString("addMapInsert"))
    {
	if (parser.parCount()==2)
	{

	    if (!selti)
	    {
		parser.setError (Aborted,"Nothing selected");
	    } else if (! selbi )
	    {		      
		parser.setError (Aborted,"Type of selection is not a branch");
	    } else 
	    {	
		t=parser.parString (ok,0);  // path to map
		n=parser.parInt(ok,1);	    // position
		if (QDir::isRelativePath(t)) t=(tmpMapDir + "/"+t);
		loadMap (t,ImportAdd,false,VymMap,n);
	    }
	} else if (parser.parCount()==1)
	{
	    t=parser.parString (ok,0);	// path to map
	    if (QDir::isRelativePath(t)) t=(tmpMapDir + "/"+t);
	    loadMap (t,ImportAdd,false);
	} else
	    parser.setError (Aborted,"Wrong number of parameters");
    /////////////////////////////////////////////////////////////////////
    } else if (com==QString("addXLink")) 
    {
	if (parser.parCount()>1)
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

		    createLink (li,true);   // create MO by default

		    if (parser.parCount()>2)
		    {
			int w=parser.parInt (ok,2); 
			if (ok) li->setWidth(w);
		    }
		    if (parser.parCount()>3)
		    {
			QColor col=parser.parColor (ok,2);
			if (ok) li->setColor (col);
		    }
		}
		else
		    parser.setError (Aborted,"begin or end of xLink are not branch or mapcenter");
		
	    } else
		parser.setError (Aborted,"Couldn't select begin or end of xLink");
	} else
	    parser.setError (Aborted,"Need at least 2 parameters for begin and end");
    /////////////////////////////////////////////////////////////////////
    } else if (com=="clearFlags")   
    {
	if (!selti )
	{
	    parser.setError (Aborted,"Nothing selected");
	} else if (! selbi )
	{		  
	    parser.setError (Aborted,"Type of selection is not a branch");
	} else if (parser.checkParCount(0))
	{
	    selbi->deactivateAllStandardFlags();
	    reposition();
	    emitDataHasChanged(selbi);
	    emitSelectionChanged();
	    setChanged();
	}
    /////////////////////////////////////////////////////////////////////
    } else if (com=="colorBranch")
    {
	if (!selti)
	{
	    parser.setError (Aborted,"Nothing selected");
	} else if (! selbi )
	{		  
	    parser.setError (Aborted,"Type of selection is not a branch");
	} else if (parser.checkParCount(1))
	{   
	    QColor c=parser.parColor (ok,0);
	    if (ok) colorBranch (c);
	}   
    /////////////////////////////////////////////////////////////////////
    } else if (com=="colorSubtree")
    {
	if (!selti)
	{
	    parser.setError (Aborted,"Nothing selected");
	} else if (! selbi )
	{		  
	    parser.setError (Aborted,"Type of selection is not a branch");
	} else if (parser.checkParCount(1))
	{   
	    QColor c=parser.parColor (ok,0);
	    if (ok) colorSubtree (c);
	}   
    /////////////////////////////////////////////////////////////////////
    } else if (com=="copy")
    {
	if (!selti)
	{
	    parser.setError (Aborted,"Nothing selected");
	} else if ( selectionType()!=TreeItem::Branch  && 
		    selectionType()!=TreeItem::MapCenter  &&
		    selectionType()!=TreeItem::Image )
	{		  
	    parser.setError (Aborted,"Type of selection is not a branch or floatimage");
	} else if (parser.checkParCount(0))
	{   
	    copy();
	}   
    /////////////////////////////////////////////////////////////////////
    } else if (com=="cut")
    {
	if (!selti)
	{
	    parser.setError (Aborted,"Nothing selected");
	} else if ( selectionType()!=TreeItem::Branch  && 
		    selectionType()!=TreeItem::MapCenter  &&
		    selectionType()!=TreeItem::Image )
	{		  
	    parser.setError (Aborted,"Type of selection is not a branch or floatimage");
	} else if (parser.checkParCount(0))
	{   
	    cut();
	}   
    /////////////////////////////////////////////////////////////////////
    } else if (com=="delete")
    {
	if (!selti)
	{
	    parser.setError (Aborted,"Nothing selected");
	} 
	/*else if (selectionType() != TreeItem::Branch && selectionType() != TreeItem::Image )
	{
	    parser.setError (Aborted,"Type of selection is wrong.");
	} 
	*/
	else if (parser.checkParCount(0))
	{   
	    deleteSelection();
	}   
    /////////////////////////////////////////////////////////////////////
    } else if (com=="deleteKeepChildren")
    {
	if (!selti)
	{
	    parser.setError (Aborted,"Nothing selected");
	} else if (! selbi )
	{
	    parser.setError (Aborted,"Type of selection is not a branch");
	} else if (parser.checkParCount(0))
	{   
	    deleteKeepChildren();
	}   
    /////////////////////////////////////////////////////////////////////
    } else if (com=="deleteChildren")
    {
	if (!selti)
	{
	    parser.setError (Aborted,"Nothing selected");
	} else if (! selbi)
	{
	    parser.setError (Aborted,"Type of selection is not a branch");
	} else if (parser.checkParCount(0))
	{   
	    deleteChildren();
	}   
    /////////////////////////////////////////////////////////////////////
    } else if (com=="exportAO")
    {
	QString fname="";
	ok=true;
	if (parser.parCount()>=1)
	    // Hey, we even have a filename
	    fname=parser.parString(ok,0); 
	if (!ok)
	{
	    parser.setError (Aborted,"Could not read filename");
	} else
	{
		exportAO (fname,false);
	}
    /////////////////////////////////////////////////////////////////////
    } else if (com=="exportASCII")
    {
	QString fname="";
	ok=true;
	if (parser.parCount()>=1)
	    // Hey, we even have a filename
	    fname=parser.parString(ok,0); 
	if (!ok)
	{
	    parser.setError (Aborted,"Could not read filename");
	} else
	{
		exportASCII (fname,false);
	}
    /////////////////////////////////////////////////////////////////////
    } else if (com=="exportHTML")
    {
	QString fname="";
	ok=true;
	if (parser.parCount()>=1)
	    // Hey, we even have a filename
	    fname=parser.parString(ok,0); 
	if (!ok)
	{
	    parser.setError (Aborted,"Could not read filename");
	} else
	{
	    exportHTML (fname,false);
	}
    /////////////////////////////////////////////////////////////////////
    } else if (com=="exportImage")
    {
	QString fname="";
	ok=true;
	if (parser.parCount()>=2)
	    // Hey, we even have a filename
	    fname=parser.parString(ok,0); 
	if (!ok)
	{
	    parser.setError (Aborted,"Could not read filename");
	} else
	{
	    QString format="PNG";
	    if (parser.parCount()>=2)
	    {
		format=parser.parString(ok,1);
	    }
	    exportImage (fname,false,format);
	}
    /////////////////////////////////////////////////////////////////////
    } else if (com=="exportXML")
    {
	QString fname="";
	ok=true;
	if (parser.parCount()>=2)
	    // Hey, we even have a filename
	    fname=parser.parString(ok,1); 
	if (!ok)
	{
	    parser.setError (Aborted,"Could not read filename");
	} else
	{
	    exportXML (fname,false);
	}
    /////////////////////////////////////////////////////////////////////
    } else if (com=="getHeading")
    { 
	if (!selti)
	{
	    parser.setError (Aborted,"Nothing selected");
	} else if (parser.checkParCount(0))
	    returnValue=selti->getHeading();
    /////////////////////////////////////////////////////////////////////
    } else if (com=="importDir")
    {
	if (!selti)
	{
	    parser.setError (Aborted,"Nothing selected");
	} else if (! selbi )
	{		  
	    parser.setError (Aborted,"Type of selection is not a branch");
	} else if (parser.checkParCount(1))
	{
	    s=parser.parString(ok,0);
	    if (ok) importDirInt(s);
	}   
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
		// 2,3	x,y of mainbranch or mapcenter
		s=parser.parString(ok,0);
		TreeItem *dst=findBySelectString (s);
		if (dst)
		{   
		    if (dst->getType()==TreeItem::Branch) 
		    {
			// Get number in parent
			n=parser.parInt (ok,1);
			if (ok)
			{
			    relinkBranch (selbi,(BranchItem*)dst,n);
			    emitSelectionChanged();
			}   
		    } else if (dst->getType()==TreeItem::MapCenter) 
		    {
			relinkBranch (selbi,(BranchItem*)dst);
			// Get coordinates of mainbranch
			x=parser.parDouble(ok,2);
			if (ok)
			{
			    y=parser.parDouble(ok,3);
			    if (ok) 
			    {
				if (selbi->getLMO()) selbi->getLMO()->move (x,y);
				emitSelectionChanged();
			    }
			}
		    }	
		}   
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
			relinkImage ( ((ImageItem*)selti),(BranchItem*)dst);
		} else	
		    parser.setError (Aborted,"Destination is not a branch");
	    }	    
	} else
	    parser.setError (Aborted,"Type of selection is not a floatimage or branch");
    /////////////////////////////////////////////////////////////////////
    } else if (com=="loadImage")
    {
	if (!selti)
	{
	    parser.setError (Aborted,"Nothing selected");
	} else if (! selbi )
	{		  
	    parser.setError (Aborted,"Type of selection is not a branch");
	} else if (parser.checkParCount(1))
	{
	    s=parser.parString(ok,0);
	    if (ok) 
		loadFloatImageInt (selbi,s);
	    else    
		parser.setError (Aborted,QString("Cannot load image: %1").arg(s));
	}   
    /////////////////////////////////////////////////////////////////////
    } else if (com=="loadNote")
    {
	if (!selti)
	{
	    parser.setError (Aborted,"Nothing selected");
	} else if (! selbi )
	{		  
	    parser.setError (Aborted,"Type of selection is not a branch");
	} else if (parser.checkParCount(1))
	{
	    s=parser.parString(ok,0);
	    if (ok) loadNote (s);
	}   
    /////////////////////////////////////////////////////////////////////
    } else if (com=="moveUp")
    {
	if (!selti )
	{
	    parser.setError (Aborted,"Nothing selected");
	} else if (! selbi )
	{		  
	    parser.setError (Aborted,"Type of selection is not a branch");
	} else if (parser.checkParCount(0))
	{
	    moveUp();
	}   
    /////////////////////////////////////////////////////////////////////
    } else if (com=="moveDown")
    {
	if (!selti )
	{
	    parser.setError (Aborted,"Nothing selected");
	} else if (! selbi )
	{		  
	    parser.setError (Aborted,"Type of selection is not a branch");
	} else if (parser.checkParCount(0))
	{
	    moveDown();
	}   
    /////////////////////////////////////////////////////////////////////
    } else if (com=="move")
    {
	if (!selti )
	{
	    parser.setError (Aborted,"Nothing selected");
	} else if ( selectionType()!=TreeItem::Branch  && 
		    selectionType()!=TreeItem::MapCenter  &&
		    selectionType()!=TreeItem::Image )
	{		  
	    parser.setError (Aborted,"Type of selection is not a branch or floatimage");
	} else if (parser.checkParCount(2))
	{   
	    x=parser.parDouble (ok,0);
	    if (ok)
	    {
		y=parser.parDouble (ok,1);
		if (ok) move (x,y);
	    }
	}   
    /////////////////////////////////////////////////////////////////////
    } else if (com=="moveRel")
    {
	if (!selti )
	{
	    parser.setError (Aborted,"Nothing selected");
	} else if ( selectionType()!=TreeItem::Branch  && 
		    selectionType()!=TreeItem::MapCenter  &&
		    selectionType()!=TreeItem::Image )
	{		  
	    parser.setError (Aborted,"Type of selection is not a branch or floatimage");
	} else if (parser.checkParCount(2))
	{   
	    x=parser.parDouble (ok,0);
	    if (ok)
	    {
		y=parser.parDouble (ok,1);
		if (ok) moveRel (x,y);
	    }
	}   
    /////////////////////////////////////////////////////////////////////
    } else if (com=="nop")
    {
    /////////////////////////////////////////////////////////////////////
    } else if (com=="paste")
    {
	if (!selti )
	{
	    parser.setError (Aborted,"Nothing selected");
	} else if (! selbi )
	{		  
	    parser.setError (Aborted,"Type of selection is not a branch");
	} else if (parser.checkParCount(1))
	{   
	    n=parser.parInt (ok,0);
	    if (ok) pasteNoSave(n);
	}   
    /////////////////////////////////////////////////////////////////////
    } else if (com=="qa")
    {
	if (!selti )
	{
	    parser.setError (Aborted,"Nothing selected");
	} else if (! selbi )
	{		  
	    parser.setError (Aborted,"Type of selection is not a branch");
	} else if (parser.checkParCount(4))
	{   
	    QString c,u;
	    c=parser.parString (ok,0);
	    if (!ok)
	    {
		parser.setError (Aborted,"No comment given");
	    } else
	    {
		s=parser.parString (ok,1);
		if (!ok)
		{
		    parser.setError (Aborted,"First parameter is not a string");
		} else
		{
		    t=parser.parString (ok,2);
		    if (!ok)
		    {
			parser.setError (Aborted,"Condition is not a string");
		    } else
		    {
			u=parser.parString (ok,3);
			if (!ok)
			{
			    parser.setError (Aborted,"Third parameter is not a string");
			} else
			{
			    if (s!="heading")
			    {
				parser.setError (Aborted,"Unknown type: "+s);
			    } else
			    {
				if (! (t=="eq") ) 
				{
				    parser.setError (Aborted,"Unknown operator: "+t);
				} else
				{
				    if (! selbi    )
				    {
					parser.setError (Aborted,"Type of selection is not a branch");
				    } else
				    {
					if (selbi->getHeading() == u)
					{
					    cout << "PASSED: " << qPrintable (c)  << endl;
					} else
					{
					    cout << "FAILED: " << qPrintable (c)  << endl;
					}
				    }
				}
			    }
			} 
		    } 
		} 
	    }
	}   
    /////////////////////////////////////////////////////////////////////
    } else if (com=="saveImage")
    {
	ImageItem *ii=getSelectedImage();
	if (!ii )
	{
	    parser.setError (Aborted,"No image selected");
	} else if (parser.checkParCount(2))
	{
	    s=parser.parString(ok,0);
	    if (ok)
	    {
		t=parser.parString(ok,1);
		if (ok) saveFloatImageInt (ii,t,s);
	    }
	}   
    /////////////////////////////////////////////////////////////////////
    } else if (com=="saveNote")
    {
	if (!selti)
	{
	    parser.setError (Aborted,"Nothing selected");
	} else if (! selbi )
	{		  
	    parser.setError (Aborted,"Type of selection is not a branch");
	} else if (parser.checkParCount(1))
	{
	    s=parser.parString(ok,0);
	    if (ok) saveNote (s);
	}   
    /////////////////////////////////////////////////////////////////////
    } else if (com=="scroll")
    {
	if (!selti)
	{
	    parser.setError (Aborted,"Nothing selected");
	} else if (! selbi )
	{		  
	    parser.setError (Aborted,"Type of selection is not a branch");
	} else if (parser.checkParCount(0))
	{   
	    if (!scrollBranch (selbi))	
		parser.setError (Aborted,"Could not scroll branch");
	}   
    /////////////////////////////////////////////////////////////////////
    } else if (com=="select")
    {
	if (parser.checkParCount(1))
	{
	    s=parser.parString(ok,0);
	    if (ok) select (s);
	}   
    /////////////////////////////////////////////////////////////////////
    } else if (com=="selectLastBranch")
    {
	if (!selti )
	{
	    parser.setError (Aborted,"Nothing selected");
	} else if (! selbi )
	{		  
	    parser.setError (Aborted,"Type of selection is not a branch");
	} else if (parser.checkParCount(0))
	{   
	    BranchItem *bi=selbi->getLastBranch();
	    if (!bi)
		parser.setError (Aborted,"Could not select last branch");
	    select (bi);
		
	}   
    /////////////////////////////////////////////////////////////////////
    } else if (com=="selectLastImage")
    {
	if (!selti )
	{
	    parser.setError (Aborted,"Nothing selected");
	} else if (! selbi )
	{		  
	    parser.setError (Aborted,"Type of selection is not a branch");
	} else if (parser.checkParCount(0))
	{   
	    ImageItem *ii=selbi->getLastImage();
	    if (!ii)
		parser.setError (Aborted,"Could not select last image");
	    select (ii);
		
	}   
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
	if (!selti )
	{
	    parser.setError (Aborted,"Nothing selected");
	} else if (! selbi )
	{		  
	    parser.setError (Aborted,"Type of selection is not a branch");
	} else if (parser.checkParCount(1))
	{
	    s=parser.parString(ok,0);
	    if (ok) 
		selbi->activateStandardFlag(s);
	}
    /////////////////////////////////////////////////////////////////////
    } else if (com=="setFrameType")
    {
	if ( selectionType()!=TreeItem::Branch && selectionType()!= TreeItem::MapCenter && selectionType()!=TreeItem::Image)
	{
	    parser.setError (Aborted,"Type of selection does not allow setting frame type");
	}
	else if (parser.checkParCount(1))
	{
	    s=parser.parString(ok,0);
	    if (ok) setFrameType (s);
	}   
    /////////////////////////////////////////////////////////////////////
    } else if (com=="setFramePenColor")
    {
	if ( selectionType()!=TreeItem::Branch && selectionType()!= TreeItem::MapCenter && selectionType()!=TreeItem::Image)
	{
	    parser.setError (Aborted,"Type of selection does not allow setting of pen color");
	}
	else if (parser.checkParCount(1))
	{
	    QColor c=parser.parColor(ok,0);
	    if (ok) setFramePenColor (c);
	}   
    /////////////////////////////////////////////////////////////////////
    } else if (com=="setFrameBrushColor")
    {
	if ( selectionType()!=TreeItem::Branch && selectionType()!= TreeItem::MapCenter && selectionType()!=TreeItem::Image)
	{
	    parser.setError (Aborted,"Type of selection does not allow setting brush color");
	}
	else if (parser.checkParCount(1))
	{
	    QColor c=parser.parColor(ok,0);
	    if (ok) setFrameBrushColor (c);
	}   
    /////////////////////////////////////////////////////////////////////
    } else if (com=="setFramePadding")
    {
	if ( selectionType()!=TreeItem::Branch && selectionType()!= TreeItem::MapCenter && selectionType()!=TreeItem::Image)
	{
	    parser.setError (Aborted,"Type of selection does not allow setting frame padding");
	}
	else if (parser.checkParCount(1))
	{
	    n=parser.parInt(ok,0);
	    if (ok) setFramePadding(n);
	}   
    /////////////////////////////////////////////////////////////////////
    } else if (com=="setFrameBorderWidth")
    {
	if ( selectionType()!=TreeItem::Branch && selectionType()!= TreeItem::MapCenter && selectionType()!=TreeItem::Image)
	{
	    parser.setError (Aborted,"Type of selection does not allow setting frame border width");
	}
	else if (parser.checkParCount(1))
	{
	    n=parser.parInt(ok,0);
	    if (ok) setFrameBorderWidth (n);
	}   
    /////////////////////////////////////////////////////////////////////
    } else if (com=="setFrameType")
    {
	if (!selti )
	{
	    parser.setError (Aborted,"Nothing selected");
	} else if (! selbi )
	{		  
	    parser.setError (Aborted,"Type of selection is not a branch");
	} else if (parser.checkParCount(1))
	{
	    s=parser.parString(ok,0);
	    if (ok) setFrameType (s);
	}
    /////////////////////////////////////////////////////////////////////
    } else if (com=="setHeading")
    {
	if (!selti )
	{
	    parser.setError (Aborted,"Nothing selected");
	} else if (! selbi )
	{		  
	    parser.setError (Aborted,"Type of selection is not a branch");
	} else if (parser.checkParCount(1))
	{
	    s=parser.parString (ok,0);
	    if (ok) 
		setHeading (s);
	}   
    /////////////////////////////////////////////////////////////////////
    } else if (com=="setHideExport")
    {
	if (!selti )
	{
	    parser.setError (Aborted,"Nothing selected");
	} else if (selectionType()!=TreeItem::Branch && selectionType() != TreeItem::MapCenter &&selectionType()!=TreeItem::Image)
	{		  
	    parser.setError (Aborted,"Type of selection is not a branch or floatimage");
	} else if (parser.checkParCount(1))
	{
	    b=parser.parBool(ok,0);
	    if (ok) setHideExport (b);
	}
    /////////////////////////////////////////////////////////////////////
    } else if (com=="setIncludeImagesHorizontally")
    { 
	if (!selti )
	{
	    parser.setError (Aborted,"Nothing selected");
	} else if (! selbi)
	{		  
	    parser.setError (Aborted,"Type of selection is not a branch");
	} else if (parser.checkParCount(1))
	{
	    b=parser.parBool(ok,0);
	    if (ok) setIncludeImagesHor(b);
	}
    /////////////////////////////////////////////////////////////////////
    } else if (com=="setIncludeImagesVertically")
    {
	if (!selti )
	{
	    parser.setError (Aborted,"Nothing selected");
	} else if (! selbi)
	{		  
	    parser.setError (Aborted,"Type of selection is not a branch");
	} else if (parser.checkParCount(1))
	{
	    b=parser.parBool(ok,0);
	    if (ok) setIncludeImagesVer(b);
	}
    /////////////////////////////////////////////////////////////////////
    } else if (com=="setHideLinkUnselected")
    {
	if (!selti )
	{
	    parser.setError (Aborted,"Nothing selected");
	} else if ( selectionType()!=TreeItem::Branch && selectionType()!= TreeItem::MapCenter && selectionType()!=TreeItem::Image)
	{		  
	    parser.setError (Aborted,"Type of selection does not allow hiding the link");
	} else if (parser.checkParCount(1))
	{
	    b=parser.parBool(ok,0);
	    if (ok) setHideLinkUnselected(b);
	}
    /////////////////////////////////////////////////////////////////////
    } else if (com=="setMapAuthor")
    {
	if (parser.checkParCount(1))
	{
	    s=parser.parString(ok,0);
	    if (ok) setAuthor (s);
	}   
    /////////////////////////////////////////////////////////////////////
    } else if (com=="setMapComment")
    {
	if (parser.checkParCount(1))
	{
	    s=parser.parString(ok,0);
	    if (ok) setComment(s);
	}   
    /////////////////////////////////////////////////////////////////////
    } else if (com=="setMapBackgroundColor")
    {
	if (!selti )
	{
	    parser.setError (Aborted,"Nothing selected");
	} else if (! selbi )
	{		  
	    parser.setError (Aborted,"Type of selection is not a branch");
	} else if (parser.checkParCount(1))
	{
	    QColor c=parser.parColor (ok,0);
	    if (ok) setMapBackgroundColor (c);
	}   
    /////////////////////////////////////////////////////////////////////
    } else if (com=="setMapDefLinkColor")
    {
	if (!selti )
	{
	    parser.setError (Aborted,"Nothing selected");
	} else if (! selbi )
	{		  
	    parser.setError (Aborted,"Type of selection is not a branch");
	} else if (parser.checkParCount(1))
	{
	    QColor c=parser.parColor (ok,0);
	    if (ok) setMapDefLinkColor (c);
	}   
    /////////////////////////////////////////////////////////////////////
    } else if (com=="setMapLinkStyle")
    {
	if (parser.checkParCount(1))
	{
	    s=parser.parString (ok,0);
	    if (ok) setMapLinkStyle(s);
	}   
    /////////////////////////////////////////////////////////////////////
    } else if (com=="setNote")
    {
	if (!selti )
	{
	    parser.setError (Aborted,"Nothing selected");
	} else if (! selbi )
	{		  
	    parser.setError (Aborted,"Type of selection is not a branch");
	} else if (parser.checkParCount(1))
	{
	    s=parser.parString (ok,0);
	    if (ok) 
		setNote (s);
	}   
    /////////////////////////////////////////////////////////////////////
    } else if (com=="setSelectionColor")
    {
	if (parser.checkParCount(1))
	{
	    QColor c=parser.parColor (ok,0);
	    if (ok) setSelectionColorInt (c);
	}   
    /////////////////////////////////////////////////////////////////////
    } else if (com=="setURL")
    {
	if (!selti )
	{
	    parser.setError (Aborted,"Nothing selected");
	} else if (! selbi )
	{		  
	    parser.setError (Aborted,"Type of selection is not a branch");
	} else if (parser.checkParCount(1))
	{
	    s=parser.parString (ok,0);
	    if (ok) setURL(s);
	}   
    /////////////////////////////////////////////////////////////////////
    } else if (com=="setVymLink")
    {
	if (!selti )
	{
	    parser.setError (Aborted,"Nothing selected");
	} else if (! selbi )
	{		  
	    parser.setError (Aborted,"Type of selection is not a branch");
	} else if (parser.checkParCount(1))
	{
	    s=parser.parString (ok,0);
	    if (ok) setVymLink(s);
	}   
    } else if (com=="sortChildren")
    {
	if (!selti )
	{
	    parser.setError (Aborted,"Nothing selected");
	} else if (! selbi )
	{		  
	    parser.setError (Aborted,"Type of selection is not a branch");
	} else if (parser.checkParCount(0))
	{
	    sortChildren();
	} else if (parser.checkParCount(1))
	{
	    b=parser.parBool(ok,0);
	    if (ok) sortChildren(b);
	}
    /////////////////////////////////////////////////////////////////////
    } else if (com=="toggleFlag")
    {
	if (!selti )
	{
	    parser.setError (Aborted,"Nothing selected");
	} else if (! selbi )
	{		  
	    parser.setError (Aborted,"Type of selection is not a branch");
	} else if (parser.checkParCount(1))
	{
	    s=parser.parString(ok,0);
	    if (ok) 
		selbi->toggleStandardFlag(s);	
	}
    /////////////////////////////////////////////////////////////////////
    } else  if (com=="unscroll")
    {
	if (!selti)
	{
	    parser.setError (Aborted,"Nothing selected");
	} else if (! selbi )
	{		  
	    parser.setError (Aborted,"Type of selection is not a branch");
	} else if (parser.checkParCount(0))
	{   
	    if (!unscrollBranch (selbi))    
		parser.setError (Aborted,"Could not unscroll branch");
	}   
    /////////////////////////////////////////////////////////////////////
    } else if (com=="unscrollChildren")
    {
	if (!selti)
	{
	    parser.setError (Aborted,"Nothing selected");
	} else if (! selbi )
	{		  
	    parser.setError (Aborted,"Type of selection is not a branch");
	} else if (parser.checkParCount(0))
	{   
	    unscrollChildren ();
	}   
    /////////////////////////////////////////////////////////////////////
    } else if (com=="unsetFlag")
    {
	if (!selti)
	{
	    parser.setError (Aborted,"Nothing selected");
	} else if (! selbi )
	{		  
	    parser.setError (Aborted,"Type of selection is not a branch");
	} else if (parser.checkParCount(1))
	{
	    s=parser.parString(ok,0);
	    if (ok) 
		selbi->deactivateStandardFlag(s);
	}
    } else 
	parser.setError (Aborted,"Unknown command");

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
	qWarning("VymModel::parseAtom: Error!");

	qWarning(parser.errorMessage());
	noErr=false;
	errorMsg=parser.errorMessage();
	returnValue=errorMsg;
    } 
    return returnValue;
}

QVariant VymModel::runScript (const QString &script)
{
    parser.setScript (script);
    parser.runScript();
    QVariant r;
    bool noErr=true;
    QString errMsg;
    while (parser.next() && noErr) 
    {
	r=parseAtom(parser.getAtom(),noErr,errMsg);
	if (!noErr) //FIXME-3 need dialog box here
	    cout << "VM::runScript aborted:\n"<<errMsg.toStdString()<<endl;
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
	fname=getMapName()+".png";
	format="PNG";
    }	

    if (askName)
    {
	QStringList fl;
	QFileDialog *fd=new QFileDialog (NULL);
	fd->setCaption (tr("Export map as image"));
	fd->setDirectory (lastImageDir);
	fd->setFileMode(QFileDialog::AnyFile);
	fd->setFilters  (imageIO.getFilters() );
	if (fd->exec())
	{
	    fl=fd->selectedFiles();
	    fname=fl.first();
	    format=imageIO.getType(fd->selectedFilter());
	} 
    }

    setExportMode (true);
    QPointF offset;
    QImage img (mapEditor->getImage(offset));
    img.save(fname, format);
    setExportMode (false);
    return offset;
}


void VymModel::exportXML(QString dir, bool askForName)
{
    if (askForName)
    {
	dir=browseDirectory(NULL,tr("Export XML to directory"));
	if (dir =="" && !reallyWriteDirectory(dir) )
	return;
    }

    // Hide stuff during export, if settings want this
    setExportMode (true);

    // Create subdirectories
    makeSubDirs (dir);

    // write image and calculate offset
    QPointF offset=exportImage (dir+"/images/"+mapName+".png",false,"PNG");

    // write to directory   //FIXME-4 check totalBBox here...
    QString saveFile=saveToDir (dir,mapName+"-",true,offset,NULL); 
    QFile file;

    file.setName ( dir + "/"+mapName+".xml");
    if ( !file.open( QIODevice::WriteOnly ) )
    {
	// This should neverever happen
	QMessageBox::critical (0,tr("Critical Export Error"),QString("VymModel::exportXML couldn't open %1").arg(file.name()));
	return;
    }	

    // Write it finally, and write in UTF8, no matter what 
    QTextStream ts( &file );
    ts.setEncoding (QTextStream::UnicodeUTF8);
    ts << saveFile;
    file.close();

    setExportMode (false);
}

void VymModel::exportAO (QString fname,bool askName)
{
    ExportAO ex;
    ex.setModel (this);
    if (fname=="") 
	ex.setFile (mapName+".txt");	
    else
	ex.setFile (fname);

    if (askName)
    {
	//ex.addFilter ("TXT (*.txt)");
	ex.setDir(lastImageDir);
	//ex.setCaption(vymName+ " -" +tr("Export as A&O report")+" "+tr("(still experimental)"));
	ex.execDialog("A&O") ; 
    } 
    if (!ex.canceled())
    {
	setExportMode(true);
	ex.doExport();
	setExportMode(false);
    }
}

void VymModel::exportASCII(QString fname,bool askName)
{
    ExportASCII ex;
    ex.setModel (this);
    if (fname=="") 
	ex.setFile (mapName+".txt");	
    else
	ex.setFile (fname);

    if (askName)
    {
	//ex.addFilter ("TXT (*.txt)");
	ex.setDir(lastImageDir);
	//ex.setCaption(vymName+ " -" +tr("Export as ASCII")+" "+tr("(still experimental)"));
	ex.execDialog("ASCII") ; 
    } 
    if (!ex.canceled())
    {
	setExportMode(true);
	ex.doExport();
	setExportMode(false);
    }
}

void VymModel::exportHTML (const QString &dir, bool useDialog)	
{
    ExportHTML ex (this);
    ex.setDir (dir);
    ex.doExport(useDialog);
}

void VymModel::exportOOPresentation(const QString &fn, const QString &cf)
{
    ExportOO ex;
    ex.setFile (fn);
    ex.setModel (this);
    if (ex.setConfigFile(cf)) 
    {
	setExportMode (true);
	ex.exportPresentation();
	setExportMode (false);
    }
}

bool VymModel::exportLastAvailable(QString &description, QString &command, QString &path)
{
    command=settings.readLocalEntry(filePath,"/export/last/command","");
    description=settings.readLocalEntry(filePath,"/export/last/description","");
    path=settings.readLocalEntry(filePath,"/export/last/exportPath","");
    if (!command.isEmpty() && command.startsWith("export") && !path.isEmpty())
	return true;
    else
	return false;
}   

void VymModel::exportLast()
{
    QString path;
    QString command;
    QString desc;
/* FIXME-3
    qDebug()<<"vm::exportLast";
    qDebug()<<exportDir;
    qDebug()<<command;
    qDebug()<<desc;
*/  
    QString s;
    if (exportLastAvailable(desc,command,path) )
	runScript (QString ("%1 (\"%2\")").arg(command).arg(path) );
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

void VymModel::setContextPos(QPointF p)
{
    contextPos=p;
}

void VymModel::unsetContextPos()
{
    contextPos=QPointF();
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
	    selti->setNote (noteEditor->getText());
	emitDataHasChanged(selti);	
    }
}

void VymModel::reposition() //FIXME-4 VM should have no need to reposition, but the views...
{
    //cout << "VM::reposition blocked="<<blockReposition<<endl;
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
    //emitSelectionChanged();	
}


void VymModel::setMapLinkStyle (const QString & s)
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
	    snow="UndefinedStyle";
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
	bo->setLinkStyle(bo->getDefLinkStyle(cur->parent() ));	//FIXME-3 better emit dataCHanged and leave the changes to View
	cur=nextBranch(cur,prev);
    }
    reposition();
}

LinkableMapObj::Style VymModel::getMapLinkStyle ()
{
    return linkstyle;
}   

uint VymModel::getID()
{
    return mapID;
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
    cur=nextBranch(cur,prev);
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
    cur=nextBranch(cur,prev);
    while (cur) 
    {
	bo=(BranchObj*)(cur->getLMO() );
	bo->setLinkColor();
	cur=nextBranch(cur,prev);
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
    cur=nextBranch(cur,prev);
    while (cur) 
    {
	bo=(BranchObj*)(cur->getLMO() );
	bo->setLinkColor();
	nextBranch(cur,prev);
    }
}

void VymModel::selectMapBackgroundImage ()  // FIXME-3 for using background image: view.setCacheMode(QGraphicsView::CacheBackground);  Also this belongs into ME
{
    Q3FileDialog *fd=new Q3FileDialog( NULL);
    fd->setMode (Q3FileDialog::ExistingFile);
    fd->addFilter (QString (tr("Images") + " (*.png *.bmp *.xbm *.jpg *.png *.xpm *.gif *.pnm)"));
    ImagePreview *p =new ImagePreview (fd);
    fd->setContentsPreviewEnabled( TRUE );
    fd->setContentsPreview( p, p );
    fd->setPreviewMode( Q3FileDialog::Contents );
    fd->setCaption(vymName+" - " +tr("Load background image"));
    fd->setDir (lastImageDir);
    fd->show();

    if ( fd->exec() == QDialog::Accepted )
    {
	// TODO selectMapBackgroundImg in QT4 use:  lastImageDir=fd->directory();
	lastImageDir=QDir (fd->dirPath());
	setMapBackgroundImage (fd->selectedFile());
    }
}   

void VymModel::setMapBackgroundImage (const QString &fn)    //FIXME-3 missing savestate, move to ME
{
    QColor oldcol=mapEditor->getScene()->backgroundBrush().color();
    /*
    saveState(
	selection,
	QString ("setMapBackgroundImage (%1)").arg(oldcol.name()),
	selection,
	QString ("setMapBackgroundImage (%1)").arg(col.name()),
	QString("Set background color of map to %1").arg(col.name()));
    */	
    QBrush brush;
    brush.setTextureImage (QPixmap (fn));
    mapEditor->getScene()->setBackgroundBrush(brush);
}

void VymModel::selectMapBackgroundColor()   // FIXME-3 move to ME
{
    QColor col = QColorDialog::getColor( mapEditor->getScene()->backgroundBrush().color(), NULL);
    if ( !col.isValid() ) return;
    setMapBackgroundColor( col );
}


void VymModel::setMapBackgroundColor(QColor col)    // FIXME-3 move to ME
{
    QColor oldcol=mapEditor->getScene()->backgroundBrush().color();
    saveState(
	QString ("setMapBackgroundColor (\"%1\")").arg(oldcol.name()),
	QString ("setMapBackgroundColor (\"%1\")").arg(col.name()),
	QString("Set background color of map to %1").arg(col.name()));
    mapEditor->getScene()->setBackgroundBrush(col);
}

QColor VymModel::getMapBackgroundColor()    // FIXME-3 move to ME
{
    return mapEditor->getScene()->backgroundBrush().color();
}


LinkableMapObj::ColorHint VymModel::getMapLinkColorHint()   // FIXME-3 move to ME
{
    return linkcolorhint;
}

QColor VymModel::getMapDefLinkColor()	// FIXME-3 move to ME
{
    return defLinkColor;
}

void VymModel::setMapDefXLinkColor(QColor col)	// FIXME-3 move to ME
{
    defXLinkColor=col;
}

QColor VymModel::getMapDefXLinkColor()	// FIXME-3 move to ME
{
    return defXLinkColor;
}

void VymModel::setMapDefXLinkWidth (int w)  // FIXME-3 move to ME
{
    defXLinkWidth=w;
}

int VymModel::getMapDefXLinkWidth() // FIXME-3 move to ME
{
    return defXLinkWidth;
}

void VymModel::move(const double &x, const double &y)
{
    int i=x; i=y;
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
    int i=x; i=y;
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
    QItemSelection sel=selModel->selection();
    emit (selectionChanged(sel,sel));

    mapEditor->getScene()->update();
    if (!animObjList.isEmpty()) animationTimer->start();
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
    cout<<"Server is running on port "<<tcpServer->serverPort()<<endl;
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
    cout<<"connected to "<<qPrintable (server)<<" port "<<port<<endl;

    
}

void VymModel::newClient()
{
    QTcpSocket *newClient = tcpServer->nextPendingConnection();
    connect(newClient, SIGNAL(disconnected()),
            newClient, SLOT(deleteLater()));

    cout <<"ME::newClient  at "<<qPrintable( newClient->peerAddress().toString() )<<endl;

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
	cout << "ME::sendData  bs="<<bs<<"  counter="<<sendCounter<<"  s="<<qPrintable(s)<<endl;

    for (int i=0; i<clientList.size(); ++i)
    {
	//cout << "Sending \""<<qPrintable (s)<<"\" to "<<qPrintable (clientList.at(i)->peerAddress().toString())<<endl;
	clientList.at(i)->write (block);
    }
}

void VymModel::readData ()
{
    while (clientSocket->bytesAvailable() >=(int)sizeof(quint16) )
    {
	if (debug)
	    cout <<"readData  bytesAvail="<<clientSocket->bytesAvailable();
	quint16 recCounter;
	quint16 blockSize;

	QDataStream in(clientSocket);
	in.setVersion(QDataStream::Qt_4_0);

	in >> blockSize;
	in >> recCounter;
	
	QString t;
	in >>t;
	if (debug)
	    cout << "VymModel::readData  command="<<qPrintable (t)<<endl;
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

/* FIXME-4 Playing with DBUS...
QDBusVariant VymModel::query (const QString &query)
{
    TreeItem *selti=getSelectedItem();
    if (selti)
	return QDBusVariant (selti->getHeading());
    else
	return QDBusVariant ("Nothing selected.");
}
*/

void VymModel::testslot()   //FIXME-4 Playing with DBUS
{
    cout << "VM::testslot called\n";
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
	unselect();
    else
	reselect();
}

//////////////////////////////////////////////
// Selection related
//////////////////////////////////////////////

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

bool VymModel::select ()
{
    return select (selModel->selectedIndexes().first());    // TODO no multiselections yet
}

bool VymModel::select (const QString &s)
{
    if (s.isEmpty())
    {
	unselect();
	return true;
    }
    TreeItem *ti=findBySelectString(s);
    if (ti) return select (index(ti));
    return false;
}

bool VymModel::select (LinkableMapObj *lmo)
{
    QItemSelection oldsel=selModel->selection();

    if (lmo)
	return select (index (lmo->getTreeItem()) );
    else    
	return false;
}

bool VymModel::select (TreeItem *ti)
{
    if (ti) return select (index(ti));
    return false;
}

bool VymModel::select (TreeItem *ti, int i)
{
    if (!ti || i<0) return false;
    if (select (index(ti)))
    {
	qDebug ()<<"VM::select with index: "<<i<<" Trying to find text in note ";
	QTextCursor c=noteEditor->getTextCursor();
	c.setPosition (i-1,QTextCursor::MoveAnchor);
	noteEditor->setTextCursor (c);
    } else  
	qDebug ()<<"VM::select with index: "<<i<<" Giving up to find text in note ";
    return true;    
}

bool VymModel::select (const QModelIndex &index)
{
    if (index.isValid() )
    {
	selModel->select (index,QItemSelectionModel::ClearAndSelect  );
	BranchItem *bi=getSelectedBranch();
	if (bi) bi->setLastSelectedBranch();
	return true;
    }
    return false;
}

void VymModel::unselect()
{
    if (!selModel->selectedIndexes().isEmpty())
    {
	lastSelectString=getSelectString();
	selModel->clearSelection();
    }
}   

bool VymModel::reselect()
{
    return select (lastSelectString);
}   

void VymModel::emitShowSelection()  
{
    if (!blockReposition)
	emit (showSelection() );
}

void VymModel::emitNoteHasChanged (TreeItem *ti)
{
    QModelIndex ix=index(ti);
    emit (noteHasChanged (ix) );
}

void VymModel::emitDataHasChanged (TreeItem *ti)
{
    QModelIndex ix=index(ti);
    emit (dataChanged (ix,ix) );
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
    QModelIndexList list=selModel->selectedIndexes();
    if (list.isEmpty()) return TreeItem::Undefined; 
    TreeItem *ti = getItem (list.first() );
    return ti->getType();

}

LinkableMapObj* VymModel::getSelectedLMO()
{
    QModelIndexList list=selModel->selectedIndexes();
    if (!list.isEmpty() )
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
    QModelIndexList list=selModel->selectedIndexes();
    if (!list.isEmpty() )
    {
	TreeItem *ti = getItem (list.first() );
	TreeItem::Type type=ti->getType();
	if (type ==TreeItem::Branch || type==TreeItem::MapCenter)
	    return (BranchItem*)ti;
    }
    return NULL;
}

ImageItem* VymModel::getSelectedImage()
{
    QModelIndexList list=selModel->selectedIndexes();
    if (!list.isEmpty())
    {
	TreeItem *ti=getItem (list.first());
	if (ti && ti->getType()==TreeItem::Image)
	    return (ImageItem*)ti;
    }
    return NULL;
}

AttributeItem* VymModel::getSelectedAttribute()	
{
    QModelIndexList list=selModel->selectedIndexes();
    if (!list.isEmpty() )
    {
	TreeItem *ti = getItem (list.first() );
	TreeItem::Type type=ti->getType();
	if (type ==TreeItem::Attribute)
	    return (AttributeItem*)ti;
    } 
    return NULL;
}

TreeItem* VymModel::getSelectedItem()	
{
    QModelIndexList list=selModel->selectedIndexes();
    if (!list.isEmpty() )
	return getItem (list.first() );
    else    
	return NULL;
}

QModelIndex VymModel::getSelectedIndex()
{
    QModelIndexList list=selModel->selectedIndexes();
    if (list.isEmpty() )
	return QModelIndex();
    else
	return list.first();
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
    if (!ti) return s;
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

