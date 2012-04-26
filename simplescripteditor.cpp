#include "simplescripteditor.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>

#include "command.h"

extern QString vymName;
extern QList <Command*> modelCommands;
extern QDir lastMapDir;

SimpleScriptEditor::SimpleScriptEditor (QWidget *parent):QDialog(parent)
{
    ui.setupUi (this);

    connect ( ui.openButton, SIGNAL (clicked() ), this, SLOT (openClicked() ));
    connect ( ui.saveButton, SIGNAL (clicked() ), this, SLOT (saveClicked() ));
    connect ( ui.saveAsButton, SIGNAL (clicked() ), this, SLOT (saveAsClicked() ));
    connect ( ui.runButton,  SIGNAL (clicked() ), this, SLOT (runClicked() ));


    // Initialize Editor
    QFont font;
    font.setFamily("Courier");
    font.setFixedPitch(true);
    font.setPointSize(12);
    ui.editor->setFont(font);

    highlighter = new Highlighter(ui.editor->document());
    QStringList list;
    foreach (Command *c, modelCommands)
	list.append (c->getName() );
    highlighter->addKeywords (list);
}

void SimpleScriptEditor::saveScript()
{
    QFile f( filename );
    if ( !f.open( QIODevice::WriteOnly ) ) 
    {
	return;
    }

    QTextStream t( &f );
    t << ui.editor->toPlainText();
    f.close();
}

void SimpleScriptEditor::setScript(const QString &s)
{
    ui.editor->setText(s);
}

void SimpleScriptEditor::saveClicked()
{
    if (filename.isEmpty() )
	saveAsClicked();
    else
	saveScript();
}

void SimpleScriptEditor::saveAsClicked()
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
		    saveScript();
		    return;
		case QMessageBox::Cancel:
		    // do nothing
		    return;
	    }
	} 
	filename=fn;
	saveScript();
    }
}

void SimpleScriptEditor::openClicked()
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
	ui.editor->setText( ts.readAll() );
	f.close();
	lastMapDir.setPath(fn.left(fn.lastIndexOf ("/")) );
    }
}

void SimpleScriptEditor::runClicked()
{
    emit runScript (ui.editor->toPlainText() );
}
