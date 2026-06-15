#include "scriptoutput.h"

#include <QAction>

ScriptOutput::ScriptOutput(QWidget *parent) : QWidget(parent)
{
    editor = new QTextEdit(this);   // FIXME-4 use QTextBrowser and add button to clear browser
    layout = new QVBoxLayout;
    layout->addWidget(editor);
    setLayout(layout);

    QAction *a = new QAction(this);
    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    a->setShortcut(Qt::CTRL | Qt::Key_D);
    addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(closeWindow()));
}

ScriptOutput::~ScriptOutput()
{
    delete layout;
    delete editor;
}

void ScriptOutput::closeWindow() { parentWidget()->hide(); }

void ScriptOutput::setFocus() { //FIXME-5 missing implementation
    // qDebug() << "SO::setFOcus";
    // Currently ScriptEditor gets focus, when output is toggled
}

void ScriptOutput::clear() { editor->clear(); }

void ScriptOutput::setText(const QString &text) { editor->setText(text); }

QString ScriptOutput::text() { return editor->toPlainText(); }

void ScriptOutput::append(const QString &text) { editor->append(text); }
