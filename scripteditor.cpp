#include "scripteditor.h"

#include <QDebug> //FIXME-3 testing
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>

#include "command.h"
#include "macros.h"
#include "mainwindow.h"
#include "options.h"
#include "slideitem.h"
#include "slidemodel.h"
#include "vymmodel.h"

extern QString vymName;
extern QList <Command*> modelCommands;
extern QDir lastMapDir;
extern Macros macros;
extern Main *mainWindow;
extern Options options;

ScriptEditor::ScriptEditor (QWidget *parent):QWidget(parent)
{
    ui.setupUi (this);

    //connect ( ui.openButton, SIGNAL (clicked() ), this, SLOT (openClicked() ));
    connect ( ui.saveSlideButton, SIGNAL (clicked() ), this, SLOT (saveSlide() ));
    //connect ( ui.saveAsButton, SIGNAL (clicked() ), this, SLOT (saveAsClicked() ));
    connect ( ui.runSlideButton,  SIGNAL (clicked() ), this, SLOT (runClicked() ));
    connect ( ui.runMacroButton,  SIGNAL (clicked() ), this, SLOT (runClicked() ));
    connect ( ui.runFileButton,   SIGNAL (clicked() ), this, SLOT (runClicked() ));
    connect ( ui.macroLoadButton, SIGNAL (pressed()), this, SLOT (loadMacroClicked() ) );
    connect ( ui.macroSaveButton, SIGNAL (pressed()), this, SLOT (saveMacroClicked() ) );


    vymModelID=-1;

    // Initialize Editor
    QFont font;
    font.setFamily("Courier");
    font.setFixedPitch(true);
    font.setPointSize(12);
    ui.editor->setFont(font);

    ui.modeTabWidget->setTabText(0,tr("Slide","Mode in scriptEditor"));
    ui.modeTabWidget->setTabText(1,tr("Macro","Mode in scriptEditor"));

    ui.keyCombo->insertItem(0, QString("---") );

    // Init function key selection
    for (int i=0; i<24; i++)
    {
        QString prefix="";
        if (i>11) prefix="Shift +";
        int n=i%12 + 1;
        ui.keyCombo->insertItem(i, QString("%1 F%2").arg(prefix).arg(n) );
    }
    
    highlighter = new Highlighter(ui.editor->document());
    QStringList list;
    foreach (Command *c, modelCommands)
	list.append (c->getName() );
    highlighter->addKeywords (list);

    QAction *a = new QAction( tr( "Save","ScriptEditor" ), ui.editor);
    a->setShortcut (Qt::CTRL + Qt::Key_S );	 
    a->setShortcutContext (Qt::WidgetWithChildrenShortcut);
    addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( saveSlide() ) );
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
    ui.editor->setText (in.readAll());
    f.close();
}

void ScriptEditor::saveFile()
{
    QFile f( filename );
    if ( !f.open( QIODevice::WriteOnly ) ) 
    {
        QMessageBox::warning(0, QObject::tr("Error"),QObject::tr("Couldn't save \"%1\"").arg(filename));
        return;
    }

    QTextStream t( &f );
    t.setCodec("UTF-8");
    t << ui.editor->toPlainText();
    f.close();
}

void ScriptEditor::saveSlide()
{
    VymModel *vm=mainWindow->getModel(vymModelID);
    if (!vm)
    {
	QMessageBox::warning(0,tr("Warning"),tr("Couldn't get model to save script into slide!"));
	return;
    }
    SlideItem *si=vm->getSlideModel()->findSlideID(slideID);
    if (!si)
    {
	QMessageBox::warning(0,tr("Warning"),tr("Couldn't find slide to save script into slide!"));
	return;
    }
    si->setInScript(ui.editor->toPlainText());
}

void ScriptEditor::setSlideScript(uint model_id, uint slide_id,const QString &s)
{
    vymModelID=model_id;
    slideID=slide_id;
    mode=Slide;
    ui.editor->setText(s);
}

void ScriptEditor::saveClicked()
{
    if (filename.isEmpty() )
	saveAsClicked();
    else
	saveFile();
}

void ScriptEditor::saveAsClicked()
{
    QString fn = QFileDialog::getSaveFileName( 
	this, 
	QString (vymName + " - " +tr("Save script")),
	QString (),
	"VYM script (*.vys);;All files (*)",
	0,
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
	filename=fn;
	saveFile();
    }
}

void ScriptEditor::openClicked()
{
    QString filter("VYM scripts (*.vys);;All (*)");
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
    ui.editor->setText( ts.readAll() );
	f.close();
	lastMapDir.setPath(fn.left(fn.lastIndexOf ("/")) );
    }
}

void ScriptEditor::runClicked()
{
    emit runScript (ui.editor->toPlainText() );
}

void ScriptEditor::loadMacroClicked()
{
    QString m=macros.getMacro (ui.keyCombo->currentIndex()+1);
    if (!m.isEmpty())
    ui.editor->setText (m);
}
void ScriptEditor::saveMacroClicked()
{
    filename=macros.getPath(ui.keyCombo->currentIndex());
    saveFile();
}
