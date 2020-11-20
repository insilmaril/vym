#ifndef HEADINGEDITOR_H 
#define HEADINGEDITOR_H

#include "texteditor.h"

class HeadingEditor :  public TextEditor {
    Q_OBJECT
public:
    HeadingEditor(QString scope);
    ~HeadingEditor();

};

#endif
