#include "headingeditor.h"
#include "settings.h"

extern Settings settings;
extern QString vymName;

HeadingEditor::HeadingEditor(QString scope):TextEditor(scope)
{
    QMainWindow::setWindowTitle (vymName +" - " +tr ("Heading Editor","Window caption"));

    // Load Settings
    init ("shortcutScope");
}

HeadingEditor::~HeadingEditor() {}

