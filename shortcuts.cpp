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

void Switchboard::addConnection (QAction *a, const QString &group)
{   
    actions.insert(group,a);
}

void Switchboard::print ()
{
    QString g;
    foreach (g,actions.uniqueKeys())
    {
	cout <<"Group: "<<g.toStdString()<<endl;
	QList <QAction*> values=actions.values(g);
	for (int i=0;i<values.size();++i)
	{
	    cout<<QString ("  %1: %2") 
		.arg(values.at(i)->text().left(30),30)
		.arg(values.at(i)->shortcut().toString()).toStdString()<<endl;
	}
	cout <<endl;
    }
}

void Switchboard::printLaTeX ()
{
    QString g;
    foreach (g,actions.uniqueKeys())
    {
	cout <<"Group: "<<g.toStdString()<<endl;
	QList <QAction*> values=actions.values(g);
	for (int i=0;i<values.size();++i)
	    if (!values.at(i)->shortcut().toString().isEmpty())
		cout<<QString ("  %1& %2\\\\ ") 
		    .arg(values.at(i)->text().left(30),30)
		    .arg(values.at(i)->shortcut().toString()).toStdString()<<endl;
	cout <<endl;
    }
}
