#ifndef NOTEEDITOR_H 
#define NOTEEDITOR_H


#include "texteditor.h"

class NoteObj;

class NoteEditor : public TextEditor {
    Q_OBJECT
public:
    NoteEditor();
    ~NoteEditor();

    NoteObj getNoteObj();
    void setNote(const NoteObj &note);
};

#endif
