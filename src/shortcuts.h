#ifndef SHORTCUTS_H
#define SHORTCUTS_H

#include <QAction>
#include <QShortcut>
#include <QString>

class KeySwitch {
  public:
    KeySwitch(
        const QString &identifier,  //! Unique identifier (still unused)
        const QString &scope,       //! Scope
        const QString &tag,         //! Tag, used for listing related shortcuts
        QAction* action);     //! Action 
    QString scopeInt;
    QString nameInt;
    QString identifierInt;
    QString tagInt;
    QKeySequence keySequence;
    QAction *actionInt;
};

class Switchboard {
  public:
    Switchboard();
    void addScope(QString gIdentifier, QString gName);
    void addSwitch(const QString &identifier, const QString &scope, const QString &tag, QAction *a);
    QString getASCII();
    void printASCII();
    void printLaTeX();

  protected:
    QMultiMap<QString, KeySwitch> switchesMap;
    QMap<QString, QString> scopesMap;   // Hash with translated names of scopes
};

#endif
