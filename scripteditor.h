#ifndef SCRIPTEDITOR_H
#define SCRIPTEDITOR_H

#include "ui_scripteditor.h"

#include "highlighter.h"

class ScriptEditor:public QWidget
{
    Q_OBJECT

public:
    ScriptEditor (QWidget* parent = 0);
    void saveScript ();
    void setScript(const QString &);

public slots:
    void saveClicked();
    void saveAsClicked();
    void openClicked();
    void runClicked();

signals:
    void runScript (QString);
    
private:
    Ui::ScriptEditor ui;
    QString filename;
    Highlighter *highlighter;
};


#endif 
