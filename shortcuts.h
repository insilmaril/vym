#ifndef SHORTCUTS_H
#define SHORTCUTS_H

#include <QAction>
#include <QShortcut>
#include <QString>

class Shortcut:public QShortcut {
public:
    Shortcut(QWidget *parent=NULL);
    void setGroup(const QString &);
    QString getGroup ();
    void setContextName (const QString &);
    QString getContextName();

protected:
    QString group;
    QString context;
};

class Switchboard {
public:
    Switchboard ();
    void addConnection(QAction *a,const QString &s);
    void addConnection(QWidget *w, QAction *a,const QString &s);
    QString getASCII();
    void printASCII();
    void printLaTeX();
protected:  
    QMultiMap <QString,QAction*> actions;
};

#endif
