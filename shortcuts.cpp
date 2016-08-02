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
        const QString &kTag,
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
Switchboard::Switchboard ()
{
}

void Switchboard::addGroup( QString gIdentifier, QString gName)
{
    groups.insert(gIdentifier, gName); // FIXME-2 check if identifier already exists
}

void Switchboard::addSwitch( QString identifier, QString scope, QAction *action, QString tag)
{
    // FIXME-2 check, if identifier already exist...
    if (!switches.uniqueKeys().contains(identifier))
    {
        KeySwitch ksw(identifier, action->text(), scope, tag, action->shortcut());
        switches.insert(scope, ksw);
    } else
        qDebug() << "Switchboard::addSwitch warning: Existing idenifier " << identifier;
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
