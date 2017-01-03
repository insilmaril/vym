#include "noteeditor.h"

#include <QMenuBar>

#include "vymnote.h"
#include "settings.h"
 
extern Settings settings;
extern QString vymName;

NoteEditor::NoteEditor(QString scope):TextEditor()
{
    editorName = tr("Note Editor","Name of editor shown as window title");
    setWindowTitle("");

    menuBar()->show();

    // Load Settings
    init(scope);
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
