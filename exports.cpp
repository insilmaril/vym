#include "exports.h"

#include <cstdlib>

#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>

#include "branchitem.h"
#include "file.h"
#include "linkablemapobj.h"
#include "misc.h"
#include "mainwindow.h"
#include "warningdialog.h"
#include "xsltproc.h"
#include "vymprocess.h"


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
    lastExportDir=QDir(dirPath);
}

void ExportBase::init()
{
    indentPerDepth="  ";
    exportName="unnamed";
    bool ok;
    tmpDir.setPath (makeTmpDir(ok,"vym-export"));
    if (!tmpDir.exists() || !ok)
	QMessageBox::critical( 0, QObject::tr( "Error" ),
		       QObject::tr("Couldn't access temporary directory\n"));
    cancelFlag=false;		       
    defaultDirPath=lastExportDir.absolutePath();
    dirPath=defaultDirPath;
}

void ExportBase::setDirPath (const QString &s)
{
    if (!s.isEmpty()) 
        dirPath=s;
    // Otherwise lastExportDir is used, which defaults to current dir
}

QString ExportBase::getDirPath()
{
    return dirPath;
}

void ExportBase::setFilePath (const QString &s)
{
    if(!s.isEmpty())
    {
        filePath=s;
        if (!filePath.contains("/"))
            // Absolute path
            filePath=lastExportDir.absolutePath() + "/" + filePath;
    } 
}

QString ExportBase::getFilePath ()
{
    if (!filePath.isEmpty())
        return filePath;
    else
        return dirPath + "/" + model->getMapName() + extension;
}

QString ExportBase::getMapName ()
{
    QString fn=basename(filePath);
    return fn.left(fn.lastIndexOf("."));
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

bool ExportBase::execDialog()
{
    QString fn=QFileDialog::getSaveFileName( 
	NULL,
	caption,
	dirPath, 
	filter,
	NULL,
	QFileDialog::DontConfirmOverwrite);

    if (!fn.isEmpty() )
    {
	if (QFile (fn).exists() ) 
	{
	    WarningDialog dia;
	    dia.showCancelButton (true);
	    dia.setCaption(QObject::tr("Warning: Overwriting file"));
	    dia.setText(QObject::tr("Exporting to %1 will overwrite the existing file:\n%2").arg(exportName).arg(fn));
	    dia.setShowAgainName("/exports/overwrite/" + exportName);
	    if (!dia.exec()==QDialog::Accepted)
	    {
		cancelFlag=true;
		return false;
	    }
	}
	dirPath=fn.left(fn.lastIndexOf ("/"));
	filePath=fn;
	if (model) model->setChanged();
	return true;
    }
    return false;
}

bool ExportBase::canceled()
{
    return cancelFlag;
}

void ExportBase::completeExport(QString args)
{
    if (args.isEmpty()) args=QString("\"%1\"").arg(filePath);

    QString cmd=QString("export%1(%2)").arg(exportName).arg(args);
    settings.setLocalValue ( model->getFilePath(), "/export/last/exportPath",filePath);
    settings.setLocalValue ( model->getFilePath(), "/export/last/command",cmd);
    settings.setLocalValue ( model->getFilePath(), "/export/last/description","CSV");
    mainWindow->statusMessage(QString("Exported as %1: %2").arg(exportName).arg(filePath));
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

QString ExportBase::indent (const int &n, bool useBullet)
{
    QString s;
    for (int i=0; i<n; i++) s += indentPerDepth;
    if (useBullet && s.length() >= 2 && bulletPoints.count() > n) 
        s.replace( s.length() - 2, 1, bulletPoints.at(n) );
    return s;
}

////////////////////////////////////////////////////////////////////////
ExportAO::ExportAO()
{
    exportName="AO";
    filter="TXT (*.txt);;All (* *.*)";
    caption=vymName+ " -" +QObject::tr("Export as ASCII")+" "+QObject::tr("(still experimental)");
    indentPerDepth="   ";
    bulletPoints.clear();
    for (int i=0; i<10; i++) 
        bulletPoints << "-";
}

void ExportAO::doExport()   
{
    QFile file (filePath);
    if ( !file.open( QIODevice::WriteOnly ) )
    {
	qWarning()<<"ExportAO::doExport couldn't open " + filePath;//FIXME-3 missing GUI warning
	return;
    }

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

    model->nextBranch (cur,prev);
    while (cur) 
    {
        QString line;
        QString colString="";
        QString noColString;
        QString statusString ="";
        QColor col;

	if (cur->getType()==TreeItem::Branch || cur->getType()==TreeItem::MapCenter)
	{
	    // Make indentstring
	    curIndent=indent(cur->depth()-4,true);

	    if (!cur->hasHiddenExportParent() )
	    {
		col=cur->getHeadingColor();
		if (col==QColor (255,0,0))
		    colString="[R] ";
		else if (col==QColor (217,81,0))
		    colString="[O] ";
		else if (col==QColor (0,85,0))
		    colString="[G] ";
                else if (cur->depth()==4)
		    colString=" *  ";
                else
		    colString="    ";

                noColString=QString(" ").repeated(colString.length() );

		dashIndent="";	
		switch (cur->depth())
		{
		    case 0: break;  // Mapcenter (Ignored)
		    case 1: break;  // Mainbranch "Archive" (Ignored)
                    case 2: // Title: "Current week number..."
			ts << "\n";
			ts << underline ( cur->getHeadingPlain(), QString("=") );
			ts << "\n";
			break;
                    case 3: // Headings: "Achievement", "Bonus", "Objective", ...
			ts << "\n";
			ts << underline ( cur->getHeadingPlain(), "-");
			ts << "\n";
			break;
		    default:	// depth 4 and higher are the items we need to know
			Task *task=cur->getTask();
			if (task)
			{
			    // Task status overrides other flags
			    switch ( task->getStatus() )
			    {
				case Task::NotStarted:
				    statusString="[NOT STARTED]";
				    break;
				case Task::WIP:   
                                    statusString="[WIP]";
				    break;
				case Task::Finished:
				    statusString="[DONE]";
				    break;
			    }
			} else
			{
			    if (cur->hasActiveStandardFlag ("hook-green") )
                                statusString="[DONE]";
			    else if (cur->hasActiveStandardFlag ("wip"))
                                statusString="[WIP]";
			    else if (cur->hasActiveStandardFlag ("cross-red"))
                                statusString="[NOT STARTED]";
			}

                        line += colString;
                        line += curIndent;
                        if (cur->depth() >3)
                            line += cur->getHeadingPlain();

                        // Pad line width before status
                        i = 80 - line.length() - statusString.length() -1;
                        for (int j=0; j<i; j++) line += " ";
                        line += " "  + statusString + "\n";

                        ts << line;
                        
                        // If necessary, write URL
                        if (!cur->getURL().isEmpty())
                             ts << noColString << indent(cur->depth()-4, false) + cur->getURL() + "\n";

                        // If necessary, write note
                        if (!cur->getNoteObj().isEmpty())
                        {
                            curIndent = noColString + indent(cur->depth()-4,false) + "| ";
                            s=cur->getNoteASCII( curIndent, 80);
                            ts << s + "\n";
                        }
			break;
		}
	    }
	}
	model->nextBranch(cur,prev);
    }
    file.close();
    completeExport();
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
    exportName="ASCII";
    filter="TXT (*.txt);;All (* *.*)";
    caption=vymName+ " -" +QObject::tr("Export as ASCII");
}

QString ExportASCII::noteToASCII( const NoteObj &noteobj, const QString &indent, const int &width)  //FIXME-3 use width
{
    QString note = noteobj.getNote();
    if (note.isEmpty()) return note;

    QRegExp rx;
    rx.setMinimal(true);

    if (!noteobj.isRichText()) 
    {
        if ( noteobj.getFontHint() != "fixed")
        {
            // Wordwrap

            QString newnote;
            QString curline;
            uint n=0;
            while ( n < note.length() )
            {
                curline = curline + note.at(n);
                if ( note.at(n) == '\n' )
                {
                    newnote = newnote + curline ;
                    curline = "";
                }

                if (curline.length() > width)
                {
                    // Try to find last previous whitespace in curline
                    uint i = curline.length() - 1;
                    while ( i> 0 )
                    {
                        if ( curline.at(i) == ' ' )
                        {
                            newnote = newnote + curline.left(i) + '\n';
                            curline = curline.right( curline.length() - i - 1 );
                            break;
                        }
                        i--;
                        if ( i == 0 ) 
                        {
                            // Cannot break this line into smaller parts
                            newnote = newnote + curline;
                            curline = "";
                        }
                    }
                }
                n++;
            }
            note = newnote + curline;
        }
        
        // Indent lines
        rx.setPattern("^");
        note = note.replace (rx,indent);
        rx.setPattern("\n");
        note = note.replace (rx, "\n" + indent) + "\n";

        return note;
    }

    // Remove all <style...> ...</style>
    rx.setPattern("<style.*>.*</style>");
    note.replace (rx,"");

    // convert all "<br*>" to "\n"
    rx.setPattern ("<br.*>");
    note.replace (rx,"\n");

    // convert all "</p>" to "\n"
    rx.setPattern ("</p>");
    note.replace (rx,"\n");
    
    // remove all remaining tags 
    rx.setPattern ("<.*>");
    note.replace (rx,"");

    // If string starts with \n now, remove it.
    // It would be wrong in an OOo export for example
    while ( note.at(0) == '\n' ) note.remove (0,1);
    
    // convert "&", "<" and ">"
    rx.setPattern ("&gt;");
    note.replace (rx,">");
    rx.setPattern ("&lt;");
    note.replace (rx,"<");
    rx.setPattern ("&amp;");
    note.replace (rx,"&");
    rx.setPattern ("&quot;");
    note.replace (rx,"\"");

    // Indent everything
    rx.setPattern ("^\n");
    note.replace (rx,indent);
    note = indent + note;   // Don't forget first line

    note = indent+"\n" + note + indent + "\n\n";
    return note;
}

void ExportASCII::doExport()	
{
    QFile file (filePath);
    if ( !file.open( QIODevice::WriteOnly ) )
    {
	qWarning ()<<"ExportASCII::doExport couldn't open "+filePath;
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

    int lastDepth=0;

    model->nextBranch (cur,prev);
    while (cur) 
    {
	if (cur->getType()==TreeItem::Branch || cur->getType()==TreeItem::MapCenter)
	{
            // Insert newline after previous list
            if ( cur->depth() < lastDepth ) ts << "\n";

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

		// If necessary, write vymlink
		if (!cur->getVymLink().isEmpty())
		    ts << (curIndent + dashIndent + cur->getVymLink()) +" (vym mindmap)\n";

		// If necessary, write note
		if (!cur->getNoteObj().isEmpty())
		{
                    // curIndent +="  | ";
                    // Only indent for bullet points
                    if (cur->depth() > 2) curIndent +="  ";
		    ts << '\n' +  noteToASCII( cur->getNoteObj(), curIndent, 80) ;
		}
                lastDepth = cur->depth();
	    }
	}
	model->nextBranch(cur,prev);
    }
    file.close();
    completeExport();
}

QString ExportASCII::underline (const QString &text, const QString &line)
{
    QString r=text + "\n";
    for (int j=0;j<text.length();j++) r+=line;
    return r;
}


////////////////////////////////////////////////////////////////////////
ExportCSV::ExportCSV() 
{
    exportName="CSV";
    filter="CSV (*.csv);;All (* *.*)";
    caption=vymName+ " -" +QObject::tr("Export as CSV");
}

void ExportCSV::doExport()
{
    QFile file (filePath);
    if ( !file.open( QIODevice::WriteOnly ) )
    {
	qWarning ()<<"ExportBase::exportXML  couldn't open "+filePath;
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
    model->nextBranch (cur,prev);
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
	
	model->nextBranch(cur,prev);
	curIndent="";
    }
    file.close();
    completeExport();
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
    exportName="HTML";
    extension=".html";
    frameURLs=true;
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
        QString s=QString("<span class='vym-branch-%1' %2 id='%3'>")
            .arg(current->depth())
            .arg(col)
            .arg(id);
        QString url=current->getURL();	
        QString heading=quotemeta(current->getHeadingPlain());	

        // Task flags
        QString taskFlags;
        if (dia.useTaskFlags)
        {
            Task *task=current->getTask();
            if (task)
                taskFlags+=QString("<img src=\"flags/flag-%1.png\">").arg(task->getIconString());
        }

        // User flags
        QString userFlags;
        if (dia.useUserFlags)
        {
            foreach (QString flag, current->activeStandardFlagNames())
                userFlags+=QString("<img src=\"flags/flag-%1.png\">").arg(flag);
        }

        // URL
        if (!url.isEmpty())
        {
            s+=QString ("<a href=\"%1\"><img src=\"flags/flag-url-16x16.png\">%2</a>")
                .arg(url)
                .arg(taskFlags + heading + userFlags);

            QRectF fbox=current->getBBoxURLFlag ();
            if (vis)	
                imageMap+=QString("  <area shape='rect' coords='%1,%2,%3,%4' href='%5'>\n")
                .arg(fbox.left()-offset.x())
                .arg(fbox.top()-offset.y())
                .arg(fbox.right()-offset.x())
                .arg(fbox.bottom()-offset.y())
                .arg(url);
        } else
            s+=taskFlags + heading + userFlags;

        s+="</span>";

        // Create imagemap
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
                if (current->getNoteObj().getFontHint() == "fixed")
                    n.replace(re,"<p class=\"vym-fixed-note-paragraph\">");
                else
                    n.replace(re,"<p class=\"vym-note-paragraph\">");

                re.setPattern("</?html>");
                n.replace(re,"");

                re.setPattern("</?head.*>");
                n.replace(re,"");

                re.setPattern("</?body.*>");
                n.replace(re,"");

                re.setPattern("</?meta.*>");
                n.replace(re,"");

                re.setPattern("<style.*>.*</style>");
                n.replace(re,"");

                //re.setPattern("<!DOCTYPE.*>");
                //n.replace(re,"");
            }
            else
            {
                n=current->getNoteASCII().replace ("<","&lt;").replace (">","&gt;");
                n.replace("\n","<br/>");
                if (current->getNoteObj().getFontHint()=="fixed")
                n="<pre>"+n+"</pre>";
            }
            s+="\n<table class=\"vym-note\"><tr><td class=\"vym-note-flag\">\n<td>\n"+n+"\n</td></tr></table>\n";
        }   
        return s;
    } 
    return QString();
}

QString ExportHTML::buildList (BranchItem *current)
{
    QString r;

    uint i=0;
    uint visChilds=0;

    BranchItem *bi=current->getFirstBranch();

    QString ind="\n" + indent(current->depth() + 1, false);

    QString sectionBegin;
    QString sectionEnd;
    QString itemBegin;
    QString itemEnd;

    switch (current->depth() + 1)
    {
        case 0: 
            sectionBegin="";
            sectionEnd="";
            itemBegin="<h1>";
            itemEnd="</h1>";
            break;
        case 1: 
            sectionBegin="";
            sectionEnd="";
            itemBegin="<h2>";
            itemEnd="</h2>";
            break;
        default:
            sectionBegin="<ul " + QString("class=\"vym-list-ul-%1\"").arg(current->depth() + 1)  +">";
            sectionEnd="</ul>";
            itemBegin="  <li>";
            itemEnd="  </li>";
            break;
    }
    
    if (bi && !bi->hasHiddenExportParent() && !bi->isHidden() )	
    {
        r+=ind + sectionBegin; 
	while (bi)
	{
	    if (!bi->hasHiddenExportParent() && !bi->isHidden()) 
	    {
                visChilds++;
		r+=ind + itemBegin;
		r+=getBranchText (bi);

                if (itemBegin.startsWith("<h") )
                    r+=itemEnd + buildList (bi);
                else
                    r+=buildList (bi) + itemEnd;	
	    }
	    i++;
	    bi=current->getBranchNum(i);
	}
        r+=ind + sectionEnd;
    }

    return r;
}

QString ExportHTML::createTOC()
{
    QString toc;
    QString number;
    toc += "<table class=\"vym-toc\">\n";
    toc += "<tr><td class=\"vym-toc-title\">\n"; 
    toc += QObject::tr("Contents:","Used in HTML export");
    toc += "\n";
    toc += "</td></tr>\n"; 
    toc += "<tr><td>\n"; 
    BranchItem *cur=NULL;
    BranchItem *prev=NULL;
    model->nextBranch(cur,prev);
    while (cur) 
    {
	if (!cur->hasHiddenExportParent() && !cur->hasScrolledParent() )
	{
            if (dia.useNumbering) number=getSectionString(cur);
            toc +=QString("<div class=\"vym-toc-branch-%1\">").arg(cur->depth());
	    toc +=QString("<a href=\"#%1\"> %2 %3</a></br>\n")
                .arg(model->getSelectString(cur))
                .arg(number)
                .arg(cur->getHeadingPlain());
            toc +="</div>";
	}
	model->nextBranch(cur,prev);
    }
    toc += "</td></tr>\n"; 
    toc += "</table>\n"; 
    return toc;
}

void ExportHTML::doExport(bool useDialog) 
{
    // Setup dialog and read settings
    dia.setMapName (model->getMapName());
    dia.setFilePath (model->getFilePath());
    dia.readSettings();

    if (dirPath != defaultDirPath)
        dia.setDirectory(dirPath); 

    if (useDialog)
    {
	if (dia.exec()!=QDialog::Accepted) return;
	model->setChanged();
    }

    // Check, if warnings should be used before overwriting
    // the output directory
    if (dia.getDir().exists() && dia.getDir().count()>0)
    {
	WarningDialog warn;
	warn.showCancelButton (true);
	warn.setText(QString(
	    "The directory %1 is not empty.\n"
	    "Do you risk to overwrite some of its contents?").arg(dia.getDir().absolutePath() ));
	warn.setCaption("Warning: Directory not empty");
	warn.setShowAgainName("mainwindow/export-XML-overwrite-dir");

	if (warn.exec()!=QDialog::Accepted) 
	{
	    mainWindow->statusMessage(QString(QObject::tr("Export aborted.")));
	    return;
	}
    }

    dirPath=dia.getDir().absolutePath(); 
    filePath=getFilePath();
    
    // Copy CSS file 
    if (dia.css_copy)
    {
        cssSrc=dia.getCssSrc();
        cssDst=dia.getDir().absolutePath() + "/" + basename(dia.getCssDst());
        if (cssSrc.isEmpty() )
        {
            QMessageBox::critical( 0,
            QObject:: tr( "Critical" ),
            QObject::tr("Could not find stylesheet %1").arg(cssSrc));
            return;
        }
        QFile src(cssSrc);
        QFile dst(cssDst);
        if (dst.exists() ) dst.remove();

        if (!src.copy(cssDst))
        {
            QMessageBox::critical (0,
                    QObject::tr( "Error","ExportHTML" ),
                    QObject::tr("Could not copy\n%1 to\n%2","ExportHTML").arg(cssSrc).arg(cssDst));
            return;
        }
    }

    // Copy flags
    QDir flagsDst(dia.getDir().absolutePath() + "/flags");
    if (!flagsDst.exists())
    {
        if (!dia.getDir().mkdir("flags"))
        {
            QMessageBox::critical( 0,
            QObject:: tr( "Critical" ),
            QObject::tr("Trying to create directory for flags:")+"\n\n"+
            QObject::tr("Could not create %1").arg(flagsDst.absolutePath()));
            return;
        }
    }   

    QDir flagsSrc(flagsPath);   // FIXME-1 don't use flagsPath anymore, copy required flags directly from memory
    if (!copyDir(flagsSrc,flagsDst,true))
    {
        QMessageBox::critical( 0,
                QObject:: tr( "Critical" ),
                QObject::tr("Could not copy %1 to %2").arg(flagsSrc.absolutePath()).arg(flagsDst.absolutePath()));
        return;
    }

    // Open file for writing
    QFile file (filePath);
    if ( !file.open( QIODevice::WriteOnly ) ) 
    {
	QMessageBox::critical (0,
	    QObject::tr("Critical Export Error"),
	    QObject::tr("Trying to save HTML file:")+"\n\n"+
	    QObject::tr("Could not write %1").arg(filePath));
	mainWindow->statusMessage(QString(QObject::tr("Export failed.")));
	return;
    }
    QTextStream ts( &file );	// use LANG decoding here...
    //FIXME-4 ts.setEncoding (QTextStream::UnicodeUTF8); // Force UTF8

    // Hide stuff during export
    model->setExportMode (true);

    // Write header
    ts<<"<html>";
    ts<<"\n<meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\"> ";
    ts<<"\n<meta name=\"generator=\" content=\" vym - view your mind - " + vymHome + "\">"; 
    ts<<"\n<meta name=\"author\" content=\"" + quotemeta(model->getAuthor()) + "\"> ";
    ts<<"\n<meta name=\"description\" content=\"" + quotemeta(model->getComment()) + "\"> ";
    ts<<"\n<link rel='stylesheet' id='css.stylesheet' href='"<<basename(cssDst)<<"' />\n";
    QString title=model->getTitle();
    if (title.isEmpty()) title=model->getMapName();
    ts<<"\n<head><title>" + quotemeta(title) + "</title></head>";
    ts<<"\n<body>\n";

    // Include image 
    // (be careful: this resets Export mode, so call before exporting branches)
    if (dia.useImage)
    {
	ts<<"<center><img src=\""<<getMapName()<<".png\" usemap='#imagemap'></center>\n";
	offset=model->exportImage (dirPath+"/"+getMapName()+".png",false,"PNG");
    }

    // Include table of contents
    if (dia.useTOC) ts << createTOC();

    // Main loop over all mapcenters
    ts << buildList(model->getRootItem()) << "\n";

    // Imagemap
    ts<<"<map name='imagemap'>\n"+imageMap+"</map>\n";

    // Write footer 
    ts<<"<hr/>\n";
    ts<<"<table class=\"vym-footer\">   \n\
      <tr> \n\
        <td class=\"vym-footerL\">"+filePath+"</td> \n\
        <td class=\"vym-footerC\">"+model->getDate()+"</td> \n\
        <td class=\"vym-footerR\"> <a href='" + vymHome + "'>vym "+vymVersion+"</a></td> \n\
      </tr> \n \
    </table>\n";
    ts<<"</body></html>";
    file.close();

    if (!dia.postscript.isEmpty()) 
    {
        VymProcess p;
	p.runScript (dia.postscript,dirPath + "/" + filePath);
    }

    completeExport();
    QString cmd="exportHTML";
    settings.setLocalValue ( model->getFilePath(), "/export/last/command",QString("exportHTML(\"%1\",\"%2\")")
            .arg(dirPath)
            .arg(filePath)
            );
    settings.setLocalValue ( model->getFilePath(), "/export/last/description","HTML");
    settings.setLocalValue ( model->getFilePath(), "/export/last/exportPath",filePath); 
    mainWindow->statusMessage(cmd + ": " + filePath);

    dia.saveSettings();
    model->setExportMode (false);
}

////////////////////////////////////////////////////////////////////////
void ExportTaskjuggler::doExport() 
{
    model->exportXML(tmpDir.path(),false);

    XSLTProc p;
    p.setInputFile (tmpDir.path()+"/"+model->getMapName()+".xml");
    p.setOutputFile (filePath);
    p.setXSLFile (vymBaseDir.path()+"/styles/vym2taskjuggler.xsl");
    p.process();
}

////////////////////////////////////////////////////////////////////////
ExportOrgMode::ExportOrgMode() 
{
    exportName="OrgMode";
    filter="org-mode (*.org);;All (* *.*)";
}

void ExportOrgMode::doExport() 
{
    // Exports a map to an org-mode file.  
    // This file needs to be read 
    // by EMACS into an org mode buffer
    QFile file (filePath);
    if ( !file.open( QIODevice::WriteOnly ) ) 
    {
	QMessageBox::critical (0,QObject::tr("Critical Export Error"),QObject::tr("Could not write %1").arg(filePath));
	mainWindow->statusMessage(QString(QObject::tr("Export failed.")));
	return;
    }
    QTextStream ts( &file );  // use LANG decoding here...
    //FIXME-4 ts.setEncoding (QTextStream::UnicodeUTF8); // Force UTF8

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
	model->nextBranch(cur,prev);
    }
    file.close();

    QString cmd=QString("exportOrgMode(\"%1\")").arg(filePath);
    settings.setLocalValue ( model->getFilePath(), "/export/last/command",cmd);
    settings.setLocalValue ( model->getFilePath(), "/export/last/description","Orgmode");
    mainWindow->statusMessage(cmd);
}

////////////////////////////////////////////////////////////////////////
ExportLaTeX::ExportLaTeX()
{
    exportName="LaTeX";
    filter="LaTeX files (*.tex);;All (* *.*)";

    // Note: key in hash on left side is the regular expression, which 
    // will be replaced by string on right side
    // E.g. a literal $ will be replaced by \$
    esc["\\$"]="\\$";
    esc["\\^"]="\\^";
    esc["%"]="\\%";
    esc["&"]="\\&";
    esc["~"]="\\~";
    esc["_"]="\\_";
    esc["\\\\"]="\\";
    esc["\\{"]="\\{";
    esc["\\}"]="\\}";
}

QString ExportLaTeX::escapeLaTeX(const QString &s)
{
    QString r=s;

    QRegExp rx;
    rx.setMinimal(true);

    foreach (QString p,esc.keys() )
    {
	//qDebug()<<"Replacing "<<p<<" with "<<esc[p];    // FIXME-3
	rx.setPattern (p);
	r.replace (rx, esc[p] );
    }	
    return r;
}

void ExportLaTeX::doExport()	
{
    // Exports a map to a LaTex file.  
    // This file needs to be included 
    // or inported into a LaTex document
    // it will not add a preamble, or anything 
    // that makes a full LaTex document.
  QFile file (filePath);
  if ( !file.open( QIODevice::WriteOnly ) ) {
    QMessageBox::critical (
	0,
	QObject::tr("Critical Export Error"),
	QObject::tr("Could not write %1").arg(filePath));
	mainWindow->statusMessage(QString(QObject::tr("Export failed.")));
    return;
  }
  QTextStream ts( &file );  // use LANG decoding here...
  //FIXME-4 ts.setEncoding (QTextStream::UnicodeUTF8); // Force UTF8
  
  // Read default section names
  QStringList sectionNames;
  sectionNames << ""
      << "chapter"
      << "section"
      << "subsection"
      << "subsubsection"
      << "paragraph";

  for (int i=0; i<6; i++)
    sectionNames.replace(i,settings.value(
	QString("/export/latex/sectionName-%1").arg(i),sectionNames.at(i)).toString() );

  // Main loop over all branches
  QString s;
  BranchItem *cur=NULL;
  BranchItem *prev=NULL;
  model->nextBranch(cur,prev);
  while (cur) 
  {
    if (!cur->hasHiddenExportParent() )
    {
	int d=cur->depth();
	s=escapeLaTeX (cur->getHeadingPlain() );
	if ( sectionNames.at(d).isEmpty() || d>=sectionNames.count() )
	    ts << s << endl;
	else    
	    ts << endl
	       << "\\" 
	       << sectionNames.at(d) 
	       << "{"
	       << s
	       << "}"
	       << endl;

	// If necessary, write note
	if (!cur->getNoteObj().isEmpty()) {
	  ts << (cur->getNoteASCII());
	  ts << endl;
	}
    }
    model->nextBranch(cur,prev);
   }
    
    file.close();
    QString cmd=QString("exportLaTeX(\"%1\")").arg(filePath);
    settings.setLocalValue ( model->getFilePath(), "/export/last/exportPath",filePath);
    settings.setLocalValue ( model->getFilePath(), "/export/last/command",cmd);
    settings.setLocalValue ( model->getFilePath(), "/export/last/description","LaTeX");
    mainWindow->statusMessage(cmd + ": " + filePath);
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
    zipDir (tmpDir,filePath);	

    QString cmd="exportImpres";
    settings.setLocalValue (model->getFilePath(),"/export/last/exportPath",filePath);
    settings.setLocalValue ( model->getFilePath(), "/export/last/command","exportImpress");
    settings.setLocalValue ( model->getFilePath(), "/export/last/configFile",configFile);
    settings.setLocalValue ( model->getFilePath(), "/export/last/description","OpenOffice.org Impress");
    mainWindow->statusMessage(cmd + ": " + filePath);
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

