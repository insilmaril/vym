#include <QDebug>
#include <QMultiMap>

#include <iostream>
using namespace std;

#include "shortcuts.h"

/////////////////////////////////////////////////////////////////
// Shortcut
/////////////////////////////////////////////////////////////////
Shortcut::Shortcut(QWidget *parent) :QShortcut (parent)
{
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

QString Switchboard::getASCII()  
{
    QString s;
    QString g;
    foreach (g,actions.uniqueKeys())
    {
	s+= g +"\n";
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
