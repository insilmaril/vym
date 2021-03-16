#ifndef TEXTEDITOR_H
#define TEXTEDITOR_H

#include <QMainWindow>
#include <QtGui>

class QTextEdit;
class QComboBox;

#include "vymtext.h"

enum EditorState { inactiveEditor, emptyEditor, filledEditor };

class TextEditor : public QMainWindow {
    Q_OBJECT
  public:
    TextEditor();
    ~TextEditor();

    void init(const QString &ename);
    bool isEmpty();
    void setEditorName(const QString &);
    void setEditorTitle(const QString &);
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

  signals:
    void textHasChanged(const VymText &vt);
    void windowClosed();
    void fontFamilyHasChanged();
    void fontSizeHasChanged();

  private slots:
    void textLoad();
    void textSaveAs();
    void textSave();
    void textExportAsASCII();
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
    void textColor();
    void textAlign(QAction *);
    void textVAlign();
    void fontChanged(const QFont &f);
    void colorChanged(const QColor &c);
    void formatChanged(const QTextCharFormat &f);
    void alignmentChanged(int a);
    void verticalAlignmentChanged(QTextCharFormat::VerticalAlignment);
    void updateActions();
    void setState(EditorState);
    void updateState();
    void setEmptyEditorColor();
    void setInactiveEditorColor();
    void setFilledEditorColor();
    void setFontColor();

  protected:
    QString shortcutScope; // used for settings and shortcut scopes
    QTextEdit *e;
    QPoint lastPos;     // save last position of window
    QString editorName; // Name of editor, e.g. note editor, heading editor, ...
    QString editorTitle; // window title: Editor name + selected branch
    QString filename;
    QString filenameHint;

    QBrush emptyPaper;    // setting the background color
    QBrush filledPaper;   // depending on the state
    QBrush inactivePaper; // depending on the state
    EditorState state;
    bool blockChangedSignal;
    bool blockTextUpdate;       // Set *while* textHasChanged is still being emitted

    QColor colorEmptyEditor;
    QColor colorFilledEditor;
    QColor colorInactiveEditor;
    QColor colorFont;

    QFont varFont;
    QFont fixedFont;
    QComboBox *comboFont, *comboSize;

    QToolBar *editToolBar;
    QToolBar *fontToolBar;
    QToolBar *fontHintsToolBar;
    QToolBar *formatToolBar;

    QAction *actionFileLoad, *actionFileSave, *actionFileSaveAs,
        *actionFilePrint, *actionFileDeleteAll, *actionEditUndo,
        *actionEditRedo, *actionEditCopy, *actionEditCut, *actionEditPaste,
        *actionFormatUseFixedFont, *actionFormatRichText,
        *actionSettingsVarFont, *actionSettingsFixedFont,
        *actionSettingsFonthintDefault, *actionEmptyEditorColor,
        *actionFilledEditorColor, *actionInactiveEditorColor, *actionFontColor;

    QAction *actionTextBold, *actionTextUnderline, *actionTextItalic,
        *actionTextColor, *actionAlignSubScript, *actionAlignSuperScript,
        *actionAlignLeft, *actionAlignCenter, *actionAlignRight,
        *actionAlignJustify;
};

#endif
