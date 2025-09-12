#include "noteeditor.h"

#include <QMenuBar>

#include "settings.h"
#include "vymnote.h"

extern Settings settings;
extern QString vymName;

NoteEditor::NoteEditor(const QString &eName) : TextEditor(eName)
{
    editorName = eName;

    setWindowTitle("");

    menuBar()->show();

    setUseMapBackgroundColor(false);
}

NoteEditor::~NoteEditor() {}

VymNote NoteEditor::getNote()
{
    VymNote note;
    if (actionFormatRichText->isChecked())
        note.setRichText(getText());
    else
        note.setPlainText(getText());
    note.setFontHint(getFontHint());
    note.setFilenameHint(getFilenameHint());
    return note;
}

void NoteEditor::setNote(const VymNote &note)
{
    if (note.isRichText())
        setRichText(note.getText());
    else {
        setPlainText(note.getText());
        setFontHint(note.getFontHint());
    }
    setFilenameHint(note.getFilenameHint());
}
