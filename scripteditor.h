#ifndef SCRIPTEDITOR_H
#define SCRIPTEDITOR_H

#include "ui_scripteditor.h"
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
    void setScriptFile (const QString &fn);
    void saveFile();
    void setSlideScript(uint vymModelID, uint slideID, const QString &);

public slots:
    void saveSlide();
    void saveClicked();
    void saveAsClicked();
    void openClicked();
    void runClicked();
    void loadMacroClicked();
    void saveMacroClicked();

signals:
    void runScript (QString);
    
private:
    Ui::ScriptEditor ui;
    QString filename;
    Highlighter *highlighter;
};


#endif 
