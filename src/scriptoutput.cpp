#include "scriptoutput.h"

ScriptOutput::ScriptOutput(QWidget *parent) : QWidget(parent)
{
    editor = new QTextEdit(this);
    layout = new QVBoxLayout;
    layout->addWidget(editor);
    setLayout(layout);
}

ScriptOutput::~ScriptOutput()
{
    delete layout;
    delete editor;
}

void ScriptOutput::setFocus() { //FIXME-0 missing implementation
    qDebug() << "SO::setFOcus";
}

void ScriptOutput::clear() { editor->clear(); }

void ScriptOutput::setText(const QString &text) { editor->setText(text); }

QString ScriptOutput::text() { return editor->toPlainText(); }

void ScriptOutput::append(const QString &text) { editor->append(text); }
