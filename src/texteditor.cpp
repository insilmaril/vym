#include "texteditor.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QColorDialog>
#include <QComboBox>
#include <QFileDialog>
#include <QFontDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QPrintDialog>
#include <QPrinter>
#include <QStatusBar>
#include <QTextEdit>
#include <QToolBar>

#include <typeinfo>

#include "mainwindow.h"
#include "settings.h"
#include "shortcuts.h"

extern Main *mainWindow;
extern int statusbarTime;
extern Settings settings;

extern QAction *actionViewToggleNoteEditor;

extern QString vymName;

extern Switchboard switchboard;

extern QPrinter *printer;
extern bool debug;

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

TextEditor::TextEditor()
{
    statusBar()->hide(); // Hide sizeGrip on default, which comes with statusBar

    e = new QTextEdit(this);
    e->setFocus();
    e->setTabStopDistance(20); // unit is pixel, default would be 80
    e->setAutoFillBackground(true);
    e->installEventFilter(this);
    connect(e, SIGNAL(textChanged()), this, SLOT(editorChanged()));
    setCentralWidget(e);
    statusBar()->showMessage(tr("Ready", "Statusbar message"), statusbarTime);

    connect(e, SIGNAL(currentCharFormatChanged(const QTextCharFormat &)), this,
            SLOT(formatChanged(const QTextCharFormat &)));

    // Don't show menubar per default
    menuBar()->hide();

    // Toolbars
    setupFileActions();
    setupEditActions();
    setupFormatActions();
    setupSettingsActions();

    // Various states
    blockChangedSignal = false;
    blockTextUpdate = false;
    setInactive();

    editorName = "Text editor";
    setEditorTitle("");
}

TextEditor::~TextEditor()
{
    // Save Settings
    QString n = QString("/satellite/%1/").arg(shortcutScope);
    settings.setValue(n + "geometry/size", size());
    settings.setValue(n + "geometry/pos", pos());
    settings.setValue(n + "state", saveState(0));

    QString s;
    if (actionSettingsFonthintDefault->isChecked())
        s = "fixed";
    else
        s = "variable";
    settings.setValue(n + "fonts/fonthintDefault", s);
    settings.setValue(n + "fonts/varFont", varFont.toString());
    settings.setValue(n + "fonts/fixedFont", fixedFont.toString());

    settings.setValue(n + "colors/richTextDefaultBackground", colorRichTextDefaultBackground.name());
    settings.setValue(n + "colors/richTextDefaultForeground", colorRichTextDefaultForeground.name());
}

void TextEditor::init(const QString &scope)
{
    shortcutScope = scope;
    QString n = QString("/satellite/%1/").arg(shortcutScope);
    restoreState(settings.value(n + "state", 0).toByteArray());
    filenameHint = "";
    fixedFont.fromString(
        settings.value(n + "fonts/fixedFont", "Courier,12,-1,5,48,0,0,0,1,0")
            .toString());
    varFont.fromString(
        settings
            .value(n + "fonts/varFont", "DejaVu Sans Mono,12,-1,0,50,0,0,0,0,0")
            .toString());
    QString s =
        settings.value(n + "fonts/fonthintDefault", "variable").toString();
    if (s == "fixed") {
        actionSettingsFonthintDefault->setChecked(true);
        e->setCurrentFont(fixedFont);
    }
    else {
        actionSettingsFonthintDefault->setChecked(false);
        e->setCurrentFont(varFont);
    }

    // Default colors
    QPixmap pix(16, 16);

    colorRichTextDefaultBackground.setNamedColor(
        settings.value(n + "colors/richTextDefaultBackground", "#ffffff").toString());
    pix.fill(colorRichTextDefaultBackground);
    actionFilledEditorColor->setIcon(pix);

    colorRichTextDefaultForeground.setNamedColor(
        settings.value(n + "colors/richTextDefaultForeground", "#000000").toString());
    e->setTextColor(colorRichTextDefaultForeground);
    pix.fill(colorRichTextDefaultForeground);
    actionFontColor->setIcon(pix);
}

bool TextEditor::isEmpty()
{
    if (e->toPlainText().length() > 0)
        return false;
    else
        return true;
}

void TextEditor::setEditorTitle(const QString &s)
{
    editorTitle = (s.isEmpty()) ? editorName : editorName + ": " + s;
    setWindowTitle(editorTitle);
}

QString TextEditor::getEditorTitle() { return editorTitle; }

void TextEditor::setEditorName(const QString &s) { editorName = s; }

void TextEditor::setFont(const QFont &font)
{
    blockChangedSignal = true;

    QTextCursor tc = e->textCursor();
    QTextCharFormat format = tc.charFormat();

    tc.select(QTextCursor::Document);
    format.setFont(font);
    tc.setCharFormat(format);
    tc.clearSelection();
    fontChanged(fixedFont);

    blockChangedSignal = false;
}

void TextEditor::setFontHint(const QString &fh)
{
    if (fh == "fixed") {
        actionFormatUseFixedFont->setChecked(true);
        e->setCurrentFont(fixedFont);
        setFont(fixedFont);
    }
    else {
        actionFormatUseFixedFont->setChecked(false);
        e->setCurrentFont(varFont);
        setFont(varFont);
    }
}

QString TextEditor::getFontHint()
{
    if (actionFormatUseFixedFont->isChecked())
        return "fixed";
    else
        return "var";
}

QString TextEditor::getFontHintDefault()
{
    if (actionSettingsFonthintDefault->isChecked())
        return "fixed";
    else
        return "var";
}

void TextEditor::setFilename(const QString &fn)
{
    if (state == filledEditor) {
        if (fn.isEmpty()) {
            filename = "";
            statusBar()->showMessage(
                tr("No filename available for this note.", "Statusbar message"),
                statusbarTime);
        }
        else {
            filename = fn;
            statusBar()->showMessage(
                tr(QString("Current filename is %1").arg(filename).toUtf8(),
                   "Statusbar message"),
                statusbarTime);
        }
    }
}

QString TextEditor::getFilename() { return filename; }

void TextEditor::setFilenameHint(const QString &fnh) { filenameHint = fnh; }

QString TextEditor::getFilenameHint() { return filenameHint; }

QString TextEditor::getText()
{
    if (e->toPlainText().isEmpty())
        return QString();

    if (actionFormatRichText->isChecked())
        return e->toHtml();
    else
        return e->toPlainText();
}

VymText TextEditor::getVymText()
{
    VymText vt;

    if (actionFormatRichText->isChecked())
        vt.setRichText(e->toHtml());
    else
        vt.setPlainText(e->toPlainText());

    if (actionFormatUseFixedFont->isChecked())
        vt.setFontHint(getFontHint());

    return vt;
}

bool TextEditor::findText(const QString &t,
                          const QTextDocument::FindFlags &flags)
{
    if (e->find(t, flags))
        return true;
    else
        return false;
}

bool TextEditor::findText(const QString &t,
                          const QTextDocument::FindFlags &flags, int i)
{
    // Position at beginning
    QTextCursor c = e->textCursor();
    c.setPosition(0, QTextCursor::MoveAnchor);
    e->setTextCursor(c);

    // Search for t
    int j = 0;
    while (j <= i) {
        if (!e->find(t, flags))
            return false;
        j++;
    }
    return true;
}

void TextEditor::setTextCursor(const QTextCursor &cursor)
{
    e->setTextCursor(cursor);
}

QTextCursor TextEditor::getTextCursor() { return e->textCursor(); }

void TextEditor::setFocus() { e->setFocus(); }

void TextEditor::setupFileActions()
{
    QToolBar *tb = addToolBar(tr("Note Actions"));
    tb->setObjectName("noteEditorFileActions");
    QMenu *fileMenu = menuBar()->addMenu(tr("&Note", "Menubar"));

    QString tag = tr("Texteditor", "Shortcuts");
    QAction *a;
    a = new QAction(QPixmap(":/fileopen.png"), tr("&Import..."), this);
    a->setShortcut(Qt::CTRL + Qt::Key_O);
    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    switchboard.addSwitch("textLoad", shortcutScope, a, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(textLoad()));
    tb->addAction(a);
    fileMenu->addAction(a);
    actionFileLoad = a;

    fileMenu->addSeparator();
    a = new QAction(QPixmap(":/filesave.png"), tr("&Export..."), this);
    a->setShortcut(Qt::CTRL + Qt::Key_S);
    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    switchboard.addSwitch("textSave", shortcutScope, a, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(textSave()));
    tb->addAction(a);
    fileMenu->addAction(a);
    addAction(a);
    actionFileSave = a;

    a = new QAction(tr("Export &As... (HTML)"), this);
    connect(a, SIGNAL(triggered()), this, SLOT(textSaveAs()));
    fileMenu->addAction(a);
    actionFileSaveAs = a;

    a = new QAction(tr("Export &As...(ASCII)"), this);
    a->setShortcut(Qt::ALT + Qt::Key_X);
    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    switchboard.addSwitch("textExportAsASCII", shortcutScope, a, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(textExportAsASCII()));
    fileMenu->addAction(a);
    addAction(a);
    actionFileSaveAs = a;

    fileMenu->addSeparator();
    a = new QAction(QPixmap(":/fileprint.png"), tr("&Print..."), this);
    a->setShortcut(Qt::CTRL + Qt::Key_P);
    switchboard.addSwitch("textPrint", shortcutScope, a, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(textPrint()));
    tb->addAction(a);
    fileMenu->addAction(a);
    actionFilePrint = a;

    a = new QAction(QPixmap(":/edittrash.png"), tr("&Delete All"), this);
    connect(a, SIGNAL(triggered()), this, SLOT(deleteAll()));
    fileMenu->addAction(a);
    tb->addAction(a);
    actionFileDeleteAll = a;
}

void TextEditor::setupEditActions()
{
    QString tag = tr("Texteditor", "Shortcuts");
    QToolBar *editToolBar = addToolBar(tr("Edit Actions"));
    editToolBar->setObjectName("noteEditorEditActions");
    editToolBar->hide();
    QMenu *editMenu = menuBar()->addMenu(tr("Edi&t"));

    QAction *a;
    a = new QAction(QPixmap(":/undo.png"), tr("&Undo"), this);
    a->setShortcut(Qt::CTRL + Qt::Key_Z);
    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    switchboard.addSwitch("textUndo", shortcutScope, a, tag);
    connect(a, SIGNAL(triggered()), e, SLOT(undo()));
    editMenu->addAction(a);
    editToolBar->addAction(a);
    actionEditUndo = a;

    a = new QAction(QPixmap(":/redo.png"), tr("&Redo"), this);
    a->setShortcut(Qt::CTRL + Qt::Key_Y);
    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    switchboard.addSwitch("textRedo", shortcutScope, a, tag);
    connect(a, SIGNAL(triggered()), e, SLOT(redo()));
    editMenu->addAction(a);
    editToolBar->addAction(a);
    actionEditRedo = a;

    editMenu->addSeparator();
    a = new QAction(QPixmap(), tr("Select and copy &all"), this);
    a->setShortcutContext(Qt::WidgetShortcut);
    a->setShortcut(Qt::CTRL + Qt::Key_A);
    switchboard.addSwitch("textCopyAll", shortcutScope, a, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(editCopyAll()));
    editMenu->addAction(a);

    editMenu->addSeparator();
    a = new QAction(QPixmap(":/editcopy.png"), tr("&Copy"), this);
    a->setShortcut(Qt::CTRL + Qt::Key_C);
    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    switchboard.addSwitch("textCopy", shortcutScope, a, tag);
    connect(a, SIGNAL(triggered()), e, SLOT(copy()));
    editMenu->addAction(a);
    editToolBar->addAction(a);
    actionEditCopy = a;

    a = new QAction(QPixmap(":/editcut.png"), tr("Cu&t"), this);
    a->setShortcut(Qt::CTRL + Qt::Key_X);
    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    switchboard.addSwitch("textCut", shortcutScope, a, tag);
    connect(a, SIGNAL(triggered()), e, SLOT(cut()));
    editMenu->addAction(a);
    editToolBar->addAction(a);
    actionEditCut = a;

    a = new QAction(QPixmap(":/editpaste.png"), tr("&Paste"), this);
    a->setShortcut(Qt::CTRL + Qt::Key_V);
    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    switchboard.addSwitch("textPaste", shortcutScope, a, tag);
    connect(a, SIGNAL(triggered()), e, SLOT(paste()));
    editMenu->addAction(a);
    editToolBar->addAction(a);
    actionEditPaste = a;
}

void TextEditor::setupFormatActions()
{
    QString tag = tr("Texteditor", "Shortcuts");
    fontHintsToolBar =
        addToolBar(tr("Font hints", "toolbar in texteditor"));
    fontHintsToolBar->setObjectName("noteEditorFontToolBar");
    QMenu *formatMenu = menuBar()->addMenu(tr("F&ormat"));

    QAction *a;

    a = new QAction(QPixmap(":/formatfixedfont.png"), tr("&Font hint"), this);
    a->setShortcut(Qt::CTRL + Qt::Key_H);
    a->setCheckable(true);
    a->setChecked(
        settings.value("/noteeditor/fonts/useFixedByDefault", false).toBool());
    switchboard.addSwitch("textToggleFonthint", shortcutScope, a, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(toggleFonthint()));
    formatMenu->addAction(a);
    fontHintsToolBar->addAction(a);
    actionFormatUseFixedFont = a;

    // Original icon: ./share/icons/oxygen/22x22/actions/format-text-color.png
    a = new QAction(QPixmap(":/formatrichtext.png"), tr("&Richtext"), this);
    a->setShortcut(Qt::CTRL + Qt::Key_R);
    //    a->setShortcutContext (Qt::WidgetShortcut);
    a->setCheckable(true);
    switchboard.addSwitch("textToggleRichText", shortcutScope, a, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(toggleRichText()));
    formatMenu->addAction(a);
    fontHintsToolBar->addAction(a);
    actionFormatRichText = a;

    fontToolBar = addToolBar(tr("Fonts", "toolbar in texteditor"));
    fontToolBar->setObjectName("noteEditorFontToolBar");

    comboFont = new QComboBox;
    fontToolBar->addWidget(comboFont);
    QFontDatabase fontDB;
    comboFont->insertItems(0, fontDB.families());
    connect(comboFont, SIGNAL(activated(const QString &)), this,
            SLOT(textFamily(const QString &)));

    comboSize = new QComboBox;
    fontToolBar->addWidget(comboSize);
    QList<int> sizes = fontDB.standardSizes();
    QList<int>::iterator it = sizes.begin();
    int i = 0;
    while (it != sizes.end()) {
        i++;
        ++it; // increment i before using it
        comboSize->insertItem(i, QString::number(*it));
    }
    connect(comboSize, SIGNAL(activated(const QString &)), this,
            SLOT(textSize(const QString &)));

    formatMenu->addSeparator();

    formatToolBar = addToolBar(tr("Format", "toolbar in texteditor"));
    formatToolBar->setObjectName("noteEditorFormatToolBar");

    QPixmap pix(16, 16);
    pix.fill(e->textColor());
    a = new QAction(pix, tr("&Color..."), this);
    formatMenu->addAction(a);
    formatToolBar->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(textColor()));
    actionTextColor = a;

    a = new QAction(QPixmap(":/text_bold.png"), tr("&Bold"), this);
    a->setShortcut(Qt::CTRL + Qt::Key_B);
//    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    switchboard.addSwitch("textToggleBold", shortcutScope, a, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(textBold()));
    formatToolBar->addAction(a);
    formatMenu->addAction(a);
    a->setCheckable(true);
    actionTextBold = a;

    a = new QAction(QPixmap(":/text_italic.png"), tr("&Italic"), this);
    a->setShortcut(Qt::CTRL + Qt::Key_I);
//    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    switchboard.addSwitch("textToggleItalic", shortcutScope, a, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(textItalic()));
    formatToolBar->addAction(a);
    formatMenu->addAction(a);
    a->setCheckable(true);
    actionTextItalic = a;

    a = new QAction(QPixmap(":/text_under.png"), tr("&Underline"), this);
    a->setShortcut(Qt::CTRL + Qt::Key_U);
//    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    switchboard.addSwitch("textToggleUnderline", shortcutScope, a, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(textUnderline()));
    formatToolBar->addAction(a);
    formatMenu->addAction(a);
    a->setCheckable(true);
    // richTextWidgets.append((QWidget*)a);
    actionTextUnderline = a;
    formatMenu->addSeparator();

    QActionGroup *actGrp2 = new QActionGroup(this);
    actGrp2->setExclusive(true);
    a = new QAction(QPixmap(":/text_sub.png"), tr("Subs&cript"), actGrp2);
    a->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_B);
//    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    a->setCheckable(true);
    formatToolBar->addAction(a);
    formatMenu->addAction(a);
    switchboard.addSwitch("textToggleSub", shortcutScope, a, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(textVAlign()));
    actionAlignSubScript = a;

    a = new QAction(QPixmap(":/text_super.png"), tr("Su&perscript"), actGrp2);
    a->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_P);
//    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    a->setCheckable(true);
    formatToolBar->addAction(a);
    formatMenu->addAction(a);
    switchboard.addSwitch("textToggleSuper", shortcutScope, a, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(textVAlign()));
    actionAlignSuperScript = a;
    QActionGroup *grp = new QActionGroup(this);
    connect(grp, SIGNAL(triggered(QAction *)), this,
            SLOT(textAlign(QAction *)));

    formatMenu->addSeparator();

    a = new QAction(QPixmap(":/text_left.png"), tr("&Left"), grp);
    // a->setShortcut( Qt::CTRL+Qt::Key_L );
    a->setCheckable(true);
    formatToolBar->addAction(a);
    formatMenu->addAction(a);
    actionAlignLeft = a;
    a = new QAction(QPixmap(":/text_center.png"), tr("C&enter"), grp);
    // a->setShortcut(  Qt::CTRL + Qt::Key_E);
    a->setCheckable(true);
    formatToolBar->addAction(a);
    formatMenu->addAction(a);
    actionAlignCenter = a;
    a = new QAction(QPixmap(":/text_right.png"), tr("&Right"), grp);
    // a->setShortcut(Qt::CTRL + Qt::Key_R );
    a->setCheckable(true);
    formatToolBar->addAction(a);
    formatMenu->addAction(a);
    actionAlignRight = a;
    a = new QAction(QPixmap(":/text_block.png"), tr("&Justify"), grp);
    // a->setShortcut(Qt::CTRL + Qt::Key_J );
    a->setCheckable(true);
    formatToolBar->addAction(a);
    formatMenu->addAction(a);
    actionAlignJustify = a;
}

void TextEditor::setupSettingsActions()
{
    QMenu *settingsMenu = menuBar()->addMenu(tr("&Settings"));

    QAction *a;
    a = new QAction(tr("Set &fixed font"), this);
    connect(a, SIGNAL(triggered()), this, SLOT(setFixedFont()));
    settingsMenu->addAction(a);
    actionSettingsFixedFont = a;

    a = new QAction(tr("Set &variable font"), this);
    connect(a, SIGNAL(triggered()), this, SLOT(setVarFont()));
    settingsMenu->addAction(a);
    actionSettingsVarFont = a;

    a = new QAction(tr("&fixed font is default"), this);
    a->setCheckable(true);
    // set state later in constructor...
    settingsMenu->addAction(a);
    actionSettingsFonthintDefault = a;

    settingsMenu->addSeparator();

    a = new QAction(
        tr("Set RichText default background color", "TextEditor") + "...", this);
    settingsMenu->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(setColorRichTextDefaultBackground()));
    actionFilledEditorColor = a;

    a = new QAction(tr("Set RichText default font color", "TextEditor") + "...", this);
    settingsMenu->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(setColorRichTextDefaultForeground()));
    actionFontColor = a;
}

void TextEditor::textLoad()
{
    if (state != inactiveEditor) {
        if (!isEmpty()) {
            QMessageBox mb(vymName + " - " + tr("Note Editor"),
                           "Loading will overwrite the existing note",
                           QMessageBox::Warning,
                           QMessageBox::Yes | QMessageBox::Default,
                           QMessageBox::Cancel, 0);
            mb.setButtonText(QMessageBox::Yes, "Load note");
            switch (mb.exec()) {
            case QMessageBox::Cancel:
                return;
                break;
            }
        }
        // Load note
        QFileDialog *fd = new QFileDialog(this);
        QStringList types;
        types << "Text (*.txt *.html)"
              << "VYM notes and HTML (*.html)"
              << "ASCII texts (*.txt)"
              << "All files (*)";
        fd->setNameFilters(types);
        fd->setDirectory(QDir().current());
        fd->show();
        QString fn;
        if (fd->exec() == QDialog::Accepted && !fd->selectedFiles().isEmpty())
            fn = fd->selectedFiles().first();

        if (!fn.isEmpty()) {
            QFile f(fn);
            if (!f.open(QIODevice::ReadOnly))
                return;

            QTextStream ts(&f);
            setTextAuto(ts.readAll());
            editorChanged();
        }
    }
}

void TextEditor::closeEvent(QCloseEvent *ce)
{
    ce->accept(); // TextEditor can be reopened with show()
    hide();
    emit(windowClosed());
    return;
}

bool TextEditor::eventFilter(QObject *obj, QEvent *ev)
{
    if (obj == e) {
        if (ev->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(ev);
            if (keyEvent == QKeySequence::Paste) {
                // switch editor mode to match clipboard content before pasting
                const QClipboard *clipboard = QApplication::clipboard();
                const QMimeData *mimeData = clipboard->mimeData();

                if (mimeData->hasHtml() && !actionFormatRichText->isChecked())
                    setRichTextMode(true);
            }
        }
    }
    // pass the event on to the parent class
    return QMainWindow::eventFilter(obj, ev);
}

void TextEditor::editorChanged()
{
    //qDebug() << "TE::editorChanged" << editorName << "blockChanged: " << blockChangedSignal;
    EditorState oldState = state;
    if (isEmpty())
        state = emptyEditor;
    else
        state = filledEditor;

    if (!blockChangedSignal) {
        blockTextUpdate = true;
        emit(textHasChanged(getVymText()));
        blockTextUpdate = false;
    }

    if (state == oldState)
        return;

    updateState();
}

void TextEditor::setRichText(const QString &t)
{
    blockChangedSignal = true;
    e->setReadOnly(false);
    e->setHtml(t);
    actionFormatRichText->setChecked(true);

    updateState();
    updateActions();
    blockChangedSignal = false;
}

void TextEditor::setPlainText(const QString &t)
{
    blockChangedSignal = true;
    e->setReadOnly(false);

    e->setTextColor(qApp->palette().color(QPalette::WindowText));
    e->setPlainText(t);
    actionFormatRichText->setChecked(false);

    updateState();
    updateActions();
    blockChangedSignal = false;
}

void TextEditor::setTextAuto(const QString &t)
{
    if (Qt::mightBeRichText(t))
        setRichText(t);
    else
        setPlainText(t);
}

void TextEditor::setVymText(const VymText &vt)
{
    // While a note is being edited, we are sending textHasChanged
    // Then we don't want to update the text additionally from outside,
    // as this would position cursor at beginning of text
    if (blockTextUpdate) return;

    if (vt.getText() == getText()) return;

    if (vt.isRichText())
        setRichText(vt.getText());
    else
        setPlainText(vt.getText());
}

void TextEditor::setInactive()
{
    setState(inactiveEditor);
}

void TextEditor::editCopyAll()
{
    e->selectAll();
    e->copy();
}

void TextEditor::clear()
{
    //qDebug() << "TE::clear" << editorName;
    bool blockChangedOrg = blockChangedSignal;

    blockChangedSignal = true;
    e->clear();
    setState(emptyEditor);
    //e->setTextColor(colorRichTextDefaultForeground); // FIXME-2 still needed?

    blockChangedSignal = blockChangedOrg;
}

void TextEditor::deleteAll()
{
    e->clear();
}

void TextEditor::textSaveAs()
{
    QString caption = tr("Export Note to single file");
    QString fn = QFileDialog::getSaveFileName(
        this, caption, QString(), "VYM Note (HTML) (*.html);;All files (*)",
        0, QFileDialog::DontConfirmOverwrite);

    if (!fn.isEmpty()) {
        QFile file(fn);
        if (file.exists()) {
            QMessageBox mb(
                vymName,
                tr("The file %1\nexists already.\nDo you want to overwrite it?",
                   "dialog 'save note as'")
                    .arg(fn),
                QMessageBox::Warning, QMessageBox::Yes | QMessageBox::Default,
                QMessageBox::Cancel | QMessageBox::Escape, Qt::NoButton);
            mb.setButtonText(QMessageBox::Yes, tr("Overwrite"));
            mb.setButtonText(QMessageBox::No, tr("Cancel"));
            switch (mb.exec()) {
            case QMessageBox::Yes:
                // save
                filename = fn;
                textSave();
                return;
            case QMessageBox::Cancel:
                // do nothing
                break;
            }
        }
        else {
            filename = fn;
            textSave();
            return;
        }
    }
    statusBar()->showMessage(
        tr("Couldn't export note ", "dialog 'save note as'") + fn,
        statusbarTime);
}

void TextEditor::textSave()
{
    if (filename.isEmpty()) {
        textSaveAs();
        return;
    }

    QString text = e->toHtml(); // FIXME-4 or plaintext? check...
    QFile f(filename);
    if (!f.open(QIODevice::WriteOnly)) {
        statusBar()->showMessage(QString("Could not write to %1").arg(filename),
                                 statusbarTime);
        return;
    }

    QTextStream t(&f);
    t.setCodec("UTF-8");
    t << text;
    f.close();

    e->document()->setModified(false);

    statusBar()->showMessage(QString("Note exported as %1").arg(filename),
                             statusbarTime);
}

void TextEditor::textExportAsASCII()
{
    QString fn, s;
    if (!filenameHint.isEmpty()) {
        if (!filenameHint.contains(".txt"))
            s = filenameHint + ".txt";
        else
            s = filenameHint;
    }
    else
        s = QString();
    QString caption = tr("Export Note to single file (ASCII)");
    fn = QFileDialog::getSaveFileName(
        this, caption, s, "VYM Note (ASCII) (*.txt);;All files (*)");
    int ret = -1;

    if (!fn.isEmpty()) {
        QFile file(fn);
        if (file.exists()) {
            QMessageBox mb(
                vymName,
                tr("The file %1\nexists already.\nDo you want to overwrite it?",
                   "dialog 'save note as'")
                    .arg(fn),
                QMessageBox::Warning, QMessageBox::Yes | QMessageBox::Default,
                QMessageBox::Cancel | QMessageBox::Escape, Qt::NoButton);
            mb.setButtonText(QMessageBox::Yes, tr("Overwrite"));
            mb.setButtonText(QMessageBox::No, tr("Cancel"));
            ret = mb.exec();
        }
        if (ret == QMessageBox::Cancel)
            return;

        // save
        if (!file.open(QIODevice::WriteOnly))
            statusBar()->showMessage(
                QString("Could not write to %1").arg(filename), statusbarTime);
        else {
            QTextStream t(&file);
            t << getVymText().getTextASCII();
            file.close();

            statusBar()->showMessage(QString("Note exported as %1").arg(fn),
                                     statusbarTime);
        }
    }
}

void TextEditor::textPrint()
{
    QTextDocument *document = e->document();

    if (!printer)
        mainWindow->setupPrinter();

    QPrintDialog dialog(printer, this);
    dialog.setWindowTitle(tr("Print", "TextEditor"));
    if (dialog.exec() != QDialog::Accepted)
        return;

    document->print(printer);
}

void TextEditor::textEditUndo() {}

void TextEditor::toggleFonthint()
{
    if (!actionFormatUseFixedFont->isChecked()) {
        e->setCurrentFont(varFont);
        setFont(varFont);
    }
    else {
        e->setCurrentFont(fixedFont);
        setFont(fixedFont);
    }
    emit(textHasChanged(getVymText()));
}

void TextEditor::setRichTextMode(bool b)
{
    qDebug() << "TE::setRichTextMode b=" << b;
    actionFormatUseFixedFont->setEnabled(false);
    if (b) {
        e->setHtml(e->toHtml());
        actionFormatRichText->setChecked(true);
    } else {
        // Reset also text format 
        QTextCharFormat textformat;
        textformat.setForeground(qApp->palette().color(QPalette::WindowText));
        textformat.setFont(varFont);
        e->setCurrentCharFormat(textformat);
        e->setPlainText(e->toPlainText());
        actionFormatUseFixedFont->setEnabled(true);
        actionFormatRichText->setChecked(false);
    }
    setState(state);    // re-set state to force updating colors
    updateActions();
    emit(textHasChanged(getVymText()));
}

void TextEditor::toggleRichText()
{
    if (actionFormatRichText->isChecked())
        setRichTextMode(true);
    else
        setRichTextMode(false);
}

void TextEditor::setFixedFont()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, fixedFont, this);
    if (ok)
        fixedFont = font;
}

void TextEditor::setVarFont()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, varFont, this);
    if (ok)
        varFont = font;
}

void TextEditor::textBold()
{
    if (actionTextBold->isChecked())
        e->setFontWeight(QFont::Bold);
    else
        e->setFontWeight(QFont::Normal);
}

void TextEditor::textUnderline()
{
    e->setFontUnderline(actionTextUnderline->isChecked());
}

void TextEditor::textItalic()
{
    e->setFontItalic(actionTextItalic->isChecked());
}

void TextEditor::textFamily(const QString &f) { e->setFontFamily(f); }

void TextEditor::textSize(const QString &p) { e->setFontPointSize(p.toInt()); }

void TextEditor::textColor()
{
    QColor col = QColorDialog::getColor(e->textColor(), this);
    if (!col.isValid())
        return;
    e->setTextColor(col);
    /*
    QPixmap pix( 16, 16 );
    pix.fill( col );
    actionTextColor->setIcon( pix );
    */
}

void TextEditor::textAlign(QAction *a)
{
    QTextCursor c = e->textCursor();

    if (a == actionAlignLeft)
        e->setAlignment(Qt::AlignLeft);
    else if (a == actionAlignCenter)
        e->setAlignment(Qt::AlignHCenter);
    else if (a == actionAlignRight)
        e->setAlignment(Qt::AlignRight);
    else if (a == actionAlignJustify)
        e->setAlignment(Qt::AlignJustify);
}

void TextEditor::textVAlign()
{
    QTextCharFormat format;

    if (sender() == actionAlignSuperScript &&
        actionAlignSuperScript->isChecked()) {
        format.setVerticalAlignment(QTextCharFormat::AlignSuperScript);
    }
    else if (sender() == actionAlignSubScript &&
             actionAlignSubScript->isChecked()) {
        format.setVerticalAlignment(QTextCharFormat::AlignSubScript);
    }
    else {
        format.setVerticalAlignment(QTextCharFormat::AlignNormal);
    }
    e->mergeCurrentCharFormat(format);
}

void TextEditor::fontChanged(const QFont &f)
{
    int i = comboFont->findText(f.family());
    if (i >= 0)
        comboFont->setCurrentIndex(i);
    i = comboSize->findText(QString::number(f.pointSize()));
    if (i >= 0)
        comboSize->setCurrentIndex(i);
    actionTextBold->setChecked(f.bold());
    actionTextItalic->setChecked(f.italic());
    actionTextUnderline->setChecked(f.underline());
}

void TextEditor::colorChanged(const QColor &c)
{
    QPixmap pix(16, 16);
    pix.fill(c);
    actionTextColor->setIcon(pix);
}

void TextEditor::formatChanged(const QTextCharFormat &f)
{
    if (!actionFormatRichText->isChecked())
        return;
    fontChanged(f.font());
    colorChanged(f.foreground().color());
    alignmentChanged(e->alignment());
    verticalAlignmentChanged(f.verticalAlignment());
}

void TextEditor::alignmentChanged(int a)
{
    if ((a == Qt::AlignLeft) || (a & Qt::AlignLeft))
        actionAlignLeft->setChecked(true);
    else if ((a & Qt::AlignHCenter))
        actionAlignCenter->setChecked(true);
    else if ((a & Qt::AlignRight))
        actionAlignRight->setChecked(true);
    else if ((a & Qt::AlignJustify))
        actionAlignJustify->setChecked(true);
}

void TextEditor::verticalAlignmentChanged(QTextCharFormat::VerticalAlignment a)
{
    actionAlignSubScript->setChecked(false);
    actionAlignSuperScript->setChecked(false);
    switch (a) {
    case QTextCharFormat::AlignSuperScript:
        actionAlignSuperScript->setChecked(true);
        break;
    case QTextCharFormat::AlignSubScript:
        actionAlignSubScript->setChecked(true);
        break;
    default:;
    }
}

void TextEditor::updateActions()
{
    bool b;
    b = (state == inactiveEditor) ? false : true;

    actionFileLoad->setEnabled(b);
    actionFileSave->setEnabled(b);
    actionFileSaveAs->setEnabled(b);
    actionFilePrint->setEnabled(b);
    actionFileDeleteAll->setEnabled(b);
    actionEditUndo->setEnabled(b);
    actionEditRedo->setEnabled(b);
    actionEditCopy->setEnabled(b);
    actionEditCut->setEnabled(b);
    actionEditPaste->setEnabled(b);
    actionFormatUseFixedFont->setEnabled(b);
    actionFormatRichText->setEnabled(b);

    if (!actionFormatRichText->isChecked() || !b) {
        comboFont->setEnabled(false);
        comboSize->setEnabled(false);
        fontToolBar->hide();
        formatToolBar->hide();
        actionTextColor->setEnabled(false);
        actionTextBold->setEnabled(false);
        actionTextUnderline->setEnabled(false);
        actionTextItalic->setEnabled(false);
        actionTextColor->setEnabled(false);
        actionAlignSubScript->setEnabled(false);
        actionAlignSuperScript->setEnabled(false);
        actionAlignLeft->setEnabled(false);
        actionAlignCenter->setEnabled(false);
        actionAlignRight->setEnabled(false);
        actionAlignJustify->setEnabled(false);
    }
    else {
        comboFont->setEnabled(true);
        comboSize->setEnabled(true);
        fontToolBar->show();
        formatToolBar->show();
        actionTextColor->setEnabled(true);
        actionTextBold->setEnabled(true);
        actionTextUnderline->setEnabled(true);
        actionTextItalic->setEnabled(true);
        actionTextColor->setEnabled(true);
        actionAlignSubScript->setEnabled(true);
        actionAlignSuperScript->setEnabled(true);
        actionAlignLeft->setEnabled(true);
        actionAlignCenter->setEnabled(true);
        actionAlignRight->setEnabled(true);
        actionAlignJustify->setEnabled(true);
        actionFormatUseFixedFont->setEnabled(false);
    }
}

void TextEditor::setState(EditorState s) // FIXME-2 called 12x when reselecting once in ME
                                         // 5 alone for HeadingEditor
{
    //qDebug() << "TE::setState" << s << editorName;
    QPalette p = qApp->palette();
    QColor baseColor;
    QColor windowTextColor;
    state = s;
    switch (state) {
        case emptyEditor:
        case filledEditor:
            if (actionFormatRichText->isChecked()) {
                if (useColorMapBackground)
                    baseColor = colorMapBackground;
                else
                    baseColor = colorRichTextDefaultBackground;     // FIXME-2
                                                                    // also read
                                                                    // from
                                                                    // settings
                                                                    // ?!??
            } else {
                baseColor = p.color(QPalette::Base);
            }
            windowTextColor = p.color(QPalette::WindowText);
            e->setReadOnly(false);
            break;
        case inactiveEditor:
            baseColor = Qt::black;
            e->setReadOnly(true);
    }
    p.setColor(QPalette::Base, baseColor);
    p.setColor(QPalette::WindowText, windowTextColor);
    e->setPalette(p);

    updateActions();
}

void TextEditor::updateState()
{
    //qDebug() << "TE::updateState" << editorName;
    if (isEmpty())
        setState(emptyEditor);
    else
        setState(filledEditor);
}

void TextEditor::setColorRichTextDefaultBackground()
{
    QColor col = QColorDialog::getColor(colorRichTextDefaultBackground, nullptr);
    if (!col.isValid())
        return;
    colorRichTextDefaultBackground = col;
    QPixmap pix(16, 16);
    pix.fill(colorRichTextDefaultBackground);
    actionFilledEditorColor->setIcon(pix);
}

void TextEditor::setColorRichTextDefaultForeground()
{
    QColor col = QColorDialog::getColor(colorRichTextDefaultForeground, nullptr);
    if (!col.isValid())
        return;
    colorRichTextDefaultForeground = col;
    QPixmap pix(16, 16);
    pix.fill(colorRichTextDefaultForeground);
    actionFontColor->setIcon(pix);
}

void TextEditor::setColorMapBackground(const QColor &col)
{
    colorMapBackground = col;
    QPalette p = e->palette();
    p.setColor(QPalette::Base, colorMapBackground);
    e->setPalette(p);
}

void TextEditor::setUseColorMapBackground(bool b)
{
    useColorMapBackground = b;
}