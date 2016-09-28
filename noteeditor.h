#ifndef NOTEEDITOR_H 
#define NOTEEDITOR_H


#include "texteditor.h"

class VymNote;

class NoteEditor : public TextEditor {
    Q_OBJECT
public:
    NoteEditor(QString scope);
    ~NoteEditor();

    VymNote getNote();
    void setNote(const VymNote &note);
};

#endif
