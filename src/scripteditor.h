#ifndef SCRIPTEDITOR_H
#define SCRIPTEDITOR_H

#include "codeeditor.h"
#include "highlighter.h"
#include "ui_scripteditor.h"

class VymModel;

class ScriptEditor : public QWidget {
    Q_OBJECT

  public:
    enum Mode { Slide, File };

  private:
    Mode mode;
    uint vymModelID;
    uint slideID;

  public:
    ScriptEditor(QWidget *parent = 0);
    QString getScriptFile();
    void setSlideScript(uint vymModelID, uint slideID, const QString &);

  public slots:
    void runMacro();
    void runSlide();
    void runScript();
    void saveSlide();
    void reloadMacros();
    void saveMacros();
    bool loadScript(QString fn = "");
    bool reloadScript();
    void saveScript();
    void saveScriptAs();

  signals:
    void runScript(QString);

  private:
    Ui::ScriptEditor ui;
    CodeEditor *slideEditor;
    CodeEditor *macroEditor;
    CodeEditor *codeEditor;
    QString filename;
    Highlighter *highlighterMacro;
    Highlighter *highlighterSlide;
    Highlighter *highlighterFile;
};

#endif
