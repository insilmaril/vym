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
#include <QPushButton>
#include <QPrinter>
#include <QStatusBar>
#include <QToolBar>
#include <QToolButton>

#include "file.h"
#include "mainwindow.h"
#include "my-textedit.h"
#include "settings.h"
#include "shortcuts.h"
#include "url-dialog.h"

extern Main *mainWindow;
extern Settings settings;
extern QFont fixedFont;
extern QFont varFont;
extern QString iconTheme;
extern QColor vymForegroundColor;
extern QColor vymBaseColor;
extern QAction *actionViewToggleNoteEditor;

extern QString editorFocusInStyle;
extern QString editorFocusOutStyle;
extern QString toolBarStyle;

extern QString vymName;

extern Switchboard switchboard;

extern QPrinter *printer;
extern bool debug;

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

TextEditor::TextEditor(const QString &id, const QString &scope)   // FEATURE #137 insert images with drag & drop
                           // https://stackoverflow.com/questions/3254652/several-ways-of-placing-an-image-in-a-qtextedit
{
    //qDebug() << "TE::constr of " << id << scope;
    editorId = id;
    shortcutScope = scope;

    statusBar()->hide(); // Hide sizeGrip on default, which comes with statusBar

    editor = new MyTextEdit(this);
    editor->setFocus();
    editor->setTabStopDistance(20); // unit is pixel, default would be 80
    editor->setAutoFillBackground(true);
    editor->installEventFilter(this);
    connect(editor, SIGNAL(textChanged()), this, SLOT(editorChanged()));
    setCentralWidget(editor);

    connect(editor, SIGNAL(currentCharFormatChanged(const QTextCharFormat &)), this,
            SLOT(formatChanged(const QTextCharFormat &)));

    connect(editor, SIGNAL(textChanged()), this, SLOT(editorChanged()));

    connect(editor, SIGNAL(editUrlCursor(QTextCursor)), this, SLOT(insertOrEditUrl(QTextCursor)));
    setWindowIcon(QPixmap(":/vym-editor.png"));

    // Load settings
    init ();

    // Various states
    blockChangedSignal = false;
    blockTextUpdate = false;
    setInactive();

    setTitle("");

    menuBar()->setNativeMenuBar(false);
}

TextEditor::~TextEditor()
{
    // Save Settings
    QString n = QString("/satellite/%1/").arg(editorId);
    settings.setValue(n + "geometry/size", size());
    settings.setValue(n + "geometry/pos", pos());
    settings.setValue(n + "state", saveState(0));

    QString s;
    if (actionSettingsFonthintDefault->isChecked())
        s = "fixed";
    else
        s = "variable";
    settings.setValue(n + "fonts/fonthintDefault", s);
    settings.setValue(n + "fonts/varFont", varFontInt.toString());
    settings.setValue(n + "fonts/fixedFont", fixedFontInt.toString());

    settings.setValue(n + "colors/richTextEditorBackground", colorRichTextEditorBackground.name());
    settings.setValue(n + "colors/richTextBackground", colorRichTextBackground.name());
    settings.setValue(n + "colors/richTextForeground", colorRichTextForeground.name());
}

void TextEditor::init()
{
    QString n = QString("/satellite/%1/").arg(editorId);

    // Toolbars
    setupFileActions();
    setupEditActions();
    setupFormatActions();
    setupSettingsActions();

    restoreState(settings.value(n + "state", 0).toByteArray());

    colorRichTextEditorBackground = QColor::fromString(
        settings.value(n + "colors/richTextEditorBackground", vymBaseColor.name()).toString());

    colorRichTextForeground = QColor::fromString(
        settings.value(n + "colors/richTextForeground", vymForegroundColor.name()).toString());
    colorFGChanged(colorRichTextForeground);

    colorRichTextBackground = QColor::fromString(
        settings.value(n + "colors/richTextBackground", vymBaseColor.name()).toString());
    colorBGChanged(colorRichTextBackground);

    /*
    qDebug() << "TE::init" << scope;
    qDebug() << "  TEBG=" << colorRichTextEditorBackground.name() << vymBaseColor.name(); 
    qDebug() << "  RTFG=" << colorRichTextForeground.name() << vymForegroundColor.name();
    qDebug() << "  RTBG=" << colorRichTextBackground.name();
    */

    fileNameInt = "";
    fixedFontInt = fixedFont;
    varFontInt = varFont;
    QString s =
        settings.value(n + "fonts/fonthintDefault", "variable").toString();
    if (s == "fixed") {
        actionSettingsFonthintDefault->setChecked(true);
        editor->setCurrentFont(fixedFontInt);
    }
    else {
        actionSettingsFonthintDefault->setChecked(false);
        editor->setCurrentFont(varFontInt);
    }

    // Default is PlainText
    actionFormatRichText->setChecked(false);

    // Hide RichText format actions on default
    setRichTextMode(false);

    clear();
}

void TextEditor::setFocus() { editor->setFocus(); }

bool TextEditor::isEmpty()
{
    if (editor->toPlainText().length() > 0)
        return false;
    else
        return true;
}

void TextEditor::setTitle(const QString &s)
{
    QString windowTitle = (s.isEmpty()) ? shortcutScope : shortcutScope + ": " + s;

    // Set title of parent dockWidget
    if (parentWidget())
        parentWidget()->setWindowTitle(windowTitle);

    setWindowTitle(windowTitle);
}

void TextEditor::setFont(const QFont &font)
{
    blockChangedSignal = true;

    QTextCursor tc = editor->textCursor();
    QTextCharFormat format = tc.charFormat();

    tc.select(QTextCursor::Document);
    format.setFont(font);
    tc.setCharFormat(format);
    tc.clearSelection();
    fontChanged(fixedFontInt);

    blockChangedSignal = false;
}

void TextEditor::setFontHint(const QString &fh)
{
    if (fh == "fixed") {
        actionFormatUseFixedFont->setChecked(true);
        editor->setCurrentFont(fixedFontInt);
        setFont(fixedFontInt);
    }
    else {
        actionFormatUseFixedFont->setChecked(false);
        editor->setCurrentFont(varFontInt);
        setFont(varFontInt);
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

void TextEditor::setFileName(const QString &fn)
{
    fileNameInt = fn;
}

QString TextEditor::fileName() { return fileNameInt; }

void TextEditor::setFileNameHint(const QString &fnh)
{
    fileNameHintInt = fnh;
}

QString TextEditor::getText()
{
    if (editor->toPlainText().isEmpty())
        return QString();

    if (actionFormatRichText->isChecked())
        return editor->toHtml();
    else
        return editor->toPlainText();
}

VymText TextEditor::getVymText()
{
    VymText vt;

    if (actionFormatRichText->isChecked()) {
        // Remove some QTextEdit specific tags and markers from RichText
        QString t = editor->toHtml();

        // Remove <!DOCTYPE settings in the beginning
        QRegularExpression re("(<!DOCTYPE.*)<html>");
        re.setPatternOptions(
                QRegularExpression::InvertedGreedinessOption |
                QRegularExpression::DotMatchesEverythingOption |
                QRegularExpression::MultilineOption);
        t.replace(re, "<html>");

        // Remove heading with characters that might cause problems in undo scripts
        re.setPattern("<head>.*head>");
        t.replace(re, "");
        vt.setRichText(t);
    } else
        vt.setPlainText(editor->toPlainText());

    if (actionFormatUseFixedFont->isChecked())
        vt.setFontHint(getFontHint());

    return vt;
}

bool TextEditor::findText(const QString &t,
                          const QTextDocument::FindFlags &flags)
{
    if (editor->find(t, flags))
        return true;
    else
        return false;
}

bool TextEditor::findText(const QString &t,
                          const QTextDocument::FindFlags &flags, int i)
{
    // Position at beginning
    QTextCursor c = editor->textCursor();
    c.setPosition(0, QTextCursor::MoveAnchor);
    editor->setTextCursor(c);

    // Search for t
    int j = 0;
    while (j <= i) {
        if (!editor->find(t, flags))
            return false;
        j++;
    }
    return true;
}

void TextEditor::setTextCursor(const QTextCursor &cursor)
{
    editor->setTextCursor(cursor);
}

QTextCursor TextEditor::getTextCursor() { return editor->textCursor(); }

void TextEditor::setupFileActions()
{
    QToolBar *tb = addToolBar(tr("Note Actions"));
    tb->setStyleSheet(toolBarStyle);
    tb->setObjectName("noteEditorFileActions");
    QMenu *fileMenu = menuBar()->addMenu(tr("&Note", "Menubar"));

    QString tag = tr("File actions", "TextEditor shortcut groups");
    QAction *a;
    a = new QAction(QPixmap(QString(":/document-open-%1").arg(iconTheme)), tr("&Import..."), this);
    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    switchboard.addAction(a, "textLoad", Qt::CTRL | Qt::Key_O, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(textLoad()));
    tb->addAction(a);
    fileMenu->addAction(a);
    actionFileLoad = a;

    fileMenu->addSeparator();
    a = new QAction(QPixmap(QString(":/document-export-%1").arg(iconTheme)), tr("&Export..."), this);
    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    switchboard.addAction(a, "textSave", Qt::CTRL | Qt::Key_S, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(textExportAs()));
    tb->addAction(a);
    fileMenu->addAction(a);
    addAction(a);
    filledEditorActions << a;
    actionFileExport = a;

    fileMenu->addSeparator();
    a = new QAction(QPixmap(QString(":/document-print-%1.svg").arg(iconTheme)), tr("&Print..."), this);
    switchboard.addAction(a, "textPrint", Qt::CTRL | Qt::Key_P, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(textPrint()));
    tb->addAction(a);
    fileMenu->addAction(a);
    filledEditorActions << a;
    actionFilePrint = a;

    a = new QAction(QPixmap(QString(":/edit-delete-%1.svg").arg(iconTheme)), tr("&Delete All"), this);
    connect(a, SIGNAL(triggered()), this, SLOT(deleteAll()));
    fileMenu->addAction(a);
    tb->addAction(a);
    filledEditorActions << a;
    actionFileDeleteAll = a;

    a = new QAction("Close window", editor);
    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    switchboard.addAction(a, "textCloseWindow", Qt::CTRL | Qt::Key_D, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(closeWindow()));
    fileMenu->addAction(a);
    editor->addAction(a);
}

void TextEditor::setupEditActions()
{
    QString tag = tr("Edit actions", "TextEditor shortcut groups");
    QToolBar *editToolBar = addToolBar(tr("Edit Actions"));
    editToolBar->setStyleSheet(toolBarStyle);
    editToolBar->setObjectName("noteEditorEditActions");
    editToolBar->hide();
    QMenu *editMenu = menuBar()->addMenu(tr("Edi&t"));

    QAction *a;
    a = new QAction(QPixmap(":/undo.png"), tr("&Undo"), this);
    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    switchboard.addAction(a, "textUndo", Qt::CTRL | Qt::Key_Z, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), editor, SLOT(undo()));
    editMenu->addAction(a);
    editToolBar->addAction(a);
    filledEditorActions << a;  // QTextEdit does not seem to have a method to check if undo/redo is available currently
    actionEditUndo = a;

    a = new QAction(QPixmap(":/redo.png"), tr("&Redo"), this);
    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    switchboard.addAction(a, "textRedo", Qt::CTRL | Qt::Key_Y, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), editor, SLOT(redo()));
    editMenu->addAction(a);
    editToolBar->addAction(a);
    filledEditorActions << a;
    actionEditRedo = a;

    editMenu->addSeparator();
    a = new QAction(QPixmap(), tr("Select and copy &all"), this);
    a->setShortcutContext(Qt::WidgetShortcut);
    switchboard.addAction(a, "textCopyAll", Qt::CTRL | Qt::Key_A, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(editCopyAll()));
    editMenu->addAction(a);
    filledEditorActions << a;
    actionSelectAll = a;

    editMenu->addSeparator();
    a = new QAction(QPixmap(QString(":/edit-copy-%1.svg").arg(iconTheme)), tr("&Copy", "Edit menu"), this);
    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    switchboard.addAction(a, "textCopy", Qt::CTRL | Qt::Key_C, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), editor, SLOT(copy()));
    editMenu->addAction(a);
    editToolBar->addAction(a);
    filledEditorActions << a;
    actionEditCopy = a;

    a = new QAction(QPixmap(QString(":/edit-cut-%1.svg").arg(iconTheme)), tr("Cu&t", "Edit menu"), this);
    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    switchboard.addAction(a, "textCut", Qt::CTRL | Qt::Key_X, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), editor, SLOT(cut()));
    editMenu->addAction(a);
    editToolBar->addAction(a);
    filledEditorActions << a;
    actionEditCut = a;

    a = new QAction(QPixmap(QString(":/edit-paste-%1.svg").arg(iconTheme)), tr("&Paste", "Edit menu"), this);
    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    switchboard.addAction(a, "textPaste", Qt::CTRL | Qt::Key_V, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), editor, SLOT(paste()));
    editMenu->addAction(a);
    editToolBar->addAction(a);
    filledEditorActions << a;
    actionEditPaste = a;

    a = new QAction(QPixmap(QString(":/flag-url.svg")), tr("Insert or edit URL", "TextEditor") + "...", this);
    editMenu->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(insertOrEditUrl()));
    editMenu->addAction(a);
    editToolBar->addAction(a);
    filledEditorRichTextActions << a;
    actionInsertOrEditUrl = a;

    a = new QAction(QPixmap(QString(":/insert-image-%1.svg").arg(iconTheme)), tr("Insert image", "TextEditor") + "...", this);
    editMenu->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(insertImage()));
    editMenu->addAction(a);
    editToolBar->addAction(a);
    filledEditorRichTextActions << a;
    actionInsertImage = a;
}

void TextEditor::setupFormatActions()
{
    QString tag = tr("Format actions", "TextEditor shortcut groups");
    fontHintsToolBar =
        addToolBar(tr("Font hints", "toolbar in texteditor"));
    fontHintsToolBar->setStyleSheet(toolBarStyle);
    fontHintsToolBar->setObjectName("noteEditorFontToolBar");
    QMenu *formatMenu = menuBar()->addMenu(tr("F&ormat"));

    QAction *a;

    a = new QAction(QPixmap(":/formatfixedfont.png"), tr("&Font hint"), this);
    a->setCheckable(true);
    a->setChecked(
        settings.value("/noteeditor/fonts/useFixedByDefault", false).toBool());
    switchboard.addAction(a, "textToggleFonthint", Qt::CTRL | Qt::Key_H, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(toggleFonthint()));
    formatMenu->addAction(a);
    fontHintsToolBar->addAction(a);
    filledEditorActions << a;
    actionFormatUseFixedFont = a;

    // Original icon: ./share/icons/oxygen/22x22/actions/format-text-color.png
    a = new QAction(QPixmap(":/formatrichtext.svg"), tr("&Richtext"), this);
    a->setCheckable(true);
    switchboard.addAction(a, "textToggleRichText", shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(toggleRichText()));
    formatMenu->addAction(a);
    fontHintsToolBar->addAction(a);
    //filledEditorActions << a;
    actionFormatRichText = a;

    addToolBarBreak();

    fontToolBar = addToolBar(tr("Fonts", "toolbar in texteditor"));
    fontToolBar->setStyleSheet(toolBarStyle);
    fontToolBar->setObjectName("noteEditorFontToolBar");

    comboFont = new QComboBox;
    fontToolBar->addWidget(comboFont);
    comboFont->insertItems(0, QFontDatabase::families()); connect(comboFont,
            SIGNAL(currentTextChanged(const QString &)), this,
            SLOT(textFamily(const QString &)));

    comboSize = new QComboBox;
    fontToolBar->addWidget(comboSize);
    QList<int> sizes = QFontDatabase::standardSizes();
    QList<int>::iterator it = sizes.begin();
    int i = 0;
    while (it != sizes.end()) {
        i++;
        ++it; // increment i before using it
        comboSize->insertItem(i, QString::number(*it));
    }
    connect(comboSize, SIGNAL(currentTextChanged(const QString &)), this,
            SLOT(textSize(const QString &)));

    formatMenu->addSeparator();

    addToolBarBreak();

    formatToolBar = addToolBar(tr("Format", "toolbar in texteditor"));
    formatToolBar->setStyleSheet(toolBarStyle);
    formatToolBar->setObjectName("noteEditorFormatToolBar");

    a = new QAction(tr("&Color text using foreground color"), this);
    switchboard.addAction(a, "Color text with foreground color", Qt::CTRL | Qt::Key_T, shortcutScope, tag);
    formatMenu->addAction(a);
    formatToolBar->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(useTextFGColor()));
    filledEditorRichTextActions << a;
    actionUseTextFGColor = a;

    a = new QAction(tr("&Select text foreground color..."), this);
    formatMenu->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(selectTextFGColor()));
    filledEditorRichTextActions << a;
    actionSelectTextFGColor = a;

    QToolButton *tb = new QToolButton;
    tb->setArrowType(Qt::DownArrow);
    tb->setDefaultAction(actionSelectTextFGColor);
    formatToolBar->addWidget(tb);
    tb->setFixedSize(22,44);

    a = new QAction(tr("&Mark text using background color..."), this);
    switchboard.addAction(a, "Mark text using background color", Qt::CTRL | Qt::Key_M, shortcutScope, tag);
    formatMenu->addAction(a);
    formatToolBar->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(useTextBGColor()));
    filledEditorRichTextActions << a;
    actionUseTextBGColor = a;

    a = new QAction(tr("&Select text background color..."), this);
    formatMenu->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(selectTextBGColor()));
    filledEditorRichTextActions << a;
    actionSelectTextBGColor = a;

    tb = new QToolButton;
    tb->setArrowType(Qt::DownArrow);
    tb->setDefaultAction(actionSelectTextBGColor);
    formatToolBar->addWidget(tb);
    tb->setFixedSize(22,44);

    formatMenu->addSeparator();

    a = new QAction(QPixmap(QString(":/format-text-bold-%1.svg").arg(iconTheme)), tr("&Bold"), this);
//    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    switchboard.addAction(a, "textToggleBold", Qt::CTRL | Qt::Key_B, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(textBold()));
    formatToolBar->addAction(a);
    formatMenu->addAction(a);
    a->setCheckable(true);
    filledEditorRichTextActions << a;
    actionTextBold = a;

    a = new QAction(QPixmap(QString(":/format-text-italic-%1.svg").arg(iconTheme)), tr("&Italic"), this);
//    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    switchboard.addAction(a, "textToggleItalic", Qt::CTRL | Qt::Key_I, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(textItalic()));
    formatToolBar->addAction(a);
    formatMenu->addAction(a);
    a->setCheckable(true);
    filledEditorRichTextActions << a;
    actionTextItalic = a;

    a = new QAction(QPixmap(QString(":/text-format-underline-%1.svg").arg(iconTheme)), tr("&Underline"), this);
//    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    switchboard.addAction(a, "textToggleUnderline", Qt::CTRL | Qt::Key_U, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(textUnderline()));
    formatToolBar->addAction(a);
    formatMenu->addAction(a);
    filledEditorRichTextActions << a;
    a->setCheckable(true);
    // richTextWidgets.append((QWidget*)a);
    actionTextUnderline = a;
    formatMenu->addSeparator();

    QActionGroup *actGrp2 = new QActionGroup(this);
    actGrp2->setExclusive(true);
    a = new QAction(QPixmap(QString(":/text-format-subscript-%1.svg").arg(iconTheme)), tr("Subs&cript"), actGrp2);
    a->setCheckable(true);
    formatToolBar->addAction(a);
    formatMenu->addAction(a);
    switchboard.addAction(a, "textToggleSub", Qt::CTRL | Qt::SHIFT | Qt::Key_B, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(textVAlign()));
    filledEditorRichTextActions << a;
    actionAlignSubScript = a;

    a = new QAction(QPixmap(QString(":/text-format-superscript-%1.svg").arg(iconTheme)), tr("Su&perscript"), actGrp2);
    a->setCheckable(true);
    formatToolBar->addAction(a);
    formatMenu->addAction(a);
    switchboard.addAction(a, "textToggleSuper", Qt::CTRL | Qt::SHIFT | Qt::Key_P, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(textVAlign()));
    filledEditorRichTextActions << a;
    actionAlignSuperScript = a;
    QActionGroup *grp = new QActionGroup(this);
    connect(grp, SIGNAL(triggered(QAction *)), this,
            SLOT(textAlign(QAction *)));

    formatMenu->addSeparator();

    a = new QAction(QPixmap(QString(":/format-justify-left-%1.svg").arg(iconTheme)), tr("&Left"), grp);
    // a->setShortcut( Qt::CTRL+Qt::Key_L );
    a->setCheckable(true);
    formatToolBar->addAction(a);
    formatMenu->addAction(a);
    filledEditorRichTextActions << a;
    actionAlignLeft = a;
    a = new QAction(QPixmap(QString(":/format-justify-center-%1.svg").arg(iconTheme)), tr("C&enter"), grp);
    // a->setShortcut(  Qt::CTRL | Qt::Key_E);
    a->setCheckable(true);
    formatToolBar->addAction(a);
    formatMenu->addAction(a);
    filledEditorRichTextActions << a;
    actionAlignCenter = a;
    a = new QAction(QPixmap(QString(":/format-justify-right-%1.svg").arg(iconTheme)), tr("&Right"), grp);
    // a->setShortcut(Qt::CTRL | Qt::Key_R );
    a->setCheckable(true);
    formatToolBar->addAction(a);
    formatMenu->addAction(a);
    filledEditorRichTextActions << a;
    actionAlignRight = a;
    a = new QAction(QPixmap(QString(":/format-justify-fill-%1.svg").arg(iconTheme)), tr("&Justify"), grp);
    // a->setShortcut(Qt::CTRL | Qt::Key_J );
    a->setCheckable(true);
    formatToolBar->addAction(a);
    formatMenu->addAction(a);
    filledEditorRichTextActions << a;
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
        tr("Set RichText mode editor background color", "TextEditor") + "...", this);
    settingsMenu->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(selectRichTextEditorBackgroundColor()));
    actionActiveEditorBGColor = a;

    a = new QAction(tr("Set RichText mode default text color", "TextEditor") + "...", this);
    settingsMenu->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(selectRichTextForegroundColor()));
    actionRichTextFGColor = a;

    a = new QAction(tr("Set RichText mode default text background color", "TextEditor") + "...", this);
    settingsMenu->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(selectRichTextBackgroundColor()));
    actionRichTextBGColor = a;
}

void TextEditor::textLoad()
{
    if (state != inactiveEditor) {
        if (!isEmpty()) {
            QMessageBox mb(
                   QMessageBox::Warning,
                   vymName + " - " + tr("Note Editor"),
                   "Loading will overwrite the existing note");
            QPushButton *overwriteButton = mb.addButton(tr("Overwrite"), QMessageBox::AcceptRole);
            mb.addButton(tr("Cancel"), QMessageBox::RejectRole);
            mb.exec();
            if (mb.clickedButton() != overwriteButton) return;
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

bool TextEditor::eventFilter(QObject *obj, QEvent *ev)
{
    //qDebug() << "TE::eventFilter  obj=" << obj << " ev=" << ev;
    if (obj == editor) {
        if (ev->type() == QEvent::FocusIn) {
            //editor->setFrameStyle(QFrame::Box);
            editor->setStyleSheet("QTextEdit {" + editorFocusInStyle + "}");
        }
        if (ev->type() == QEvent::FocusOut) {
            editor->setFrameStyle(QFrame::NoFrame);
            editor->setStyleSheet("QTextEdit {" + editorFocusOutStyle + "}");

        }
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
        emit textHasChanged(getVymText());
        blockTextUpdate = false;
    }

    if (state == oldState)
        return;

    updateState();
}

void TextEditor::setRichText(const QString &t)
{
    blockChangedSignal = true;
    editor->setRichTextMode(true);
    editor->setReadOnly(false);
    editor->setHtml(t);
    actionFormatRichText->setChecked(true);

    // Update state including colors
    updateState();

    updateActions();
    blockChangedSignal = false;
}

void TextEditor::setPlainText(const QString &t)
{
    blockChangedSignal = true;
    editor->setRichTextMode(false);
    editor->setReadOnly(false);

    editor->setPlainText(t);
    actionFormatRichText->setChecked(false);

    // Reset also text format
    QTextCharFormat textformat;
    textformat.setForeground(vymForegroundColor);
    textformat.setFont(varFontInt);
    editor->setCurrentCharFormat(textformat);

    // Update state including colors
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
    editor->selectAll();
    editor->copy();
}

void TextEditor::clear()
{
    //qDebug() << "TE::clear" << editorName;
    bool blockChangedOrg = blockChangedSignal;

    blockChangedSignal = true;
    editor->clear();
    setState(emptyEditor);

    fileNameInt.clear();

    blockChangedSignal = blockChangedOrg;
}

void TextEditor::closeWindow()
{
    parentWidget()->hide();
    emit windowClosed();
}

void TextEditor::deleteAll()
{
    editor->clear();
}

void TextEditor::textExportAs()
{
    QString text, postfix;
    if (actionFormatRichText->isChecked()) {
        text = editor->toHtml();
        postfix = ".html";
        QString fgcol = colorRichTextForeground.name();
        QString bgcol = colorRichTextEditorBackground.name();
        text.replace("white-space: pre-wrap;", "white-space: pre-wrap; color:" + fgcol + ";");
        text.replace("<body style=\"","<body style=\"background-color:" + bgcol + "; ");
    } else {
        text = editor->toPlainText();
        postfix = ".txt";
    }

    QString fn;
    if (fileNameInt.isEmpty())
        fn = fileNameHintInt + postfix;
    else
        fn = fileNameInt;

    QString caption = tr("Export Note to single file");

    fn = QFileDialog::getSaveFileName(
        this, caption, fn, "VYM Note (HTML) (*.html);VYM Note (Text) (*.txt);All files (*)",
        0, QFileDialog::DontConfirmOverwrite);

    if (!fn.isEmpty()) {

// Macs check for replacing existing file in native dialog
#ifndef Q_OS_MACOS
        QFile file(fn);
        if (file.exists()) {
            QMessageBox mb(
                QMessageBox::Warning,
                vymName,
                tr("The file %1\nexists already.\nDo you want to overwrite it?",
                   "dialog 'save note as'").arg(fn));
            QPushButton *overwriteButton = mb.addButton(tr("Overwrite"), QMessageBox::AcceptRole);
            mb.addButton(tr("Cancel"), QMessageBox::RejectRole);
            mb.exec();
            if (mb.clickedButton() != overwriteButton) return;
        }
#endif

        fileNameInt = fn;

        QFile f(fileNameInt);
        if (!f.open(QIODevice::WriteOnly)) {
            mainWindow->statusMessage(QString("Could not write to %1").arg(fileNameInt));
            return;
        }

        QTextStream t(&f);
        t << text;
        f.close();

        mainWindow->statusMessage(QString("Note exported as %1").arg(fileNameInt));
    }
}

void TextEditor::textPrint()
{
    QTextDocument *document = editor->document();

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
        editor->setCurrentFont(varFontInt);
        setFont(varFontInt);
    }
    else {
        editor->setCurrentFont(fixedFontInt);
        setFont(fixedFontInt);
    }
    emit textHasChanged(getVymText());
}

bool TextEditor::richTextMode()
{
    return editor->richTextMode();
}

void TextEditor::setRichTextMode(bool b)
{
    if (b) {
        setRichText(editor->toHtml());

        // Use default foreground color for all text when switching to RichText
        QTextCursor cursor = editor->textCursor();
        editor->selectAll();
        editor->setTextColor(colorRichTextForeground);
        editor->setTextBackgroundColor(colorRichTextBackground);
        editor->setTextCursor(cursor);
    } else {
        setPlainText(editor->toPlainText());
        QTextCursor cursor = editor->textCursor();
        editor->selectAll();
        editor->setTextColor(qApp->palette().color(QPalette::WindowText));
        editor->setTextBackgroundColor(QColor::fromString("00000000"));
        editor->setTextCursor(cursor);
    }
    emit textHasChanged(getVymText());
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
    QFont font = QFontDialog::getFont(&ok, fixedFontInt, this);
    if (ok)
        fixedFontInt = font;
}

void TextEditor::setVarFont()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, varFontInt, this);
    if (ok)
        varFontInt = font;
}

void TextEditor::textBold()
{
    if (actionTextBold->isChecked())
        editor->setFontWeight(QFont::Bold);
    else
        editor->setFontWeight(QFont::Normal);
}

void TextEditor::textUnderline()
{
    editor->setFontUnderline(actionTextUnderline->isChecked());
}

void TextEditor::textItalic()
{
    editor->setFontItalic(actionTextItalic->isChecked());
}

void TextEditor::textFamily(const QString &f) { editor->setFontFamily(f); }

void TextEditor::textSize(const QString &p) { editor->setFontPointSize(p.toInt()); }

void TextEditor::useTextFGColor()
{
    editor->setTextColor( colorRichTextForeground);
}

void TextEditor::selectTextFGColor()
{
    QColor col = QColorDialog::getColor(
            editor->textColor(),
            this,
            tr("Text color","TextEditor windows"),
            QColorDialog::ShowAlphaChannel);
    if (!col.isValid())
        return;
    editor->setTextColor(col);
    colorFGChanged(col);
    colorRichTextForeground = col;
}

void TextEditor::useTextBGColor()
{
    editor->setTextBackgroundColor(colorRichTextBackground);
}

void TextEditor::selectTextBGColor()
{
    QColor col = QColorDialog::getColor(
            editor->textBackgroundColor(),
            this,
            tr("Text background color","TextEditor windows"),
            QColorDialog::ShowAlphaChannel);
    if (!col.isValid())
        return;

    colorRichTextBackground = col;
    colorBGChanged(col);
    editor->setTextBackgroundColor(col);
}

void TextEditor::textAlign(QAction *a)
{
    QTextCursor c = editor->textCursor();

    if (a == actionAlignLeft)
        editor->setAlignment(Qt::AlignLeft);
    else if (a == actionAlignCenter)
        editor->setAlignment(Qt::AlignHCenter);
    else if (a == actionAlignRight)
        editor->setAlignment(Qt::AlignRight);
    else if (a == actionAlignJustify)
        editor->setAlignment(Qt::AlignJustify);
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
    editor->mergeCurrentCharFormat(format);
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

void TextEditor::colorFGChanged(const QColor &c)    // FIXME-0 see also setRichTextForegroundColor
{
    QImage image(":color-text.svg");
    QPainter painter;
    painter.begin(&image);
    painter.setBrush(c);
    painter.drawRect(0,110,128,128);
    painter.end();

    actionUseTextFGColor->setIcon(QPixmap::fromImage(image));
}

void TextEditor::colorBGChanged(const QColor &c)
{
    QImage image(":draw-brush.svg");
    QPainter painter;
    painter.begin(&image);
    painter.setBrush(c);
    painter.drawRect(0,110,128,128);
    painter.end();

    actionUseTextBGColor->setIcon(QPixmap::fromImage(image));
}

void TextEditor::formatChanged(const QTextCharFormat &f)
{
    //qDebug() << "TE::formatChanged  fg=" << f.foreground().color() << " bg=" << f.background().color() << " valid=" << f.isValid();
    if (!actionFormatRichText->isChecked())
        return;
    fontChanged(f.font());
    // colorFGChanged(f.foreground().color()); // FIXME-0
    // colorBGChanged(f.background().color()); // FIXME-0
    alignmentChanged(editor->alignment());
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
    if (state == inactiveEditor) {
        actionFileLoad->setEnabled(false);
        foreach (QAction* a, filledEditorActions)
            a->setEnabled(false);
        foreach (QAction* a, filledEditorActions)
            a->setEnabled(false);
        foreach (QAction* a, filledEditorRichTextActions)
            a->setEnabled(false);

        fontToolBar->hide();
        formatToolBar->hide();
        actionFormatUseFixedFont->setEnabled(false);
        actionFormatRichText->setEnabled(false);
        return;
    }

    actionFileLoad->setEnabled(true);

    // editorState is filledEditor or emptyEditor
    foreach (QAction* a, emptyEditorActions)
        a->setEnabled(true);

    bool b = (state == filledEditor) ? true : false;
    foreach (QAction* a, filledEditorActions)
        a->setEnabled(b);

    b = (state == filledEditor && actionFormatRichText->isChecked()) ? true : false;
    foreach (QAction* a, filledEditorRichTextActions)
    {
        a->setEnabled(b); // FIXME-3  Only on Mac not greyed out when disabled. Qt bug? 
        // qDebug() << "Setting action " << a << " to " << b;
    }

    actionFormatRichText->setEnabled(true);
    if (richTextMode()) {
        actionFormatUseFixedFont->setEnabled(false);    // FIXME-3 Maybe even hide it in RT mode
        fontToolBar->show();
        formatToolBar->show();
    } else {
        actionFormatUseFixedFont->setEnabled(true);
        fontToolBar->hide();
        formatToolBar->hide();
    }
}

void TextEditor::setState(EditorState s)
{
    // qDebug() << "TE::setState" << s;
    QPalette p = qApp->palette();
    QColor baseColor;
    state = s;
    switch (state) {
        case emptyEditor:
            if (actionFormatRichText->isChecked()) {
                editor->setTextColor(colorRichTextForeground);
                editor->setTextBackgroundColor(colorRichTextBackground);
	    } else
                editor->setTextColor(p.color(QPalette::Text));

        case filledEditor:
            if (actionFormatRichText->isChecked()) {
                if (useColorMapBackground)
                    baseColor = colorMapBackground;
                else
                    baseColor = colorRichTextEditorBackground;
            } else {
                baseColor = vymBaseColor;
            }
            editor->setReadOnly(false);
            break;
        case inactiveEditor:
            baseColor = Qt::black;
            editor->setReadOnly(true);
    }

    // Just setting base color sometimes seems not enough...
    p.setColor(QPalette::Base, baseColor);
    p.setColor(QPalette::Window, baseColor);
    editor->setPalette(p);

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

void TextEditor::selectRichTextEditorBackgroundColor()
{
    QColor col = QColorDialog::getColor(
            colorRichTextEditorBackground,
            nullptr,
            tr("Text editor background color","TextEditor windows"),
            QColorDialog::ShowAlphaChannel);
    if (!col.isValid())
        return;
    colorRichTextEditorBackground = col;
    QPixmap pix(16, 16);
    pix.fill(colorRichTextEditorBackground);
    actionActiveEditorBGColor->setIcon(pix);

    // Update color
    setState(state);
}

void TextEditor::selectRichTextForegroundColor()
{
    QColor col = QColorDialog::getColor(
            colorRichTextForeground,
            nullptr,
            tr("Text editor default text color","TextEditor windows"),
            QColorDialog::ShowAlphaChannel);
    setRichTextForegroundColor(col);
}

void TextEditor::selectRichTextBackgroundColor()
{
    QColor col = QColorDialog::getColor(
            colorRichTextBackground,
            nullptr,
            tr("Text editor default text background color","TextEditor windows"),
            QColorDialog::ShowAlphaChannel);
    setRichTextBackgroundColor(col);
}

void TextEditor::insertOrEditUrl()
{
    insertOrEditUrl(editor->textCursor());
}

void TextEditor::insertOrEditUrl(QTextCursor cursor)
{
    QTextCharFormat fmt = cursor.charFormat();
    QString url;
    QString text;
    bool edit = false;
    int anchorStart;
    int anchorEnd;

    if (fmt.isAnchor()) {
        QString plainText = editor->toPlainText();
        url = fmt.anchorHref();
        edit = true;

        // Find beginning of URL in block
        int pos = cursor.position();
        int pos_org = pos;
        anchorStart = pos;
        while (!cursor.atBlockStart() && pos == anchorStart) {
            pos--;
            cursor.setPosition(pos);
            if (cursor.charFormat().isAnchor())
                anchorStart = pos;
        }

        // Find end of URL in block
        pos = pos_org;
        anchorEnd = pos;
        while (!cursor.atBlockEnd() && pos == anchorEnd) {
            pos++;
            cursor.setPosition(pos);
            if (cursor.charFormat().isAnchor())
                anchorEnd = pos;
        }
        text = plainText.slice(anchorStart - 1, anchorEnd - anchorStart + 1);
    }

    UrlDialog dia (this);
    dia.setUrl(url);
    dia.setText(text);
    if (dia.exec()) {
        url = dia.url();
        fmt = cursor.charFormat();
        if (!url.isEmpty()) {
            fmt.setAnchor(true);
            fmt.setAnchorHref(url);
            fmt.setFontUnderline(true);
        } else {
            fmt.setAnchor(false);
            fmt.setFontUnderline(false);
        }

        if (edit) {
            cursor.setPosition(anchorStart - 1);
            cursor.movePosition(
                    QTextCursor::NextCharacter, 
                    QTextCursor::KeepAnchor, 
                    anchorEnd - anchorStart + 1);
            cursor.removeSelectedText();
            cursor.insertText(dia.text(), fmt);
        } else
            cursor.insertText(dia.text(), fmt);
    }
}

void TextEditor::insertImage()
{
    QStringList imagePaths = openImageDialog(tr("Load image", "TextEditor"));

    foreach (QString path, imagePaths) {
	QUrl Uri ( QString ( "file://%1" ).arg (path));
	QImage image = QImageReader (path).read();

	QBuffer buffer;
	buffer.open(QIODevice::WriteOnly);
	image.save(&buffer, "PNG");
	QString encodedImage = buffer.data().toBase64();

	QTextDocument * textDocument = editor->document();
	textDocument->addResource( QTextDocument::ImageResource, Uri, QVariant (image));
	QTextCursor cursor = editor->textCursor();
	QTextImageFormat imageFormat;
	imageFormat.setWidth(image.width());
	imageFormat.setHeight(image.height());
	imageFormat.setName(Uri.toString());
	//cursor.insertImage(imageFormat);
	cursor.insertHtml("<img src=\"data:image;base64," + encodedImage + "\"/>");
    }
}

void TextEditor::setRichTextForegroundColor(const QColor &col)
{
    if (!col.isValid()) return;

    colorRichTextForeground = col;
    QPixmap pix(16, 16);
    pix.fill(colorRichTextForeground);
    actionRichTextFGColor->setIcon(pix);
}

void TextEditor::setRichTextBackgroundColor(const QColor &col)
{
    if (!col.isValid()) return;

    colorRichTextBackground = col;
    QPixmap pix(16, 16);
    pix.fill(colorRichTextBackground);
    actionRichTextBGColor->setIcon(pix);
}

void TextEditor::setMapBackgroundColor(const QColor &col)
{
    colorMapBackground = col;
}

void TextEditor::setUseMapBackgroundColor(bool b)
{
    useColorMapBackground = b;
}
