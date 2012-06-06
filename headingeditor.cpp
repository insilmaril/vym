#include "headingeditor.h"
#include "settings.h"

extern Settings settings;
extern QString vymName;

HeadingEditor::HeadingEditor()	
{
    setWindowTitle (vymName +" - " +tr ("Heading Editor","Window caption"));

    // Load Settings
    init ("headingeditor");
}

HeadingEditor::~HeadingEditor() {}

