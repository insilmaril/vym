#include "scripteditor.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>

#include "command.h"
#include "file.h"
#include "macros.h"

#include "mainwindow.h"
#include "options.h"
#include "shortcuts.h"
#include "settings.h"
#include "slideitem.h"
#include "slidemodel.h"
#include "vymmodel.h"

extern QString vymName;
extern QList<Command *> branchCommands;
extern QList<Command *> imageCommands;
extern QList<Command *> itemListCommands;
extern QList<Command *> modelCommands;
extern QList<Command *> vymCommands;
extern QList<Command *> xlinkCommands;
extern QDir lastScriptDir;
extern Macros macros;
extern Main *mainWindow;
extern Options options;
extern Settings settings;
extern QFont fixedFont;
extern QString editorFocusInStyle;
extern Switchboard switchboard;

ScriptEditor::ScriptEditor(QWidget *parent) : QWidget(parent)
{
    ui.setupUi(this);

    codeEditor = new CodeEditor(this);
    ui.fileVerticalLayout->insertWidget(0, codeEditor);

    macroEditor = new CodeEditor(this);
    ui.macroVerticalLayout->insertWidget(0, macroEditor);

    slideEditor = new CodeEditor(this);
    ui.slideVerticalLayout->insertWidget(0, slideEditor);

    connect(ui.slideSaveButton, SIGNAL(clicked()), this, SLOT(saveSlide()));
    connect(ui.slideRunButton, SIGNAL(clicked()), this, SLOT(runSlide()));
    connect(ui.macroRunButton, SIGNAL(clicked()), this, SLOT(runMacro()));
    connect(ui.fileRunButton, SIGNAL(clicked()), this, SLOT(runScript()));
    connect(ui.macroLoadButton, SIGNAL(clicked()), this, SLOT(reloadMacros()));
    connect(ui.macroSaveButton, SIGNAL(clicked()), this, SLOT(saveMacros()));
    connect(ui.fileReloadButton, SIGNAL(clicked()), this, SLOT(reloadScript()));
    connect(ui.fileLoadButton, SIGNAL(clicked()), this, SLOT(loadScript()));
    connect(ui.fileSaveButton, SIGNAL(clicked()), this, SLOT(saveScript()));
    connect(ui.fileSaveAsButton, SIGNAL(clicked()), this, SLOT(saveScriptAs()));

    vymModelID = -1;

    // Initialize Editor
    slideEditor->setFont(fixedFont);
    macroEditor->setFont(fixedFont);
    codeEditor->setFont(fixedFont);

    // Define tab width
    const qreal d = 20; // unit is pixels
    codeEditor->setTabStopDistance(d);
    slideEditor->setTabStopDistance(d);
    macroEditor->setTabStopDistance(d);

    ui.modeTabWidget->setTabText(0, tr("Slide", "Mode in scriptEditor"));
    ui.modeTabWidget->setTabText(1, tr("Macro", "Mode in scriptEditor"));
    ui.modeTabWidget->setTabText(2, tr("Script", "Mode in scriptEditor"));

    ui.scriptPathLineEdit->setText(
        tr("No script selected", "scriptname in scriptEditor"));

    reloadMacros();

    highlighterMacro = new Highlighter(macroEditor->document());
    highlighterSlide = new Highlighter(slideEditor->document());
    highlighterFile = new Highlighter(codeEditor->document());
    QStringList list;
    foreach (Command *c, vymCommands)
        list.append(QString("\\b%1\\b").arg(c->name()));
    foreach (Command *c, modelCommands)
        list.append(QString("\\b%1\\b").arg(c->name()));
    foreach (Command *c, branchCommands)
        list.append(QString("\\b%1\\b").arg(c->name()));
    foreach (Command *c, imageCommands)
        list.append(QString("\\b%1\\b").arg(c->name()));
    foreach (Command *c, itemListCommands)
        list.append(QString("\\b%1\\b").arg(c->name()));
    foreach (Command *c, xlinkCommands)
        list.append(QString("\\b%1\\b").arg(c->name()));
    highlighterMacro->addKeywords(list);
    highlighterSlide->addKeywords(list);
    highlighterFile->addKeywords(list);

    codeEditor->setStyleSheet("QPlainTextEdit {" + editorFocusInStyle + "}");
    slideEditor->setStyleSheet("QPlainTextEdit {" + editorFocusInStyle + "}");
    macroEditor->setStyleSheet("QPlainTextEdit {" + editorFocusInStyle + "}");

    QString shortcutScope = parentWidget()->windowTitle();
    switchboard.addScope("MainWindow", shortcutScope);

    QAction *a = new QAction("Close window", this);
    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    switchboard.addAction(a, "textCloseWindow", Qt::CTRL | Qt::Key_D, shortcutScope, "Misc"); // FIXME-3 translation?
    connect(a, SIGNAL(triggered()), this, SLOT(closeWindow()));
    //fileMenu->addAction(a);
    addAction(a);

    // QAction *a = new QAction( tr( "Save","ScriptEditor" ), ui.editor);
    // a->setShortcut (Qt::CTRL | Qt::Key_S );
    // a->setShortcutContext (Qt::WidgetWithChildrenShortcut);
    // addAction (a);
    // connect( a, SIGNAL( triggered() ), this, SLOT( saveSlide() ) );
}

void ScriptEditor::setFocus() {
    switch (ui.modeTabWidget->currentIndex()) {
        case 0:
            slideEditor->setFocus();
            break;
        case 1:
            macroEditor->setFocus();
            break;
        case 2:
            codeEditor->setFocus();
            break;
    }
}

QString ScriptEditor::getScriptFile() { return codeEditor->toPlainText(); }

void ScriptEditor::saveSlide()
{
    VymModel *vm = mainWindow->getModel(vymModelID);
    if (!vm) {
        QMessageBox::warning(
            0, tr("Warning"),
            tr("Couldn't get model to save script into slide!"));
        return;
    }
    SlideItem *si = vm->getSlideModel()->findSlideID(slideID);
    if (!si) {
        QMessageBox::warning(
            0, tr("Warning"),
            tr("Couldn't find slide to save script into slide!"));
        return;
    }
    si->setInScript(slideEditor->toPlainText());
}

void ScriptEditor::setSlideScript(uint model_id, uint slide_id,
                                  const QString &s)
{
    vymModelID = model_id;
    slideID = slide_id;
    mode = Slide;
    slideEditor->setPlainText(s);
}

void ScriptEditor::closeWindow()
{
    parentWidget()->hide();
    mainWindow->updateActions();
    qDebug() << "SE::closeWindow";
}

void ScriptEditor::runMacro() { emit runScript(macroEditor->toPlainText()); }

void ScriptEditor::runSlide() { emit runScript(slideEditor->toPlainText()); }

void ScriptEditor::runScript() { emit runScript(codeEditor->toPlainText()); }

void ScriptEditor::reloadMacros()
{
    QString m = macros.get();
    if (!m.isEmpty()) {
        macroEditor->setPlainText(m);
        ui.macroPathLineEdit->setText(macros.getPath());
    }
}

void ScriptEditor::saveMacros()
{
    if (saveStringToDisk(macros.getPath(), macroEditor->toPlainText()))
        mainWindow->statusMessage(
            tr("Macros saved to %1").arg(macros.getPath()));
    else {
        QString error(QObject::tr("Error"));
        QString msg(QObject::tr("Couldn't write macros to \"%1\"\n.")
                        .arg(macros.getPath()));
        QMessageBox::warning(0, error, msg);
    }
}

bool ScriptEditor::loadScript(QString fn)
{
    if (fn.isEmpty()) {
        QString filter("VYM scripts (*.vys);;All (*)");
        fn = QFileDialog::getOpenFileName(this,
                                          vymName + " - " + tr("Load script"),
                                          lastScriptDir.path(), filter);
    }

    if (!fn.isEmpty()) {
        filename = fn;
        QString s;
        if (loadStringFromDisk(filename, s)) {
            codeEditor->setPlainText(s);
            ui.scriptPathLineEdit->setText(filename);
            lastScriptDir.setPath(filename.left(filename.lastIndexOf("/")));
            return true;
        }
        else {
            QString error(QObject::tr("Error"));
            QString msg(
                QObject::tr("Couldn't read script from \"%1\"\n.").arg(fn));
            QMessageBox::warning(0, error, msg);
        }
    }
    return false;
}

bool ScriptEditor::reloadScript()
{
    if (filename.isEmpty())
        return false;
    else {
        QString s;
        if (loadStringFromDisk(filename, s)) {
            codeEditor->setPlainText(s);
            ui.scriptPathLineEdit->setText(filename);
            lastScriptDir.setPath(filename.left(filename.lastIndexOf("/")));
            return true;
        }
        else {
            QString error(QObject::tr("Error"));
            QString msg(
                QObject::tr("Couldn't read script from \"%1\"\n.").arg(filename));
            QMessageBox::warning(0, error, msg);
        }
    }
    return false;
}

void ScriptEditor::saveScript()
{
    if (filename.isEmpty())
        saveScriptAs();
    else {
        if (saveStringToDisk(filename, codeEditor->toPlainText()))
            mainWindow->statusMessage(tr("Script saved to %1").arg(filename));
        else {
            QString error(QObject::tr("Error"));
            QString msg(QObject::tr("Couldn't write script to \"%1\"\n.")
                            .arg(filename));
            QMessageBox::warning(0, error, msg);
        }
    }
}

void ScriptEditor::saveScriptAs()
{
    QString filter("VYM scripts (*.vys *.js);;All (*)");
    QString fn = QFileDialog::getSaveFileName(
        this, QString(vymName + " - " + tr("Save script")), QString(),
        "VYM script (*js *.vys);;All files (*)");

    if (!fn.isEmpty()) {
        QFile file(fn);
        // Already tested in QFileDialog, if we may overwrite in case file exists already

        filename = fn;
        ui.scriptPathLineEdit->setText(filename);
        lastScriptDir.setPath(filename.left(filename.lastIndexOf("/")));
        saveScript();
    }
}
