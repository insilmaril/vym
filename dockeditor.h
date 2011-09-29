#ifndef DOCKEDITOR_H 
#define DOCKEDITOR_H

#include <QDockWidget>

class VymModel;

class DockEditor:public QDockWidget
{
    Q_OBJECT
public:
    DockEditor ();
    DockEditor (QString title, QWidget *p=0, VymModel *m=0);
    void init();

public slots:
    void changeTopLevel( bool topLevel);
private:
    QString editorTitle;
    VymModel *model;
};


#endif

