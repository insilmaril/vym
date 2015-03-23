#include "noteeditor.h"

#include <QMenuBar>

#include "noteobj.h"
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

NoteObj NoteEditor::getNoteObj()
{
    NoteObj note;
    if (actionFormatRichText->isChecked() )
        note.setNoteRichText( getText());
    else
        note.setNotePlain( getText());
    note.setFontHint (getFontHint() );
    note.setFilenameHint (getFilenameHint () );
    return note;
}

void NoteEditor::setNote (const NoteObj &note)
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
