#include "scripteditor.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>

#include "branchitem.h"
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

    branchEditor = new CodeEditor(this);
    ui.branchVerticalLayout->insertWidget(0, branchEditor);
    branchEditor->installEventFilter(this);

    connect(branchEditor, SIGNAL(textChanged()), this,
            SLOT(branchScriptChanged()));
    connect(ui.branchRunButton, SIGNAL(clicked()), this, SLOT(runBranchScript()));
    connect(ui.branchDeleteButton, SIGNAL(clicked()), this,
            SLOT(deleteBranchScript()));

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

    branchModelID = 0;
    branchScriptModified = false;
    blockBranchScriptSignal = false;

    // Initialize Editor
    branchEditor->setFont(fixedFont);
    slideEditor->setFont(fixedFont);
    macroEditor->setFont(fixedFont);
    codeEditor->setFont(fixedFont);

    // Define tab width
    const qreal d = 20; // unit is pixels
    codeEditor->setTabStopDistance(d);
    slideEditor->setTabStopDistance(d);
    macroEditor->setTabStopDistance(d);
    branchEditor->setTabStopDistance(d);

    ui.modeTabWidget->setTabText(0, tr("Slide", "Mode in scriptEditor"));
    ui.modeTabWidget->setTabText(1, tr("Macro", "Mode in scriptEditor"));
    ui.modeTabWidget->setTabText(2, tr("Script", "Mode in scriptEditor"));
    ui.modeTabWidget->setTabText(3, tr("Branch", "Mode in scriptEditor"));

    updateBranchScript(nullptr);

    ui.scriptPathLineEdit->setText(
        tr("No script selected", "scriptname in scriptEditor"));

    reloadMacros();

    highlighterBranch = new Highlighter(branchEditor->document());
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
    highlighterBranch->addKeywords(list);
    highlighterMacro->addKeywords(list);
    highlighterSlide->addKeywords(list);
    highlighterFile->addKeywords(list);

    codeEditor->setStyleSheet("QPlainTextEdit {" + editorFocusInStyle + "}");
    slideEditor->setStyleSheet("QPlainTextEdit {" + editorFocusInStyle + "}");
    macroEditor->setStyleSheet("QPlainTextEdit {" + editorFocusInStyle + "}");
    branchEditor->setStyleSheet("QPlainTextEdit {" + editorFocusInStyle + "}");

    QString shortcutScope = tr("Script editor", "Shortcut scope");
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
        case 3:
            branchEditor->setFocus();
            break;
    }
}

void ScriptEditor::setFocusBranchScript()
{
    ui.modeTabWidget->setCurrentIndex(3);
    branchEditor->setFocus();
}

bool ScriptEditor::eventFilter(QObject *obj, QEvent *ev)
{
    // Save modified script of branch, when editor looses focus, e.g. because
    // another branch is going to be selected in the map
    if (obj == branchEditor && ev->type() == QEvent::FocusOut)
        saveBranchScript();

    return QWidget::eventFilter(obj, ev);
}

BranchItem *ScriptEditor::branchScriptItem()
{
    if (branchUuid.isNull())
        return nullptr;

    VymModel *vm = mainWindow->modelWithId(branchModelID);
    if (!vm)
        // Map has been closed meanwhile
        return nullptr;

    TreeItem *ti = vm->findUuid(branchUuid);
    if (ti && ti->hasTypeBranch())
        return (BranchItem *)ti;

    // Branch has been deleted meanwhile
    return nullptr;
}

void ScriptEditor::branchScriptChanged()
{
    if (blockBranchScriptSignal)
        return;

    branchScriptModified = true;

    BranchItem *bi = branchScriptItem();
    if (!bi)
        return;

    // Update the branch immediately, so that the flag in the map appears or
    // disappears while editing. The state for the history is only saved later
    // in saveBranchScript(), which avoids an undo step for every keypress.
    if (bi->setScript(branchEditor->toPlainText())) {
        // Flag has changed
        bi->getModel()->emitDataChanged(bi);
        bi->getModel()->reposition();
    }
}

void ScriptEditor::saveBranchScript()
{
    if (!branchScriptModified)
        return;

    branchScriptModified = false;

    BranchItem *bi = branchScriptItem();
    if (!bi)
        return;

    // The branch already contains the modified script (see
    // branchScriptChanged), restore the original one, so that setScript can
    // save both states in the history
    QString script = branchEditor->toPlainText();
    bi->setScript(branchScriptOriginal);
    bi->getModel()->setScript(script, bi);

    branchScriptOriginal = script;
}

void ScriptEditor::updateBranchScript(VymModel *vm)
{
    // Save pending changes of previously shown branch first
    saveBranchScript();

    BranchItem *bi = nullptr;
    if (vm && vm->getSelectedBranches().count() == 1)
        bi = vm->getSelectedBranch();

    blockBranchScriptSignal = true;
    if (bi) {
        branchModelID = vm->modelId();
        branchUuid = bi->getUuid();
        branchScriptOriginal = bi->getScript();
        branchEditor->setPlainText(branchScriptOriginal);
        ui.branchHeadingLineEdit->setText(bi->headingPlain());
    }
    else {
        branchModelID = 0;
        branchUuid = QUuid();
        branchScriptOriginal.clear();
        branchEditor->clear();
        ui.branchHeadingLineEdit->setText(
            tr("No branch selected", "scriptEditor"));
    }
    blockBranchScriptSignal = false;

    branchScriptModified = false;

    branchEditor->setEnabled(bi != nullptr);
    ui.branchRunButton->setEnabled(bi != nullptr);
    ui.branchDeleteButton->setEnabled(bi != nullptr);
}

void ScriptEditor::reloadBranchScript(BranchItem *bi)
{
    if (!bi || bi != branchScriptItem())
        // Branch is not shown in branch tab
        return;

    if (branchEditor->toPlainText() == bi->getScript())
        // Nothing to do, e.g. we have saved the script ourselves.
        // Don't touch the editor, this would also reset the cursor position
        return;

    blockBranchScriptSignal = true;
    branchScriptOriginal = bi->getScript();
    branchEditor->setPlainText(branchScriptOriginal);
    blockBranchScriptSignal = false;

    branchScriptModified = false;
}

void ScriptEditor::runBranchScript()
{
    saveBranchScript();

    emit runScript(branchEditor->toPlainText());
}

void ScriptEditor::deleteBranchScript()
{
    // Save pending modifications first, so that they can be undone separately
    saveBranchScript();

    BranchItem *bi = branchScriptItem();
    if (!bi)
        return;

    blockBranchScriptSignal = true;
    branchEditor->clear();
    blockBranchScriptSignal = false;

    branchScriptOriginal.clear();
    bi->getModel()->setScript(QString(), bi);
}

QString ScriptEditor::getScriptFile() { return codeEditor->toPlainText(); }

void ScriptEditor::saveSlide()
{
    VymModel *vm = mainWindow->modelWithId(vymModelID);
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
