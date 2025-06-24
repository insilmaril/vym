#ifndef HEADINGEDITOR_H
#define HEADINGEDITOR_H

#include "texteditor.h"

class HeadingEditor : public TextEditor {
    Q_OBJECT
  public:
    HeadingEditor(const QString &eName = "undefined");
    ~HeadingEditor();
};

#endif
