#ifndef SCRIPTEDITOR_H
#define SCRIPTEDITOR_H

#include "ui_scripteditor.h"
#include "codeeditor.h"
#include "highlighter.h"

class VymModel;

class ScriptEditor:public QWidget
{
    Q_OBJECT

public:
    enum Mode {Slide,File};
private:
    Mode mode;
    uint vymModelID;
    uint slideID;

public:
    ScriptEditor (QWidget *parent = 0);
    QString getScriptFile();
    void setSlideScript(uint vymModelID, uint slideID, const QString &);

public slots:
    void runMacro();
    void runSlide();
    void runFile();
    void saveSlide();
    void loadMacro();
    void saveMacro();
    bool loadFile(QString fn = "");
    void saveFile();
    void saveFileAs();

signals:
    void runScript (QString);
    
private:
    Ui::ScriptEditor ui;
    CodeEditor *codeEditor;
    QString filename;
    Highlighter *highlighterMacro;
    Highlighter *highlighterSlide;
    Highlighter *highlighterFile;
};


#endif 
