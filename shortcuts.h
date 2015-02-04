#ifndef SHORTCUTS_H
#define SHORTCUTS_H

#include <QAction>
#include <QShortcut>
#include <QString>

class KeySwitch {
public:
    KeySwitch(
            const QString &kIdentifier,
            const QString &kName,
            const QString &kGroup,
            const QKeySequence &kseq);
    QString group;
    QString name;
    QString identifier;
    QKeySequence keySequence;
};

class Switchboard {
public:
    Switchboard ();
    void addConnection(QAction *a,const QString &s);
    void addConnection(QWidget *w, QAction *a,const QString &s);
    void addGroup( QString gIdentifier, QString gName);
    void addSwitch( QString identifier, QString name, QString scope, QKeySequence ks);
    void addSwitch( QString identifier, QString scope, QAction *a);
    void addSwitch( QString identifier, QString scope, QAction *a, QString tag);
    QString getASCII();
    void printASCII();
    void printLaTeX();
protected:  
    QMultiMap <QString,QAction*> actions;
    QMultiMap <QString, KeySwitch> switches;
    QMap <QString, QString> groups;
};

#endif
