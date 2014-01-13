#include "noteeditor.h"

#include <QMenuBar>

#include "noteobj.h"
#include "settings.h"

extern Settings settings;
extern QString vymName;

NoteEditor::NoteEditor()    
{
    setWindowTitle (vymName +" - " +tr ("Note Editor","Window caption"));

    menuBar()->show();

    // Load Settings
    init("noteeditor");
}

NoteEditor::~NoteEditor() {}

NoteObj NoteEditor::getNoteObj()
{
    NoteObj note (getText() );
    note.setFontHint (getFontHint() );
    note.setFilenameHint (getFilenameHint () );
    return note;
}

void NoteEditor::setNote (const NoteObj &note)
{
    setText (note.getNote() );
    if (!note.isRichText ())
	setFontHint (note.getFontHint() );
    setFilenameHint (note.getFilenameHint() );
}


