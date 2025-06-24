#ifndef TEXTEDITOR_H
#define TEXTEDITOR_H

#include <QMainWindow>
#include <QString>
#include <QtGui>
#include "vymtext.h"
class QTextEdit;
class QComboBox;

enum EditorState { inactiveEditor, emptyEditor, filledEditor };

class TextEditor : public QMainWindow {
    Q_OBJECT
  public:
    TextEditor(const QString eName = "undefinedEditorName");
    ~TextEditor();

    void init(const QString &ename);
    bool isEmpty();
    void setEditorName(const QString &);
    void setEditorTitle(const QString &t = "");
    QString getEditorTitle();
    void setFont(const QFont &);
    void setFontHint(const QString &);
    QString getFontHint();
    QString getFontHintDefault();
    void setFilename(const QString &);
    QString getFilename();
    void setFilenameHint(const QString &);
    QString getFilenameHint();
    QString getText();
    VymText getVymText();

    bool findText(const QString &, const QTextDocument::FindFlags &);
    bool findText(const QString &, const QTextDocument::FindFlags &, int i);
    void setTextCursor(const QTextCursor &cursor);
    QTextCursor getTextCursor();
    void setFocus();

  protected:
    void setupFileActions();
    void setupEditActions();
    void setupFormatActions();
    void setupSettingsActions();
    void closeEvent(QCloseEvent *);
    bool eventFilter(QObject *obj, QEvent *ev);

  public slots:
    void editorChanged(); // received when text() changed
    void setRichText(const QString &t);
    void setPlainText(const QString &t);
    void setTextAuto(const QString &); // set Text and autodetect mode
    void setVymText(const VymText &vt);
    void setInactive(); // Nothing can be entered
    void editCopyAll();
    void clear();

  protected slots:
    void deleteAll();

  signals:
    void textHasChanged(const VymText &vt);
    void windowClosed();
    void fontFamilyHasChanged();
    void fontSizeHasChanged();

  private slots:
    void textLoad();
    void textExportAs();
    void textExport();
    void textExportText();
    void textPrint();
    void textEditUndo();
    void toggleFonthint();
    void setRichTextMode(bool b);
    void toggleRichText();
    void setFixedFont();
    void setVarFont();
    void textBold();
    void textUnderline();
    void textItalic();
    void textFamily(const QString &f);
    void textSize(const QString &p);
    void selectTextFGColor();
    void selectTextBGColor();
    void textAlign(QAction *);
    void textVAlign();
    void fontChanged(const QFont &f);
    void colorFGChanged(const QColor &c);
    void colorBGChanged(const QColor &c);
    void formatChanged(const QTextCharFormat &f);
    void alignmentChanged(int a);
    void verticalAlignmentChanged(QTextCharFormat::VerticalAlignment);
    void updateActions();
    void setState(EditorState);
    void updateState();
    void selectRichTextEditorBackgroundColor();
    void selectRichTextForegroundColor();
    void selectRichTextBackgroundColor();
    void insertImage();

  public:
    void setRichTextEditorBackgroundColor(const QColor &);
    void setRichTextForegroundColor(const QColor &);
    void setRichTextBackgroundColor(const QColor &);
    void setMapBackgroundColor(const QColor&);
    void setUseMapBackgroundColor(bool);

  protected:
    QString shortcutScope; // used for settings and shortcut scopes
    QTextEdit *editor;
    QPoint lastPos;     // save last position of window
    QString editorName; // Name of editor, e.g. note editor, heading editor, ...
    QString editorTitle; // window title: Editor name + selected branch
    QString filename;
    QString filenameHint;

    EditorState state;
    bool blockChangedSignal;
    bool blockTextUpdate;       // Set *while* textHasChanged is still being emitted

    QColor colorRichTextEditorBackground;
    QColor colorRichTextBackground;
    QColor colorRichTextForeground;
    QColor colorMapBackground;
    bool useColorMapBackground;

    QFont varFontInt;
    QFont fixedFontInt;
    QComboBox *comboFont, *comboSize;

    QToolBar *editToolBar;
    QToolBar *fontToolBar;
    QToolBar *fontHintsToolBar;
    QToolBar *formatToolBar;

    // Filled editor only actions
    QList <QAction*> filledEditorActions;

    // Filled editor RichText actions
    QList <QAction*> filledEditorRichTextActions;

    // Empty editor actions
    QList <QAction*> emptyEditorActions;

    QAction *actionFileLoad,
        *actionFileExport, *actionFileExportHtml, *actionFileExportText,
        *actionFilePrint, *actionFileDeleteAll, *actionEditUndo,
        *actionEditRedo, *actionEditCopy, *actionEditCut, *actionEditPaste,
        *actionSelectAll,
	*actionInsertImage,
        *actionFormatUseFixedFont, *actionFormatRichText,
        *actionSettingsVarFont, *actionSettingsFixedFont,
        *actionSettingsFonthintDefault, *actionEmptyEditorBGColor,
        *actionActiveEditorBGColor, *actionInactiveEditorBGColor,
	*actionRichTextFGColor,
	*actionRichTextBGColor;

    QAction
        *actionTextFGColor,
        *actionTextBGColor,
	*actionTextBold, *actionTextUnderline, *actionTextItalic,
	*actionAlignSubScript, *actionAlignSuperScript,
        *actionAlignLeft, *actionAlignCenter, *actionAlignRight,
        *actionAlignJustify;
};

#endif
