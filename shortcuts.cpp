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

void Switchboard::print ()  //FIXME-3 open messagebox, maybe showtextdialog
{
    QString g;
    foreach (g,actions.uniqueKeys())
    {
	cout << qPrintable(g)<<endl;
	QList <QAction*> values=actions.values(g);
	for (int i=0;i<values.size();++i)
	{
	    QString desc=values.at(i)->text();
	    QString   sc=values.at(i)->shortcut().toString();
	    desc=desc.remove('&');
	    desc=desc.remove("...");
	    printf (" %10s: %s\n",qPrintable(sc),qPrintable(desc));
	}
	cout <<endl;
    }
}

void Switchboard::printLaTeX ()	//FIXME-2 open messagebox, maybe showtextdialog
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
		printf (" %10s & %s\\\\ \n",qPrintable(sc),qPrintable(desc));
	    }
	cout <<endl;
    }
}
