#include "dockeditor.h"

#include "vymmodel.h"

DockEditor::DockEditor() { init(); }
DockEditor::DockEditor(QString title, QWidget *p, VymModel *m)
    : QDockWidget(title, p)
{
    editorTitle = title;
    model = m;
    init();
}

void DockEditor::init()
{
    connect(this, SIGNAL(topLevelChanged(bool)), this,
            SLOT(changeTopLevel(bool)));
}

void DockEditor::changeTopLevel(bool topLevel)
{
    if (topLevel && model)
        setWindowTitle(editorTitle + ": " + model->getFileName());
    else
        setWindowTitle(editorTitle);
}
