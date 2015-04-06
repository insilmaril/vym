#include "scriptoutput.h"

ScriptOutput::ScriptOutput(QWidget *parent)
{
    editor = new QTextEdit(this);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(editor);
    setLayout(layout);
}

ScriptOutput::~ScriptOutput()
{
    delete layout;
    delete editor;
}

void ScriptOutput::setText(const QString &text)
{
    editor->setText(text);
}
void ScriptOutput::append(const QString &text)
{
    editor->append(text);
}
