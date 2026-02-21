#include "headingeditor.h"
#include "settings.h"

extern Settings settings;
extern QString vymName;

HeadingEditor::HeadingEditor(const QString &id, const QString &scope) : TextEditor(id, scope)
{
    // qDebug() << "Constr HE" << id << scope;
    setWindowTitle("");

    setUseMapBackgroundColor(true);
}

HeadingEditor::~HeadingEditor() {}
