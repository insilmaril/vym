#include "headingeditor.h"
#include "settings.h"

extern Settings settings;
extern QString vymName;

HeadingEditor::HeadingEditor(const QString &eName) : TextEditor(eName)
{
    editorName = eName;
    setWindowTitle("");

    setUseMapBackgroundColor(true);
}

HeadingEditor::~HeadingEditor() {}
