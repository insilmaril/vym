#include <QDebug>
#include <QMultiMap>

#include <iostream>

#include "shortcuts.h"

/////////////////////////////////////////////////////////////////
// KeySwitch
/////////////////////////////////////////////////////////////////
KeySwitch::KeySwitch(const QString &kIdentifier, const QString &kName,
                     const QString &kGroup, const QString &kTag,
                     const QKeySequence &kseq)
{
    identifier = kIdentifier;
    name = kName;
    group = kGroup;
    tag = kTag;
    keySequence = kseq;
}

/////////////////////////////////////////////////////////////////
// Switchboard
/////////////////////////////////////////////////////////////////
Switchboard::Switchboard() {}

void Switchboard::addGroup(QString gIdentifier, QString gName)
{
    if (groups.contains(gIdentifier)) {
        qDebug() << "Warning switchboard: Shortcut group " << gIdentifier
                 << " already exists";
        return;
    }
    groups.insert(gIdentifier, gName);
}

void Switchboard::addSwitch(QString identifier, QString scope, QAction *action,
                            QString tag)
{
    if (!switches.contains(identifier)) {
        KeySwitch ksw(identifier, action->text(), scope, tag,
                      action->shortcut());
        switches.insert(scope, ksw);
    }
    else
        qDebug()
            << "Warning switchboard::addSwitch warning: Existing idenifier "
            << identifier;
}

QString Switchboard::getASCII()
{
    QString s;
    QString g;
    foreach (g, switches.uniqueKeys()) {
        s += "Scope " + g + ":\n";
        QList<KeySwitch> values = switches.values(g);
        for (int i = 0; i < values.size(); ++i) {
            QString desc = values.at(i).name;
            QString sc = values.at(i).keySequence.toString();
            desc = desc.remove('&');
            desc = desc.remove("...");
            s += QString(" %1: %2\n").arg(sc, 12).arg(desc);
        }
        s += "\n";
    }

    /*
    foreach (g, actions.uniqueKeys())
    {
        s += g +"\n";
        QList <QAction*> values=actions.values(g);
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
    QString g;
    foreach (g, actions.uniqueKeys()) {
        std::cout << "Group: " << qPrintable(g) << "\\\\ \\hline" << std::endl;
        QList<QAction *> values = actions.values(g);
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
}
