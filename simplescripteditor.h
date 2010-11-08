#ifndef SIMPLESCRIPTEDITOR_H
#define SIMPLESCRIPTEDITOR_H

#include "ui_simplescripteditor.h"

#include "highlighter.h"

class SimpleScriptEditor:public QDialog
{
    Q_OBJECT

public:
    SimpleScriptEditor (QWidget* parent = 0);
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
    Ui::SimpleScriptEditor ui;
    QString filename;
    Highlighter *highlighter;
};


#endif 
