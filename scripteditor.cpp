#include "scripteditor.h"

#include <QDebug> //FIXME-3 testing
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>

#include "command.h"
#include "macros.h"
#include "mainwindow.h"
#include "options.h"
#include "settings.h"
#include "slideitem.h"
#include "slidemodel.h"
#include "vymmodel.h"

extern QString vymName;
extern QList <Command*> modelCommands;
extern QDir lastMapDir;
extern Macros macros;
extern Main *mainWindow;
extern Options options;
extern Settings settings;

ScriptEditor::ScriptEditor (QWidget *parent):QWidget( parent )
{
    ui.setupUi (this);

    connect ( ui.slideSaveButton,  SIGNAL (clicked() ), this, SLOT (saveSlide() ));
    connect ( ui.slideRunButton,   SIGNAL (clicked() ), this, SLOT (runSlide() ));
    connect ( ui.macroRunButton,   SIGNAL (clicked() ), this, SLOT (runMacro() ));
    connect ( ui.fileRunButton,   SIGNAL (clicked() ), this, SLOT (runFile() ));
    connect ( ui.macroLoadButton,  SIGNAL (clicked() ), this, SLOT (loadMacro() ) );
    connect ( ui.macroSaveButton,  SIGNAL (clicked() ), this, SLOT (saveMacro() ) );
    connect ( ui.fileLoadButton,   SIGNAL (clicked() ), this, SLOT (loadFile() ) );
    connect ( ui.fileSaveButton,   SIGNAL (clicked() ), this, SLOT (saveFile() ) );
    connect ( ui.fileSaveAsButton, SIGNAL (clicked() ), this, SLOT (saveFileAs() ) );


    vymModelID=-1;

    // Initialize Editor
    QFont font;
    font.setFamily("Courier");
    font.setFixedPitch(true);
    font.setPointSize(12);
    ui.slideEditor->setFont(font);
    ui.macroEditor->setFont(font);
    ui.fileEditor->setFont(font);

    ui.modeTabWidget->setTabText(0,tr("Slide","Mode in scriptEditor"));
    ui.modeTabWidget->setTabText(1,tr("Macro","Mode in scriptEditor"));
    ui.modeTabWidget->setTabText(2,tr("Script","Mode in scriptEditor"));

    ui.fileNameLabel->setText( tr("No script selected","scriptname in scriptEditor"));
    ui.keyCombo->insertItem(0, QString("---") );

    // Init function key selection
    for (int i=0; i<24; i++)
    {
        QString prefix="";
        if (i>11) prefix="Shift +";
        int n=i%12 + 1;
        ui.keyCombo->insertItem(i, QString("%1 F%2").arg(prefix).arg(n) );
    }
    
    highlighterMacro = new Highlighter(ui.macroEditor->document());
    highlighterSlide = new Highlighter(ui.slideEditor->document());
    highlighterFile = new Highlighter(ui.fileEditor->document());
    QStringList list;
    foreach (Command *c, modelCommands)
	list.append (c->getName() );
    highlighterMacro->addKeywords (list);
    highlighterSlide->addKeywords (list);
    highlighterFile->addKeywords (list);

    //QAction *a = new QAction( tr( "Save","ScriptEditor" ), ui.editor);
    //a->setShortcut (Qt::CTRL + Qt::Key_S );	 
    //a->setShortcutContext (Qt::WidgetWithChildrenShortcut);
    //addAction (a);
    //connect( a, SIGNAL( triggered() ), this, SLOT( saveSlide() ) );
}

void ScriptEditor::setScriptFile(const QString &fn) 
{
    QFile f( fn );
    if ( !f.open( QFile::ReadOnly|QFile::Text ) )
    {
	QString error (QObject::tr("Error"));
	QString msg (QObject::tr("Couldn't open \"%1\"\n%2.").arg(fn).arg(f.errorString()));
	if (options.isOn("batch"))
	    qWarning ()<<error+": "+msg;
	else    
	    QMessageBox::warning(0, error,msg);
	return;
    }	

    QTextStream in( &f );
    ui.fileEditor->setText (in.readAll());
    f.close();
}

void ScriptEditor::saveSlide()
{
    VymModel *vm = mainWindow->getModel(vymModelID);
    if (!vm)
    {
	QMessageBox::warning(0,tr("Warning"),tr("Couldn't get model to save script into slide!"));
	return;
    }
    SlideItem *si = vm->getSlideModel()->findSlideID(slideID);
    if (!si)
    {
	QMessageBox::warning(0,tr("Warning"),tr("Couldn't find slide to save script into slide!"));
	return;
    }
    si->setInScript(ui.slideEditor->toPlainText());
}

void ScriptEditor::setSlideScript(uint model_id, uint slide_id,const QString &s)
{
    vymModelID=model_id;
    slideID=slide_id;
    mode=Slide;
    ui.slideEditor->setText(s);
}

void ScriptEditor::runMacro()
{
    emit runLegacyScript (ui.macroEditor->toPlainText() );
}

void ScriptEditor::runSlide()
{
    emit runLegacyScript (ui.slideEditor->toPlainText() );
}

void ScriptEditor::runFile()
{
    emit runScript (ui.fileEditor->toPlainText() );
}

void ScriptEditor::loadMacro()
{
    QString m = macros.getMacro (ui.keyCombo->currentIndex()+1);
    if ( !m.isEmpty() ) ui.macroEditor->setText (m);
}
void ScriptEditor::saveMacro()
{
    filename = macros.getPath(ui.keyCombo->currentIndex());
    saveFile();
}
void ScriptEditor::loadFile()
{
    QString filter("VYM scripts (*.vys *.js);;All (*)");
    QString fn=QFileDialog::getOpenFileName( 
	this,
	vymName+" - " + tr("Load script"), 
	lastMapDir.path(), 
	filter);

    if (!fn.isEmpty() )
    {
	QFile f( fn);
	if ( !f.open( QIODevice::ReadOnly ) )
	{
	    QMessageBox::warning(0, 
		tr("Error"),
		tr("Couldn't open %1.\n").arg(filename));
	    return;
	}   

	QTextStream ts( &f );
    ts.setCodec("UTF-8");
    ui.fileEditor->setText( ts.readAll() );
	f.close();
	lastMapDir.setPath(fn.left(fn.lastIndexOf ("/")) );
    }
}

void ScriptEditor::saveFile()
{
    if (filename.isEmpty() )
	saveFileAs();
    else
    {
        QFile f( filename );
        if ( !f.open( QIODevice::WriteOnly ) ) 
        {
            QMessageBox::warning(0, QObject::tr("Error"),QObject::tr("Couldn't save \"%1\"").arg(filename));
            return;
        }

        QTextStream t( &f );
        t.setCodec("UTF-8");
        t << ui.fileEditor->toPlainText();
        f.close();
    }
}

void ScriptEditor::saveFileAs()
{
    QString filter("VYM scripts (*.vys *.js);;All (*)");
    QString fn = QFileDialog::getSaveFileName( 
	this, 
	QString (vymName + " - " + tr("Save script")),
	QString (), "VYM script (*js *.vys);;All files (*)", 0, 
        QFileDialog::DontConfirmOverwrite);
	
    if ( !fn.isEmpty() ) 
    {
	QFile file (fn);
	if (file.exists())
	{
	    QMessageBox mb( vymName,
		tr("The file %1\nexists already.\nDo you want to overwrite it?","dialog 'save as'").arg(fn),
	    QMessageBox::Warning,
	    QMessageBox::Yes | QMessageBox::Default,
	    QMessageBox::Cancel | QMessageBox::Escape,
	    Qt::NoButton );
	    mb.setButtonText( QMessageBox::Yes, tr("Overwrite") );
	    mb.setButtonText( QMessageBox::No, tr("Cancel"));
	    switch( mb.exec() ) 
	    {
		case QMessageBox::Yes:
		    // save 
		    filename = fn;
		    saveFile();
		    return;
		case QMessageBox::Cancel:
		    // do nothing
		    return;
	    }
	} 
	filename = fn;
	saveFile();
    }
}

