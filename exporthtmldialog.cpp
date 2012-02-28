#include "exporthtmldialog.h"	

#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>

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
    connect(ui.browseCSSButton, SIGNAL(pressed()), this, SLOT(browseCSSPressed()));
    connect(ui.imageButton, SIGNAL(toggled(bool)), this, SLOT(imageButtonPressed(bool)));
    connect(ui.textColorButton, SIGNAL(toggled(bool)), this, SLOT(textcolorButtonPressed(bool)));
    connect(ui.lineEditDir, SIGNAL(textChanged(const QString&)), this, SLOT(dirChanged()));
    connect(ui.lineEditCSS, SIGNAL(textChanged(const QString&)), this, SLOT(cssChanged()));
    connect(ui.saveSettingsInMapButton, SIGNAL(toggled(bool)), this, SLOT(saveSettingsInMapButtonPressed(bool)));
    connect(ui.lineEditPostScript, SIGNAL(textChanged(const QString&)), this, SLOT(postscriptChanged()));
    connect(ui.browsePostExportButton, SIGNAL(pressed()), this, SLOT(browsePostExportButtonPressed()));
}   


void ExportHTMLDialog::readSettings()
{

    dir=settings.localValue (filepath,"/export/html/exportDir",vymBaseDir.currentPath() ).toString();
    ui.lineEditDir->setText(dir);
    
    if ( settings.localValue (filepath,"/export/html/useImage","yes")=="yes")
	useImage=true;
    else    
	useImage=false;
    ui.imageButton->setChecked(useImage);
	
    if ( settings.localValue (filepath,"/export/html/useTextColor","no")=="yes")
	useTextColor=true;
    else    
	useTextColor=false;
    ui.textColorButton->setChecked(useTextColor);
    
/* TODO this was used in old html export, is not yet in new stylesheet
    if ( settings.readValue ("/export/html/useHeading","no")=="yes")
	useHeading=true;
    else    
	useHeading=false;
    checkBox4_2->setChecked(useHeading);
*/	

    if ( settings.localValue (filepath,"/export/html/saveSettingsInMap","no")=="yes")
	saveSettingsInMap=true;
    else    
	saveSettingsInMap=false;
    ui.saveSettingsInMapButton->setChecked(saveSettingsInMap);

/*
    if ( settings.value ("/export/html/showOutput","no").toBool())
	showOutput=true;
    else    
	showOutput=false;
    ui.outputButton->setChecked(showOutput);
    */

    // For testing better use local styles
    const QString defcss(vymBaseDir.path() + "/styles/vym.css");
    if (options.isOn ("local"))
    {
	css=defcss;
    } else
    {
	css=settings.localValue 
	    (filepath,"/export/html/css",defcss).toString();   
    }
    ui.lineEditCSS->setText(css);
    
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
    dir=d;
    if (dir.right(1)!="/") dir+="/";
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

void ExportHTMLDialog::cssChanged()
{
    css=ui.lineEditCSS->text();
    settingsChanged=true;
}

QString ExportHTMLDialog::getCSSPath()
{
    return css;
}

void ExportHTMLDialog::browseCSSPressed()
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
	    css=fd.selectedFiles().first();
	    ui.lineEditCSS->setText (css );
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
    settings.setLocalValue (filepath,"/export/html/exportDir",dir);
    settings.setLocalValue (filepath,"/export/html/postscript",postscript);

    if (useImage)
	settings.setLocalValue (filepath,"/export/html/useImage","yes");
    else
	settings.setLocalValue (filepath,"/export/html/useImage","no");	
    
  if (useTextColor)
	settings.setLocalValue (filepath,"/export/html/useTextColor","yes");
    else
	settings.setLocalValue (filepath,"/export/html/useTextColor","no"); 
    
   if (showWarnings)
	settings.setValue ("/export/html/showWarnings","yes");
    else
	settings.setValue ("/export/html/showWarnings","no");	
	    
    if (showOutput)
	settings.setValue ("/export/html/showOutput","yes");
    else
	settings.setValue ("/export/html/showOutput","no");	

    QString ipath;  
    ipath=vymBaseDir.path()+"/flags/flag-url-16x16.png";
    if (!options.isOn ("local"))
    {
	settings.setLocalValue 
	    (filepath,"/export/html/css",css);	
    }

    if (!saveSettingsInMap)
	settings.clearLocal("/export/html");
    else    
	settings.setLocalValue 
	    (filepath,"/export/html/saveSettingsInMap","yes");

}

void ExportHTMLDialog::setFilePath(const QString &s)
{
    filepath=s;
}

void ExportHTMLDialog::setMapName(const QString &s)
{
    mapname=s;
}

QString ExportHTMLDialog::getDir()
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


