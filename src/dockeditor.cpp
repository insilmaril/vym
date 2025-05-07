#include "dockeditor.h"

#include "mainwindow.h"
#include "vymmodel.h"

#include <QCloseEvent>

extern Main *mainWindow;

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

void DockEditor::closeEvent(QCloseEvent *event)
{
    // Pass event on to overloaded editors (TreeEditor, SlideEditor)
    ((DockEditor*)widget())->closeEvent(event);
}
