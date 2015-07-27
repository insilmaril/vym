#include "noteeditor.h"

#include <QMenuBar>

#include "vymnote.h"
#include "settings.h"
 
extern Settings settings;
extern QString vymName;

NoteEditor::NoteEditor(QString scope):TextEditor(scope)
{
    setWindowTitle (vymName +" - " +tr ("Note Editor","Window caption"));

    menuBar()->show();

    // Load Settings
    init(shortcutScope);
}

NoteEditor::~NoteEditor() {}

VymNote NoteEditor::getNote()
{
    VymNote note;
    if (actionFormatRichText->isChecked() )
        note.setRichText( getText());
    else
        note.setPlainText( getText());
    note.setFontHint (getFontHint() );
    note.setFilenameHint (getFilenameHint () );
    return note;
}

void NoteEditor::setNote (const VymNote &note)  
{
    if (note.isRichText ())
        setRichText(note.getText());
    else
    {
        setPlainText(note.getText());
        setFontHint (note.getFontHint() );
    }
    setFilenameHint (note.getFilenameHint() );
}
