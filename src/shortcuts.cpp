#include <QDebug>
#include <QMultiMap>

#include <iostream>

#include "shortcuts.h"

/////////////////////////////////////////////////////////////////
// KeySwitch
/////////////////////////////////////////////////////////////////
KeySwitch::KeySwitch(const QString &identifier,
                     const QString &scope,
                     const QString &tag,
                     QAction* action)
{
    identifierInt = identifier;
    scopeInt = scope;
    tagInt = tag;
    actionInt = action;
}

/////////////////////////////////////////////////////////////////
// Switchboard
/////////////////////////////////////////////////////////////////
Switchboard::Switchboard() {}

void Switchboard::addScope(QString scopeIdentifier, QString scopeName)
{
    if (scopesMap.contains(scopeIdentifier)) {
        qDebug() << "Warning switchboard: Shortcut scope " << scopeIdentifier
                 << " already exists";
        return;
    }
    scopesMap.insert(scopeIdentifier, scopeName);
}

void Switchboard::addSwitch(const QString &identifier,
        const QString &scope,
        const QString &tag,
        QAction *action)
{
    if (!switchesMap.contains(identifier)) {
        KeySwitch ksw(identifier, scope, tag, action);
        switchesMap.insert(scope, ksw);
    }
    else
        qDebug()
            << "Warning switchboard::addSwitch warning: Existing idenifier "
            << identifier;
}

QString Switchboard::getASCII()
{
    QString s;
    QString scope;
    foreach (scope, switchesMap.uniqueKeys()) {
        s += "Scope " + scope + ":\n";
        QList<KeySwitch> values = switchesMap.values(scope);
        for (int i = 0; i < values.size(); ++i) {
            QString desc = values.at(i).actionInt->text();
            QString sc = values.at(i).actionInt->shortcut().toString();
            if (!sc.isEmpty()) {
                desc = desc.remove('&');
                desc = desc.remove("...");
                desc += " " + values.at(i).tagInt;
                s += QString(" %1: %2\n").arg(sc, 12).arg(desc);
            }
        }
        s += "\n";
    }

    /*
    foreach (g, actionsMap.uniqueKeys())
    {
        s += g +"\n";
        QList <QAction*> values=actionsMap.values(g);
        for (int i=0;i<values.size();++i)
        {
            QString desc=values.at(i)->text();
            QString   sc=values.at(i)->shortcut().toString();
            desc=desc.remove('&');
            desc=desc.remove("...");
            s+= QString(" %1: %2\n").arg(sc,12).arg(desc);
        }
    }
    */
    return s;
}

void Switchboard::printASCII() { std::cout << qPrintable(getASCII()); }

void Switchboard::printLaTeX()
{
    /*
    QString g;
    foreach (g, actionsMap.uniqueKeys()) {
        std::cout << "Group: " << qPrintable(g) << "\\\\ \\hline" << std::endl;
        QList<QAction *> values = actionsMap.values(g);
        for (int i = 0; i < values.size(); ++i)
            if (!values.at(i)->shortcut().toString().isEmpty()) {
                QString desc = values.at(i)->text();
                QString sc = values.at(i)->shortcut().toString();
                desc = desc.remove('&');
                desc = desc.remove("...");
                std::cout << qPrintable(QString(" %1& %2").arg(sc, 12).arg(desc))
                     << std::endl;
            }
        std::cout << std::endl;
    }
    */
}
