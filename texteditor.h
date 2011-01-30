#ifndef TEXTEDITOR_H 
#define TEXTEDITOR_H

#include <QtGui>

enum EditorState {inactiveEditor,emptyEditor,filledEditor};

class TextEditor :public QMainWindow
{    Q_OBJECT
public:
    TextEditor();
    ~TextEditor();

    void reset();
    bool isEmpty();
    void setShowWithMain (bool);
    bool showWithMain ();
    void setFontHint(const QString&);
    QString getFontHint();
    QString getFontHintDefault();
    void setFilename (const QString&);
    QString getFilename ();
    void setFilenameHint (const QString&);
    QString getFilenameHint ();
    QString getText();

    bool findText(const QString &, const QTextDocument::FindFlags &); 
    bool findText(const QString &, const QTextDocument::FindFlags &,int i); 
    void setTextCursor (const QTextCursor & cursor );
    QTextCursor getTextCursor();

protected:
    void setupFileActions();
    void setupEditActions();
    void setupFormatActions();
    void setupSettingsActions();
    void closeEvent( QCloseEvent* );

public slots:
    void editorChanged();	    // received when text() changed
    void setText(const QString &);  // set Text (by MapEditor)
    void setInactive();		    // Nothing can be entered
    void editCopyAll();

signals:
    void textHasChanged();
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
    void toggleRichText();
    void setFixedFont();
    void setVarFont();
    void textBold();
    void textUnderline();
    void textItalic();
    void textFamily( const QString &f );
    void textSize( const QString &p );
    void textColor();
    void textAlign(QAction*);
    void textVAlign();
    void fontChanged( const QFont &f );
    void colorChanged( const QColor &c );
    void formatChanged (const QTextCharFormat &f);
    void alignmentChanged( int a );
    void verticalAlignmentChanged(QTextCharFormat::VerticalAlignment);
    void updateActions();
    void setState (EditorState);

protected:
    QTextEdit *e;
    QPoint lastPos;	    // save last position of window
    QString filename;
    QString filenameHint;

    QBrush emptyPaper;	    // setting the background color
    QBrush filledPaper;	    // depending on the state
    QBrush inactivePaper;   // depending on the state
    EditorState state;
    bool showwithmain;	    // same visibility as mainwindow?
    bool blockChangedSignal;

    QFont varFont;
    QFont fixedFont;
    QComboBox *comboFont, *comboSize;
    
    QToolBar *editToolBar;
    QAction *actionFileLoad,
    *actionFileSave,
    *actionFileSaveAs,
    *actionFilePrint,
    *actionFileDeleteAll,
    *actionEditUndo,
    *actionEditRedo,
    *actionEditCopy,
    *actionEditCut,
    *actionEditPaste,
    *actionFormatUseFixedFont,
    *actionFormatRichText,
    *actionSettingsVarFont,
    *actionSettingsFixedFont,
    *actionSettingsFonthintDefault;

    QAction *actionTextBold,
    *actionTextUnderline,
    *actionTextItalic,
    *actionTextColor,
    *actionAlignSubScript,
    *actionAlignSuperScript,
    *actionAlignLeft,
    *actionAlignCenter,
    *actionAlignRight,
    *actionAlignJustify;
};

#endif
