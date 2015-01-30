#include <QDebug>
#include <QMultiMap>

#include <iostream>
using namespace std;

#include "shortcuts.h"

/////////////////////////////////////////////////////////////////
// KeySwitch
/////////////////////////////////////////////////////////////////
KeySwitch::KeySwitch (
        const QString &kIdentifier,
        const QString &kName,
        const QString &kGroup,
        const QKeySequence &kseq)
{
    identifier = kIdentifier;
    name = kName;
    group = kGroup;
    keySequence = kseq;
}

/////////////////////////////////////////////////////////////////
// Switchboard
/////////////////////////////////////////////////////////////////
Switchboard::Switchboard ()
{
}

void Switchboard::addConnection (QAction *a, const QString &group)  //FIXME-4 obsolete
{   
    actions.insert(group,a);
}

void Switchboard::addConnection (QWidget *w, QAction *a, const QString &group)
{   
    actions.insert(group,a);
    if (w) w->addAction (a);
}

void Switchboard::addGroup( QString gIdentifier, QString gName)
{
    groups.insert(gIdentifier, gName); // FIXME-2 check if identifier already exists
}

void Switchboard::addSwitch( QString identifier, QString name, QString group, QKeySequence kseq)
{
    // FIXME-1 check, if identifier already exist...
    if (!switches.uniqueKeys().contains(name))
    {
        KeySwitch ksw(identifier, name, group, kseq);
        switches.insert(group, ksw);
    }
}
void Switchboard::addSwitch( QString identifier, QString group, QAction *action)
{
    // Overloaded for convenience
    // Get parameters from action
    addSwitch( identifier, action->text(), group, action->shortcut());
}

QString Switchboard::getASCII()  
{
    QString s;
    QString g;
    foreach (g, switches.uniqueKeys())
    {
        s += "Scope " + g +":\n";
        QList <KeySwitch> values=switches.values(g);
        for (int i=0; i<values.size(); ++i)
        {
            QString desc=values.at(i).name;
            QString   sc=values.at(i).keySequence.toString();
            desc=desc.remove('&');
            desc=desc.remove("...");
            s += QString(" %1: %2\n").arg(sc,12).arg(desc);
        }
        s += "\n";
    }

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
    return s;
}

void Switchboard::printASCII ()	
{
    cout <<qPrintable(getASCII() );
}

void Switchboard::printLaTeX ()	
{
    QString g;
    foreach (g,actions.uniqueKeys())
    {
        cout <<"Group: "<<qPrintable(g)<<"\\\\ \\hline"<<endl;
        QList <QAction*> values=actions.values(g);
        for (int i=0;i<values.size();++i)
            if (!values.at(i)->shortcut().toString().isEmpty())
            {
                QString desc=values.at(i)->text();
                QString   sc=values.at(i)->shortcut().toString();
                desc=desc.remove('&');
                desc=desc.remove("...");
                cout << qPrintable( QString(" %1& %2").arg(sc,12).arg(desc) )<<endl;
            }
        cout <<endl;
    }
}
