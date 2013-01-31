#include "exporthtmldialog.h"	

#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>

#include "file.h"
#include "options.h"
#include "settings.h"
#include "warningdialog.h"


extern Options options;
extern QDir vymBaseDir;
extern Settings settings;
extern bool debug;

ExportHTMLDialog::ExportHTMLDialog(QWidget* parent) : QDialog(parent)
{
    ui.setupUi(this);

    filepath="";
    settingsChanged=false;

    // signals and slots connections
    connect(ui.browseExportDirButton, SIGNAL(pressed()), this, SLOT(browseDirectoryPressed()));
    connect(ui.browseCssSrcButton, SIGNAL(pressed()), this, SLOT(browseCssSrcPressed()));
    connect(ui.browseCssDstButton, SIGNAL(pressed()), this, SLOT(browseCssDstPressed()));
    connect(ui.imageButton, SIGNAL(toggled(bool)), this, SLOT(imageButtonPressed(bool)));
    connect(ui.TOCButton, SIGNAL(toggled(bool)), this, SLOT(TOCButtonPressed(bool)));
    connect(ui.numberingButton, SIGNAL(toggled(bool)), this, SLOT(numberingButtonPressed(bool)));
    connect(ui.taskFlagsButton, SIGNAL(toggled(bool)), this, SLOT(taskFlagsButtonPressed(bool)));
    connect(ui.userFlagsButton, SIGNAL(toggled(bool)), this, SLOT(userFlagsButtonPressed(bool)));
    connect(ui.textColorButton, SIGNAL(toggled(bool)), this, SLOT(textcolorButtonPressed(bool)));
    connect(ui.lineEditDir, SIGNAL(textChanged(const QString&)), this, SLOT(dirChanged()));
    connect(ui.copyCssButton, SIGNAL(pressed()), this, SLOT(copyCssPressed()));
    connect(ui.lineEditCssSrc, SIGNAL(textChanged(const QString&)), this, SLOT(cssSrcChanged()));
    connect(ui.lineEditCssDst, SIGNAL(textChanged(const QString&)), this, SLOT(cssDstChanged()));
    connect(ui.saveSettingsInMapButton, SIGNAL(toggled(bool)), this, SLOT(saveSettingsInMapButtonPressed(bool)));
    connect(ui.lineEditPostScript, SIGNAL(textChanged(const QString&)), this, SLOT(postscriptChanged()));
    connect(ui.browsePostExportButton, SIGNAL(pressed()), this, SLOT(browsePostExportButtonPressed()));
}   

void ExportHTMLDialog::readSettings()
{
    dir=settings.localValue (filepath,"/export/html/exportDir",vymBaseDir.currentPath() ).toString();
    ui.lineEditDir->setText(dir.absolutePath());
    
    useImage=settings.localValue (filepath,"/export/html/useImage","true").toBool();
    ui.imageButton->setChecked(useImage);
	
    useTOC=settings.localValue (filepath,"/export/html/useTOC","true").toBool();
    ui.TOCButton->setChecked(useTOC);
	
    useNumbering=settings.localValue (filepath,"/export/html/useNumbering","true").toBool();
    ui.numberingButton->setChecked(useNumbering);
	
    useTaskFlags=settings.localValue (filepath,"/export/html/useTaskFlags","true").toBool();
    ui.taskFlagsButton->setChecked(useTaskFlags);
	
    useUserFlags=settings.localValue (filepath,"/export/html/useUserFlags","true").toBool();
    ui.userFlagsButton->setChecked(useUserFlags);
	
    useTextColor=settings.localValue (filepath,"/export/html/useTextColor","no").toBool();
    ui.textColorButton->setChecked(useTextColor);
    
/* FIXME-3 this was used in old html export, is not yet in new stylesheet
    useHeading=settings.readValue ("/export/html/useHeading","false").toBool();
    checkBox4_2->setChecked(useHeading);
*/	

    saveSettingsInMap=settings.localValue (filepath,"/export/html/saveSettingsInMap","no").toBool();
    ui.saveSettingsInMapButton->setChecked(saveSettingsInMap);

    //CSS settings
    css_copy=settings.localValue 
        (filepath,"/export/html/copy_css",true).toBool();   
    ui.copyCssButton->setChecked (css_copy);

    QString css_org=vymBaseDir.path() + "/styles/vym.css";
    css_src=settings.localValue 
        (filepath,"/export/html/css_src",css_org).toString();   
    css_dst=settings.localValue 
        (filepath,"/export/html/css_dst",basename(css_org)).toString();   
    
    ui.lineEditCssSrc->setText(css_src);
    ui.lineEditCssDst->setText(css_dst);
    
    postscript=settings.localValue
	(filepath,"/export/html/postscript","").toString();
    ui.lineEditPostScript->setText (postscript);    

    if (!postscript.isEmpty())
    {
	QMessageBox::warning( 0, tr( "Warning" ),tr(
	"The settings saved in the map "
	"would like to run script:\n\n"
	"%1\n\n"
	"Please check, if you really\n"
	"want to allow this in your system!").arg(postscript));
    }
}

void ExportHTMLDialog::setDirectory (const QString &d)
{
    dir.setPath(d);
}

void ExportHTMLDialog::dirChanged()
{
    setDirectory (ui.lineEditDir->text());
    settingsChanged=true;
}

void ExportHTMLDialog::browseDirectoryPressed()
{
    QFileDialog fd( this);
    fd.setFileMode (QFileDialog::DirectoryOnly);
    fd.setWindowTitle (tr("VYM - Export HTML to directory"));
    fd.setModal (true);
    fd.setDirectory (QDir::current());
    fd.show();

    if ( fd.exec() == QDialog::Accepted )
    {
	QDir dir=fd.directory();
	ui.lineEditDir->setText (dir.path() );
	settingsChanged=true;
    }
}

void ExportHTMLDialog::imageButtonPressed(bool b)
{
    useImage=b;
    settingsChanged=true;
}

void ExportHTMLDialog::TOCButtonPressed(bool b)
{
    useTOC=b;
    settingsChanged=true;
}

void ExportHTMLDialog::numberingButtonPressed(bool b)
{
    useNumbering=b;
    settingsChanged=true;
}

void ExportHTMLDialog::taskFlagsButtonPressed(bool b)
{
    useTaskFlags=b;
    settingsChanged=true;
}

void ExportHTMLDialog::userFlagsButtonPressed(bool b)
{
    useUserFlags=b;
    settingsChanged=true;
}

void ExportHTMLDialog::textcolorButtonPressed(bool b)
{
    useTextColor=b; 
    settingsChanged=true;
}

void ExportHTMLDialog::saveSettingsInMapButtonPressed(bool b)
{
    saveSettingsInMap=b;    
    settingsChanged=true;
}

void ExportHTMLDialog::warningsButtonPressed(bool b)
{
    showWarnings=b;
    settingsChanged=true;
}


void ExportHTMLDialog::outputButtonPressed(bool b)
{
    showOutput=b;
    settingsChanged=true;
}

void ExportHTMLDialog::cssSrcChanged()
{
    css_src=ui.lineEditCssSrc->text();
    settingsChanged=true;
}

void ExportHTMLDialog::cssDstChanged()
{
    css_dst=ui.lineEditCssDst->text();
    settingsChanged=true;
}

QString ExportHTMLDialog::getCssSrc()
{
    if (css_copy)
        return css_src;
    else
        return QString();
}

QString ExportHTMLDialog::getCssDst()
{
    return css_dst;
}

void ExportHTMLDialog::copyCssPressed()
{
    css_copy=ui.imageButton->isChecked();
    settingsChanged=true;
}

void ExportHTMLDialog::browseCssSrcPressed()
{
    QFileDialog fd( this);
    fd.setModal (true);
    fd.setFilter ("Cascading Stylesheet (*.css)");
    fd.setDirectory (QDir::current());
    fd.show();

    if ( fd.exec() == QDialog::Accepted )
    {
	if (!fd.selectedFiles().isEmpty())
	{
	    css_src=fd.selectedFiles().first();
	    ui.lineEditCssSrc->setText (css_src );
	    settingsChanged=true;
	}
    }
}

void ExportHTMLDialog::browseCssDstPressed()
{
    QFileDialog fd( this);
    fd.setModal (true);
    fd.setFilter ("Cascading Stylesheet (*.css)");
    fd.setDirectory (QDir::current());
    fd.show();

    if ( fd.exec() == QDialog::Accepted )
    {
	if (!fd.selectedFiles().isEmpty())
	{
	    css_dst=fd.selectedFiles().first();
	    ui.lineEditCssDst->setText (css_dst );
	    settingsChanged=true;
	}
    }
}

void ExportHTMLDialog::postscriptChanged()
{
    postscript=ui.lineEditPostScript->text();
    settingsChanged=true;
}

void ExportHTMLDialog::browsePostExportButtonPressed()
{
    QFileDialog fd( this);
    fd.setModal (true);
    fd.setFilter ("Scripts (*.sh *.pl *.py *.php)");
    fd.setDirectory (QDir::current());
    fd.show();

    if ( fd.exec() == QDialog::Accepted )
    {
	if (!fd.selectedFiles().isEmpty())
	{
	    postscript=fd.selectedFiles().first();
	    ui.lineEditPostScript->setText (postscript );
	    settingsChanged=true;
       } 
    }
}

void ExportHTMLDialog::saveSettings ()
{
    // Save options to settings file 
    // (but don't save at destructor, which
    // is called for "cancel", too)
    if (!saveSettingsInMap)
	settings.clearLocal(filepath,"/export/html");
    else    
    {
	settings.setLocalValue (filepath,"/export/html/exportDir",dir.absolutePath());
	settings.setLocalValue (filepath,"/export/html/saveSettingsInMap","yes");
        settings.setLocalValue (filepath,"/export/html/postscript",postscript);
        settings.setLocalValue (filepath,"/export/html/useImage",useImage);
        settings.setLocalValue (filepath,"/export/html/useTOC",useTOC);
        settings.setLocalValue (filepath,"/export/html/useNumbering",useNumbering);
        settings.setLocalValue (filepath,"/export/html/useTaskFlags",useTaskFlags);
        settings.setLocalValue (filepath,"/export/html/useUserFlags",useUserFlags);
        settings.setLocalValue (filepath,"/export/html/useTextColor",useTextColor);
        settings.setLocalValue (filepath,"/export/html/css_copy",css_copy);	
        settings.setLocalValue (filepath,"/export/html/css_src",css_src);	
        settings.setLocalValue (filepath,"/export/html/css_dst",css_dst);	
        settings.setValue ("/export/html/showWarnings",showWarnings);
        settings.setValue ("/export/html/showOutput",showOutput);
    }
}

void ExportHTMLDialog::setFilePath(const QString &s)
{
    filepath=s;
}

void ExportHTMLDialog::setMapName(const QString &s)
{
    mapname=s;
}

QDir ExportHTMLDialog::getDir()
{
    return dir;
}

bool ExportHTMLDialog::warnings()
{
    return showWarnings;
}

bool ExportHTMLDialog::hasChanged()
{
    return settingsChanged;
}


