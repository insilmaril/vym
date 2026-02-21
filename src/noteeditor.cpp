#include "noteeditor.h"

#include <QMenuBar>

#include "settings.h"
#include "vymnote.h"

extern Settings settings;
extern QString vymName;

NoteEditor::NoteEditor(const QString &id, const QString &scope) : TextEditor(id, scope)
{
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
    note.setFileName(fileName());
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
    setFileName(note.fileName());
}
