#include "headingeditor.h"
#include "settings.h"

extern Settings settings;
extern QString vymName;

HeadingEditor::HeadingEditor(QString scope):TextEditor()
{
    editorName = tr("Heading Editor","Name of editor shown as window title");
    setWindowTitle("");

    // Load Settings
    init (scope);
}

HeadingEditor::~HeadingEditor() {}

