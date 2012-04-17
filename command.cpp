#include "command.h"

#include <QDebug>
Command::Command (const QString &n, SelectionType st)
{
    name=n;
    selectionType=st;
}

QString Command::getName()
{
    return name;
}

void Command::addPar (ParameterType t, bool opt, const QString &c)  
{
    parTypes.append (t);
    parOpts.append (opt);
    parComments.append (c);
}

int Command::parCount()
{
    return parTypes.count();
}

Command::ParameterType Command::getParType (int n)
{
    if (n>=0 && n<parTypes.count() )
    {
	return parTypes.at(n);
    }
    qDebug()<<"Command::getParType n out of range";
    return Undefined;
}

Command::SelectionType Command::getSelectionType ()
{
    return selectionType;
}

bool Command::isParOptional (int n)
{
    if (n>=0 && n<parTypes.count() )
    {
	return parOpts.at(n);
    }
    qDebug()<<"Command::isParOpt n out of range";
    return false;
}

QString Command::getParComment(int n)
{
    if (n>=0 && n<parTypes.count() )
    {
	return parComments.at(n);
    }
    qDebug()<<"Command::getParComment n out of range";
    return QString();
}
