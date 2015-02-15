#ifndef SHORTCUTS_H
#define SHORTCUTS_H

#include <QAction>
#include <QShortcut>
#include <QString>

class KeySwitch {
public:
    KeySwitch(
            const QString &kIdentifier, //! Unique identifier (still unused)
            const QString &kName,       //! text saved in related action (translated)
            const QString &kGroup,      //! Scope
            const QString &kTag,        //! Tag, used for listing related shortcuts
            const QKeySequence &kseq);  //! Keysequence from action
    QString group;
    QString name;
    QString identifier;
    QString tag;
    QKeySequence keySequence;
};

class Switchboard {
public:
    Switchboard ();
    void addGroup( QString gIdentifier, QString gName);
    void addSwitch( QString identifier, QString scope, QAction *a, QString tag);
    QString getASCII();
    void printASCII();
    void printLaTeX();
protected:  
    QMultiMap <QString,QAction*> actions;
    QMultiMap <QString, KeySwitch> switches;
    QMap <QString, QString> groups;
    QStringList tags;
};

#endif
