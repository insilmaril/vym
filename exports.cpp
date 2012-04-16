#include "exports.h"

#include <QDebug>

#include "branchitem.h"
#include "file.h"
#include "linkablemapobj.h"
#include "misc.h"
#include "mainwindow.h"
#include "warningdialog.h"
#include "xsltproc.h"


extern Main *mainWindow;
extern QDir vymBaseDir;
extern QString flagsPath;
extern QString vymName;
extern QString vymVersion;
extern QString vymHome;
extern Settings settings;
extern QDir lastExportDir;

ExportBase::ExportBase()
{
    init();
}

ExportBase::ExportBase(VymModel *m)
{
    model=m;
    init();
}

ExportBase::~ExportBase()
{
    // Cleanup tmpdir
    removeDir (tmpDir);

    // Remember current directory
    lastExportDir=outDir;
}

void ExportBase::init()
{
    indentPerDepth="  ";
    bool ok;
    tmpDir.setPath (makeTmpDir(ok,"vym-export"));
    if (!tmpDir.exists() || !ok)
	QMessageBox::critical( 0, QObject::tr( "Error" ),
		       QObject::tr("Couldn't access temporary directory\n"));
    cancelFlag=false;		       
    outDir=lastExportDir;
}

void ExportBase::setDirectory (const QDir &d)
{
    outDir=d;
}

void ExportBase::setFile (const QString &p)
{
    outputFile=p;
}

QString ExportBase::getFile ()
{
    return outputFile;
}

void ExportBase::setModel(VymModel *m)
{
    model=m;
}

void ExportBase::setWindowTitle (const QString &s)
{
    caption=s;
}

void ExportBase::addFilter(const QString &s)
{
    filter=s;
}

bool ExportBase::execDialog(const QString &overwriteWarning)
{
    QString fn=QFileDialog::getOpenFileName( 
	NULL,
	"Export xxx", 
	outDir.path(), 
	"LaTeX files (*.tex)");

    if (!fn.isEmpty() )
    {
	if (QFile (fn).exists() ) 
	{
	    WarningDialog dia;
	    dia.showCancelButton (true);
	    dia.setText(QObject::tr("Exporting to %1 will overwrite the existing file:\n%2").arg(overwriteWarning).arg(fn));
	    dia.setCaption(QObject::tr("Warning: Overwriting file"));
	    dia.setShowAgainName("/exports/overwrite/"+overwriteWarning);
	    if (!dia.exec()==QDialog::Accepted)
	    {
		cancelFlag=true;
		return false;
	    }
	}
	outDir.setPath(fn.left(fn.lastIndexOf ("/")) );
	outputFile=fn;
	if (model) model->setChanged();
	return true;
    }
    return false;
}

bool ExportBase::canceled()
{
    return cancelFlag;
}

QString ExportBase::getSectionString(TreeItem *start)
{
    // Make prefix like "2.5.3" for "bo:2,bo:5,bo:3"
    QString r;
    TreeItem *ti=start;
    int depth=ti->depth();
    while (depth>0)
    {
	r=QString("%1").arg(1+ti->num(),0,10)+"." + r;
	ti=ti->parent(); 
	depth=ti->depth();
    }	
    if (r.isEmpty())
	return r;
    else    
	return r + " ";
}

////////////////////////////////////////////////////////////////////////
ExportAO::ExportAO()
{
    filter="TXT (*.txt)";
    caption=vymName+ " -" +QObject::tr("Export as ASCII")+" "+QObject::tr("(still experimental)");
    indentPerDepth=" ";
}

void ExportAO::doExport()   
{
    QFile file (outputFile);
    if ( !file.open( QIODevice::WriteOnly ) )
    {
	qWarning()<<"ExportAO::doExport couldn't open "+outputFile;//FIXME-3 missing GUI warning
	return;
    }

    settings.setLocalValue (model->getFilePath(),"/export/last/exportDir",outputFile);
    settings.setLocalValue ( model->getFilePath(), "/export/last/command","exportAO");
    settings.setLocalValue ( model->getFilePath(), "/export/last/description","A&O report");

    QTextStream ts( &file );	// use LANG decoding here...

    // Main loop over all branches
    QString s;
    QString curIndent;
    QString dashIndent;

    int i;
    BranchItem *cur=NULL;
    BranchItem *prev=NULL;

    QString colString;
    QColor col;

    cur=model->nextBranch (cur,prev);
    while (cur) 
    {
	if (cur->getType()==TreeItem::Branch || cur->getType()==TreeItem::MapCenter)
	{
	    // Make indentstring
	    curIndent="";
	    for (i=0;i<cur->depth()-1;i++) curIndent+= indentPerDepth;

	    if (!cur->hasHiddenExportParent() )
	    {
		col=cur->getHeadingColor();
		if (col==QColor (255,0,0))
		    colString="[R] ";
		else if (col==QColor (217,81,0))
		    colString="[O] ";
		else if (col==QColor (0,85,0))
		    colString="[G] ";
		else	
		    colString="  - ";

		dashIndent="";	
		switch (cur->depth())
		{
		    case 0:
			//ts << underline (cur->getHeadingPlain(),QString("="));
			//ts << "\n";
			break;
		    case 1:
			//ts << "\n";
			//ts << (underline ( cur->getHeadingPlain(), QString("-") ) );
			//ts << "\n";
			break;
		    case 2: // Main heading
			ts << "\n";
			ts << underline ( cur->getHeadingPlain(), QString("=") );
			ts << "\n";
			break;
		    case 3: // Achievement, Bonus, Objective ...
			ts << "\n";
			ts << underline ( cur->getHeadingPlain(), "-");
			ts << "\n";
			break;
		    default:	// depth 4 are the items we need to know
			if (cur->depth()>4) ts<<curIndent;

			Task *task=cur->getTask();
			if (task)
			{
			    // Task status overrides other flags
			    switch ( task->getStatus() )
			    {
				case Task::NotStarted:
				    ts <<colString+cur->getHeadingPlain()<< " [NOT STARTED] ";
				    break;
				case Task::WIP:   
				    ts <<colString+cur->getHeadingPlain()<< " [WIP] ";
				    break;
				case Task::Finished:
				    ts <<colString+cur->getHeadingPlain()<< " [DONE] ";
				    break;
			    }
			} else
			{
			    if (cur->hasActiveStandardFlag ("hook-green") )
				ts <<colString+cur->getHeadingPlain()<< " [DONE] ";
			    else	if (cur->hasActiveStandardFlag ("wip"))
				ts <<colString+cur->getHeadingPlain()<< " [WIP] ";
			    else	if (cur->hasActiveStandardFlag ("cross-red"))
				ts <<colString+cur->getHeadingPlain()<< " [NOT STARTED] ";
			    else	
				ts <<"  - "+cur->getHeadingPlain();
			}
			ts << "\n";
			break;
		}

		// If necessary, write URL
		if (!cur->getURL().isEmpty())
		    ts << (curIndent + dashIndent + cur->getURL()) +"\n";

		// If necessary, write note
		if (!cur->getNoteObj().isEmpty())
		{
		    curIndent +="  | ";
		    s=cur->getNoteASCII( curIndent, 80);
		    ts << s;
		}
	    }
	}
	cur=model->nextBranch(cur,prev);
    }
    file.close();
    QString cmd="exportAO";
    settings.setLocalValue ( model->getFilePath(), "/export/last/exportPath",outputFile);
    settings.setLocalValue ( model->getFilePath(), "/export/last/command",cmd);
    settings.setLocalValue ( model->getFilePath(), "/export/last/description","A&O report");
    mainWindow->statusMessage(cmd + ": " + outputFile);
}

QString ExportAO::underline (const QString &text, const QString &line)
{
    QString r=text + "\n";
    for (int j=0;j<text.length();j++) r+=line;
    return r;
}


////////////////////////////////////////////////////////////////////////
ExportASCII::ExportASCII() 
{
    filter="TXT (*.txt)";
    caption=vymName+ " -" +QObject::tr("Export as ASCII");
}

void ExportASCII::doExport()	
{
    QFile file (outputFile);
    if ( !file.open( QIODevice::WriteOnly ) )
    {
	qWarning ()<<"ExportASCII::doExport couldn't open "+outputFile;
	return;
    }
    QTextStream ts( &file );	// use LANG decoding here...

    // Main loop over all branches
    QString s;
    QString curIndent;
    QString dashIndent;
    int i;
    BranchItem *cur=NULL;
    BranchItem *prev=NULL;

    cur=model->nextBranch (cur,prev);
    while (cur) 
    {
	if (cur->getType()==TreeItem::Branch || cur->getType()==TreeItem::MapCenter)
	{
	    // Make indentstring
	    curIndent="";
	    for (i=1;i<cur->depth()-1;i++) curIndent+= indentPerDepth;

	    if (!cur->hasHiddenExportParent() )
	    {
		//qDebug() << "ExportASCII::  "<<curIndent.toStdString()<<cur->getHeadingPlain().toStdString();

		dashIndent="";
		switch (cur->depth())
		{
		    case 0:
			ts << underline (cur->getHeadingPlain(),QString("="));
			ts << "\n";
			break;
		    case 1:
			ts << "\n";
			ts << (underline (getSectionString(cur) + cur->getHeadingPlain(), QString("-") ) );
			ts << "\n";
			break;
		    case 2:
			ts << "\n";
			ts << (curIndent + "* " + cur->getHeadingPlain());
			ts << "\n";
			dashIndent="  ";
			break;
		    case 3:
			ts << (curIndent + "- " + cur->getHeadingPlain());
			ts << "\n";
			dashIndent="  ";
			break;
		    default:
			ts << (curIndent + "- " + cur->getHeadingPlain());
			ts << "\n";
			dashIndent="  ";
			break;
		}

		// If necessary, write URL
		if (!cur->getURL().isEmpty())
		    ts << (curIndent + dashIndent + cur->getURL()) +"\n";

		// If necessary, write note
		if (!cur->getNoteObj().isEmpty())
		{
		    curIndent +="  | ";
		    s=cur->getNoteASCII( curIndent, 80);
		    ts << s;
		}
	    }
	}
	cur=model->nextBranch(cur,prev);
    }
    file.close();
    QString cmd="exportASCII";
    settings.setLocalValue ( model->getFilePath(), "/export/last/exportPath",outputFile);
    settings.setLocalValue ( model->getFilePath(), "/export/last/command",cmd);
    settings.setLocalValue ( model->getFilePath(), "/export/last/description","ASCII");
    mainWindow->statusMessage(cmd + ": " + outputFile);
}

QString ExportASCII::underline (const QString &text, const QString &line)
{
    QString r=text + "\n";
    for (int j=0;j<text.length();j++) r+=line;
    return r;
}


////////////////////////////////////////////////////////////////////////
void ExportCSV::doExport()
{
    QFile file (outputFile);
    if ( !file.open( QIODevice::WriteOnly ) )
    {
	qWarning ()<<"ExportBase::exportXML  couldn't open "+outputFile;
	return;
    }
    QTextStream ts( &file );	// use LANG decoding here...

    // Write header
    ts << "\"Note\""  <<endl;

    // Main loop over all branches
    QString s;
    QString curIndent("");
    int i;
    BranchItem *cur=NULL;
    BranchItem *prev=NULL;
    cur=model->nextBranch (cur,prev);
    while (cur) 
    {
	if (!cur->hasHiddenExportParent() )
	{
	    // If necessary, write note
	    if (!cur->getNoteObj().isEmpty())
	    {
		s =cur->getNoteASCII();
		s=s.replace ("\n","\n"+curIndent);
		ts << ("\""+s+"\",");
	    } else
		ts <<"\"\",";

	    // Make indentstring
	    for (i=0;i<cur->depth();i++) curIndent+= "\"\",";

	    // Write heading
	    ts << curIndent << "\"" << cur->getHeadingPlain()<<"\""<<endl;
	}
	
	cur=model->nextBranch(cur,prev);
	curIndent="";
    }
    file.close();
}

////////////////////////////////////////////////////////////////////////
void ExportKDE3Bookmarks::doExport() 
{
    WarningDialog dia;
    dia.showCancelButton (true);
    dia.setText(QObject::tr("Exporting the %1 bookmarks will overwrite\nyour existing bookmarks file.").arg("KDE"));
    dia.setCaption(QObject::tr("Warning: Overwriting %1 bookmarks").arg("KDE 3"));
    dia.setShowAgainName("/exports/overwrite/KDE3Bookmarks");
    if (dia.exec()==QDialog::Accepted)
    {
	model->exportXML(tmpDir.path(),false);

	XSLTProc p;
	p.setInputFile (tmpDir.path()+"/"+model->getMapName()+".xml");
	p.setOutputFile (tmpDir.home().path()+"/.kde/share/apps/konqueror/bookmarks.xml");
	p.setXSLFile (vymBaseDir.path()+"/styles/vym2kdebookmarks.xsl");
	p.process();

	QString ub=vymBaseDir.path()+"/scripts/update-bookmarks";
	QProcess *proc= new QProcess ;
	proc->start( ub);
	if (!proc->waitForStarted())
	{
	    QMessageBox::warning(0, 
		QObject::tr("Warning"),
		QObject::tr("Couldn't find script %1\nto notifiy Browsers of changed bookmarks.").arg(ub));
	}   
    }
}

////////////////////////////////////////////////////////////////////////
void ExportKDE4Bookmarks::doExport() 
{
    WarningDialog dia;
    dia.showCancelButton (true);
    dia.setText(QObject::tr("Exporting the %1 bookmarks will overwrite\nyour existing bookmarks file.").arg("KDE 4"));
    dia.setCaption(QObject::tr("Warning: Overwriting %1 bookmarks").arg("KDE"));
    dia.setShowAgainName("/exports/overwrite/KDE4Bookmarks");
    if (dia.exec()==QDialog::Accepted)
    {
	model->exportXML(tmpDir.path(),false);

	XSLTProc p;
	p.setInputFile (tmpDir.path()+"/"+model->getMapName()+".xml");
	p.setOutputFile (tmpDir.home().path()+"/.kde4/share/apps/konqueror/bookmarks.xml");
	p.setXSLFile (vymBaseDir.path()+"/styles/vym2kdebookmarks.xsl");
	p.process();

	QString ub=vymBaseDir.path()+"/scripts/update-bookmarks";
	QProcess *proc= new QProcess ;
	proc->start( ub);
	if (!proc->waitForStarted())
	{
	    QMessageBox::warning(0, 
		QObject::tr("Warning"),
		QObject::tr("Couldn't find script %1\nto notifiy Browsers of changed bookmarks.").arg(ub));
	}   
    }
}

////////////////////////////////////////////////////////////////////////
void ExportFirefoxBookmarks::doExport() 
{
    WarningDialog dia;
    dia.showCancelButton (true);
    dia.setText(QObject::tr("Exporting the %1 bookmarks will overwrite\nyour existing bookmarks file.").arg("Firefox"));
    dia.setCaption(QObject::tr("Warning: Overwriting %1 bookmarks").arg("Firefox"));
    dia.setShowAgainName("/vym/warnings/overwriteImportBookmarks");
    if (dia.exec()==QDialog::Accepted)
    {
	model->exportXML(tmpDir.path(),false);

/*
	XSLTProc p;
	p.setInputFile (tmpDir.path()+"/"+me->getMapName()+".xml");
	p.setOutputFile (tmpDir.home().path()+"/.kde/share/apps/konqueror/bookmarks.xml");
	p.setXSLFile (vymBaseDir.path()+"/styles/vym2kdebookmarks.xsl");
	p.process();

	QString ub=vymBaseDir.path()+"/scripts/update-bookmarks";
	QProcess *proc = new QProcess( );
	proc->addArgument(ub);

	if ( !proc->start() ) 
	{
	    QMessageBox::warning(0, 
		QObject::tr("Warning"),
		QObject::tr("Couldn't find script %1\nto notifiy Browsers of changed bookmarks.").arg(ub));
	}   

*/
    }
}

////////////////////////////////////////////////////////////////////////
ExportHTML::ExportHTML():ExportBase()
{
    init();
}

ExportHTML::ExportHTML(VymModel *m):ExportBase(m)
{
    init();
}

void ExportHTML::init()
{
    singularDelimiter=": ";
    noSingulars=false;	// Deactivated for the time being...
    frameURLs=true;
    cssFileName="vym.css";
    cssOriginalPath="";	// Is set in VymModel, based on default setting in ExportHTMLDialog

}

QString ExportHTML::getBranchText(BranchItem *current)
{
    if (current)
    {
	bool vis=false;
	QRectF hr;
	LinkableMapObj *lmo=current->getLMO();
	if (lmo)
	{
	    hr=((BranchObj*)lmo)->getBBoxHeading();
	    vis=lmo->isVisibleObj();
	}
	QString col;
	QString id=model->getSelectString(current);
	if (dia.useTextColor)
	    col=QString("style='color:%1'").arg(current->getHeadingColor().name());
	QString s=QString("<span class='vym-branch%1' %2 id='%3'>")
	    .arg(current->depth())
	    .arg(col)
	    .arg(id);
	QString url=current->getURL();	
	QString heading=quotemeta(current->getHeadingPlain());	
	if (!url.isEmpty())
	{
	    s+=QString ("<a href=\"%1\">").arg(url);
	    s+=QString ("<img src=\"flags/flag-url-16x16.png\">%1</a>").arg(heading);
	    s+="</a>";
	    
	    QRectF fbox=current->getBBoxURLFlag ();
	    if (vis)	
		imageMap+=QString("  <area shape='rect' coords='%1,%2,%3,%4' href='%5'>\n")
		    .arg(fbox.left()-offset.x())
		    .arg(fbox.top()-offset.y())
		    .arg(fbox.right()-offset.x())
		    .arg(fbox.bottom()-offset.y())
		    .arg(url);
	} else	
	    s+=quotemeta(current->getHeadingPlain());	
	s+="</span>";

	if (vis && dia.useImage)
	    imageMap+=QString("  <area shape='rect' coords='%1,%2,%3,%4' href='#%5'>\n")
		.arg(hr.left()-offset.x())
		.arg(hr.top()-offset.y())
		.arg(hr.right()-offset.x())
		.arg(hr.bottom()-offset.y())
		.arg(id);

	// Include note
	if (!current->getNoteObj().isEmpty())
	{
	    QString n;
	    if (current->getNoteObj().isRichText())
	    {
		n=current->getNote();
		QRegExp re("<p.*>");
		re.setMinimal (true);
		n.replace(re,"<p class=\"vym-note-paragraph\"");
	    }
	    else
	    {
		n=current->getNoteASCII().replace ("<","&lt;").replace (">","&gt;");
		n.replace("\n","<br/>");
	    }
	    s+="\n<table class=\"vym-note\"><tr><td>\n"+n+"\n</td></tr></table>\n";
	}   

	return s;
    } 
    return QString();
}

QString ExportHTML::buildList (BranchItem *current)
{
    QString r;

    uint i=0;
    BranchItem *bi=current->getFirstBranch();

    // Only add itemized list, if we have more than one subitem.
    // For only one subitem, just add a separator to keep page more compact
    bool noSingularsHere=false;
    if (current->branchCount()<2 && noSingulars) noSingularsHere=true;

    if (bi)
    {
	if (!noSingularsHere)
	    r+="<ul>\n";
	else
	    r+=singularDelimiter;

	while (bi)
	{
	    if (!bi->hasHiddenExportParent() )	
	    {
		if (!noSingularsHere) r+="<li>";
		r+=getBranchText (bi);
		if (!bi->getURL().isEmpty() && frameURLs && noSingularsHere)
		    // Add frame, if we have subitems to an URL
		    r+="<table border=1><tr><td>"+buildList (bi)+"</td></tr></table>";	// recursivly add deeper branches
		else
		    r+=buildList (bi);	// recursivly add deeper branches
		if (!noSingularsHere) r+="</li>";
		r+="\n";
	    }
	    i++;
	    bi=current->getBranchNum(i);
	}

	if (!noSingularsHere) r+="</ul>\n";
    }
    return r;
}

void ExportHTML::setCSSPath(const QString &p)
{
    cssOriginalPath=p;
}

void ExportHTML::doExport(bool useDialog) 
{
    // Execute dialog
    dia.setFilePath (model->getFilePath());
    dia.setMapName (model->getMapName());
    dia.readSettings();
    if (useDialog)
    {
	if (dia.exec()!=QDialog::Accepted) return;
	model->setChanged();
    }

    // Check if destination is not empty
    QDir d=dia.getDir();
    // Check, if warnings should be used before overwriting
    // the output directory
    if (d.exists() && d.count()>0)
    {
	WarningDialog warn;
	warn.showCancelButton (true);
	warn.setText(QString(
	    "The directory %1 is not empty.\n"
	    "Do you risk to overwrite some of its contents?").arg(d.path() ));
	warn.setCaption("Warning: Directory not empty");
	warn.setShowAgainName("mainwindow/export-XML-overwrite-dir");

	if (warn.exec()!=QDialog::Accepted) 
	{
	    mainWindow->statusMessage(QString(QObject::tr("Export aborted.")));
	    return;
	}
    }

    setFile (d.path()+"/"+model->getMapName()+".html");
    setCSSPath( dia.getCSSPath() ); 

    // Copy CSS file
    QString css;

    QString css_src (cssOriginalPath);
    QString css_dst (d.path()+"/"+cssFileName);
    if (!loadStringFromDisk (css_src,css))
	QMessageBox::warning( 0, 
	    QObject::tr( "Warning","ExportHTML" ),
	    QObject::tr("Trying to load stylesheet:")+"\n\n"+
	    QObject::tr("Could not open %1","ExportHTML").arg(cssOriginalPath));
    else
    {
	if (!saveStringToDisk (css_dst,css))
	    QMessageBox::warning( 0, 
		QObject::tr( "Warning" ), 
		QObject::tr("Trying to save stylesheet:")+"\n\n"+
		QObject::tr("Could not open %1").arg(css_dst));
    }

    // Provide a smaller URL-icon to improve Layout 
    QPixmap pm;
    QString urlName="flag-url-16x16.png";
    QString ipath=flagsPath+urlName;

    if (!pm.load(ipath,"PNG") )
	QMessageBox::warning( 0, 
	QObject::tr( "Warning" ),
	QObject::tr("Trying to load small icon for URLs:")+"\n\n"+
	QObject::tr("Could not open %1").arg(ipath));
    else
    {
	QString flagsPathExport=d.path()+"/flags";
	if (!d.exists(d.path()+"/flags"))
	{
	    if (!d.mkdir  ("flags"))
		QMessageBox::warning( 0,
		QObject:: tr( "Warning" ),
		QObject::tr("Trying to create directory for flags:")+"\n\n"+
		QObject::tr("Could not create %1").arg(flagsPathExport));
	}   
	if(!pm.save (flagsPathExport+"/"+urlName,"PNG"))
	    QMessageBox::warning( 0,
	    QObject::tr( "Warning" ),
	    QObject::tr("Trying to save small icon for URLs:")+"\n\n"+
	    QObject::tr("Could not write %1").arg(flagsPathExport+"/"+urlName));
    }	
    // Open file for writing
    QFile file (outputFile);
    if ( !file.open( QIODevice::WriteOnly ) ) 
    {
	QMessageBox::critical (0,
	    QObject::tr("Critical Export Error"),
	    QObject::tr("Trying to save HTML file:")+"\n\n"+
	    QObject::tr("Could not write %1").arg(outputFile));
	mainWindow->statusMessage(QString(QObject::tr("Export failed.")));
	return;
    }
    QTextStream ts( &file );	// use LANG decoding here...
    //FIXME-3 ts.setEncoding (QTextStream::UnicodeUTF8); // Force UTF8

    // Include image (be careful: this resets Export mode, so call before exporting branches)
    if (dia.useImage)
    {
	ts<<"<center><img src=\""<<model->getMapName()<<".png\" usemap='#imagemap'></center>\n";
	offset=model->exportImage (d.path()+"/"+model->getMapName()+".png",false,"PNG");
    }

    // Hide stuff during export
    model->setExportMode (true);

    // Write header
    ts<<"<html>";
    ts<<"<meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\"> ";
    ts<<"<meta name=\"generator=\" content=\" vym - view your mind - " + vymHome + "\">"; 
    ts<<"<meta name=\"author\" content=\"" + model->getAuthor() + "\"> ";
    ts<<"<meta name=\"description\" content=\"" + model->getComment() + "\"> ";
    ts<<"<title>"+model->getMapName()<<"</title><body>";
    ts<<" <link rel='stylesheet' id='css.stylesheet' href='"<<cssFileName<<"' />\n";

    // Main loop over all mapcenters
    QString s;
    TreeItem *rootItem=model->getRootItem();
    BranchItem *bi;
    for (int i=0; i<rootItem->branchCount(); i++)
    {
	bi=rootItem->getBranchNum(i);
	if (!bi->hasHiddenExportParent())
	{
	    ts<<getBranchText (bi);
	    ts<<buildList (bi);
	}
    }	

    // Imagemap
    ts<<"<map name='imagemap'>\n"+imageMap+"</map>\n";

    // Write footer 
    ts<<"<hr/>\n";
    ts<<"<table class=\"vym-footer\">   \n\
      <tr> \n\
        <td class=\"vym-footerL\">"+model->getFileName()+"</td> \n\
        <td class=\"vym-footerC\">"+model->getDate()+"</td> \n\
        <td class=\"vym-footerR\"> <a href='" + vymHome + "'>vym "+vymVersion+"</a></td> \n\
      </tr> \n \
    </table>\n";
    ts<<"</body></html>";
    file.close();

    if (!dia.postscript.isEmpty()) 
    {
	Process p;
	p.runScript (dia.postscript,d.path()+"/"+model->getMapName()+".html");
    }

    QString cmd="exportHTML";
    settings.setLocalValue (model->getFilePath(),"/export/last/exportPath",d.path());
    settings.setLocalValue ( model->getFilePath(), "/export/last/command","exportHTML");
    settings.setLocalValue ( model->getFilePath(), "/export/last/description","HTML");
    mainWindow->statusMessage(cmd + ": " + outputFile);

    dia.saveSettings();
    model->setExportMode (false);
}

////////////////////////////////////////////////////////////////////////
void ExportTaskjuggler::doExport() 
{
    model->exportXML(tmpDir.path(),false);

    XSLTProc p;
    p.setInputFile (tmpDir.path()+"/"+model->getMapName()+".xml");
    p.setOutputFile (outputFile);
    p.setXSLFile (vymBaseDir.path()+"/styles/vym2taskjuggler.xsl");
    p.process();
}

////////////////////////////////////////////////////////////////////////
void ExportOrgMode::doExport() 
{
    // Exports a map to an org-mode file.  
    // This file needs to be read 
    // by EMACS into an org mode buffer
    QFile file (outputFile);
    if ( !file.open( QIODevice::WriteOnly ) ) 
    {
	QMessageBox::critical (0,QObject::tr("Critical Export Error"),QObject::tr("Could not write %1").arg(outputFile));
	mainWindow->statusMessage(QString(QObject::tr("Export failed.")));
	return;
    }
    QTextStream ts( &file );  // use LANG decoding here...
    //FIXME-3 ts.setEncoding (QTextStream::UnicodeUTF8); // Force UTF8

    // Main loop over all branches
    QString s;
    int i;
    BranchItem *cur=NULL;
    BranchItem *prev=NULL;
    model->nextBranch(cur,prev);
    while (cur) 
    {
	if (!cur->hasHiddenExportParent() )
	{
	    for(i=0;i<=cur->depth();i++)
		ts << ("*");
	    ts << (" " + cur->getHeadingPlain()+ "\n");
	    // If necessary, write note
	    if (!cur->getNoteObj().isEmpty()) 
	    {
		ts << (cur->getNoteASCII());
		ts << ("\n");
	    }
	}
	cur=model->nextBranch(cur,prev);
    }
    file.close();
}

////////////////////////////////////////////////////////////////////////
void ExportLaTeX::doExport()	//FIXME-2 remember last directory
{
    // Exports a map to a LaTex file.  
    // This file needs to be included 
    // or inported into a LaTex document
    // it will not add a preamble, or anything 
    // that makes a full LaTex document.
  QFile file (outputFile);
  if ( !file.open( QIODevice::WriteOnly ) ) {
    QMessageBox::critical (0,QObject::tr("Critical Export Error"),QObject::tr("Could not write %1").arg(outputFile));
    mainWindow->statusMessage(QString(QObject::tr("Export failed.")));
    return;
  }
  QTextStream ts( &file );  // use LANG decoding here...
  //FIXME-3 ts.setEncoding (QTextStream::UnicodeUTF8); // Force UTF8
  
  // Main loop over all branches
  QString s;
  // QString curIndent("");
  // int i;
  BranchItem *cur=NULL;
  BranchItem *prev=NULL;
  model->nextBranch(cur,prev);
  while (cur) 
  {
    if (!cur->hasHiddenExportParent() )
    {
	switch (cur->depth() ) 
	{
	    case 0: break;
	    case 1: 
	      ts << ("\\chapter{" + cur->getHeadingPlain()+ "}\n");
	      break;
	    case 2: 
	      ts << ("\\section{" + cur->getHeadingPlain()+ "}\n");
	      break;
	    case 3: 
	      ts << ("\\subsection{" + cur->getHeadingPlain()+ "}\n");
	      break;
	    case 4: 
	      ts << ("\\subsubsection{" + cur->getHeadingPlain()+ "}\n");
	      break;
	    default:
	      ts << ("\\paragraph*{" + cur->getHeadingPlain()+ "}\n");
	    
	}
	// If necessary, write note
	if (!cur->getNoteObj().isEmpty()) {
	  ts << (cur->getNoteASCII());
	  ts << ("\n");
	}
    }
    cur=model->nextBranch(cur,prev);
   }
    
    file.close();
    QString cmd="exportLaTeX";
    settings.setLocalValue ( model->getFilePath(), "/export/last/exportPath",outputFile);
    settings.setLocalValue ( model->getFilePath(), "/export/last/command",cmd);
    settings.setLocalValue ( model->getFilePath(), "/export/last/description","LaTeX");
    mainWindow->statusMessage(cmd + ": " + outputFile);
}

////////////////////////////////////////////////////////////////////////
ExportOO::ExportOO()
{
    useSections=false;
}

ExportOO::~ExportOO()
{
}   

QString ExportOO::buildList (TreeItem *current)
{
    QString r;

    uint i=0;
    BranchItem *bi=current->getFirstBranch();
    if (bi)
    {
	// Start list
	r+="<text:list text:style-name=\"vym-list\">\n";
	while (bi)
	{
	    if (!bi->hasHiddenExportParent() )	
	    {
		r+="<text:list-item><text:p >";
		r+=quotemeta(bi->getHeadingPlain());
		// If necessary, write note
		if (!bi->getNoteObj().isEmpty())
		    r+=bi->getNoteOpenDoc();
		r+="</text:p>";
		r+=buildList (bi);  // recursivly add deeper branches
		r+="</text:list-item>\n";
	    }
	    i++;
	    bi=current->getBranchNum(i);
	}
	r+="</text:list>\n";
    }
    return r;
}


void ExportOO::exportPresentation()
{
    QString allPages;

    BranchItem *firstMCO=(BranchItem*)(model->getRootItem()->getFirstBranch());
    if (!firstMCO) 
    {
	QMessageBox::critical (0,QObject::tr("Critical Export Error"),QObject::tr("No objects in map!"));
	return;
    }

    // Insert new content
    // FIXME add extra title in mapinfo for vym 1.13.x
    content.replace ("<!-- INSERT TITLE -->",quotemeta(firstMCO->getHeadingPlain()));
    content.replace ("<!-- INSERT AUTHOR -->",quotemeta(model->getAuthor()));

    QString onePage;
    QString list;
    
    BranchItem *sectionBI;  
    int i=0;
    BranchItem *pagesBI;
    int j=0;

    int mapcenters=model->getRootItem()->branchCount();

    // useSections already has been set in setConfigFile 
    if (mapcenters>1)	
	sectionBI=firstMCO;
    else
	sectionBI=firstMCO->getFirstBranch();

    // Walk sections
    while (sectionBI && !sectionBI->hasHiddenExportParent() )
    {
	if (useSections)
	{
	    // Add page with section title
	    onePage=sectionTemplate;
	    onePage.replace ("<!-- INSERT PAGE HEADING -->", quotemeta(sectionBI->getHeadingPlain() ) );
	    allPages+=onePage;
	    pagesBI=sectionBI->getFirstBranch();
	} else
	{
	    //i=-2; // only use inner loop to 
		    // turn mainbranches into pages
	    //sectionBI=firstMCO;
	    pagesBI=sectionBI;
	}

	j=0;
	while (pagesBI && !pagesBI->hasHiddenExportParent() )
	{
	    // Add page with list of items
	    onePage=pageTemplate;
	    onePage.replace ("<!-- INSERT PAGE HEADING -->", quotemeta (pagesBI->getHeadingPlain() ) );
	    list=buildList (pagesBI);
	    onePage.replace ("<!-- INSERT LIST -->", list);
	    allPages+=onePage;
	    if (pagesBI!=sectionBI)
	    {
		j++;
		pagesBI=((BranchItem*)pagesBI->parent())->getBranchNum(j);
	    } else
		pagesBI=NULL;	// We are already iterating over the sectionBIs
	}
	i++;
	if (mapcenters>1 )
	    sectionBI=model->getRootItem()->getBranchNum (i);
	else
	    sectionBI=firstMCO->getBranchNum (i);
    }
    
    content.replace ("<!-- INSERT PAGES -->",allPages);

    // Write modified content
    QFile f (contentFile);
    if ( !f.open( QIODevice::WriteOnly ) ) 
    {
	QMessageBox::critical (0,QObject::tr("Critical Export Error"),QObject::tr("Could not write %1").arg(contentFile));
	mainWindow->statusMessage(QString(QObject::tr("Export failed.")));
	return;
    }

    QTextStream t( &f );
    t << content;
    f.close();

    // zip tmpdir to destination
    zipDir (tmpDir,outputFile);	
}

bool ExportOO::setConfigFile (const QString &cf)
{
    configFile=cf;
    int i=cf.lastIndexOf ("/");
    if (i>=0) configDir=cf.left(i);
    SimpleSettings set;

    if (!set.readSettings(configFile))
    {
	QMessageBox::critical (0,QObject::tr("Critical Export Error"),QObject::tr("Couldn't read settings from \"%1\"").arg(configFile));
	return false;
    }

    // set paths
    templateDir=configDir+"/"+set.value ("Template");

    QDir d (templateDir);
    if (!d.exists())
    {
	QMessageBox::critical (0,QObject::tr("Critical Export Error"),QObject::tr("Check \"%1\" in\n%2").arg("Template="+set.value ("Template")).arg(configFile));
	return false;

    }

    contentTemplateFile=templateDir+"content-template.xml";
    contentFile=tmpDir.path()+"/content.xml";
    pageTemplateFile=templateDir+"page-template.xml";
    sectionTemplateFile=templateDir+"section-template.xml";

    if (set.value("useSections").contains("yes"))
	useSections=true;

    // Copy template to tmpdir	
    copyDir (templateDir,tmpDir);

    // Read content-template
    if (!loadStringFromDisk (contentTemplateFile,content))
    {
	QMessageBox::critical (0,QObject::tr("Critical Export Error"),QObject::tr("Could not read %1").arg(contentTemplateFile));
	return false;
    }

    // Read page-template
    if (!loadStringFromDisk (pageTemplateFile,pageTemplate))
    {
	QMessageBox::critical (0,QObject::tr("Critical Export Error"),QObject::tr("Could not read %1").arg(pageTemplateFile));
	return false;
    }
    
    // Read section-template
    if (useSections && !loadStringFromDisk (sectionTemplateFile,sectionTemplate))
    {
	QMessageBox::critical (0,QObject::tr("Critical Export Error"),QObject::tr("Could not read %1").arg(sectionTemplateFile));
	return false;
    }
    return true;
}

