#include "macros.h"

#include "settings.h"

#include <QDir>
#include <QTextStream>

extern Settings settings;
extern QDir vymBaseDir;
extern QString macroPath;

QString Macros::getPath (const int &i)
{
    QString pad;
    int n=i;
    if (i%13 <10)
        pad="0";
    else
        pad="";

    QString prefix;
    if (i<13) 
        prefix="";
    else
    {
        prefix="s";
        n=i-12;
    }
    return macroPath + QString("macro-%1%2%3.vys").arg(prefix).arg(pad).arg(n);
}

QString Macros::getMacro (const int &i)    
{
    QString fn=getPath(i);

    QFile f (fn);
    if ( !f.open( QIODevice::ReadOnly ) )
    {
        QObject::tr("Warning"),
                QObject::tr("Couldn't find a macro at  %1.\n").arg(fn)+
                QObject::tr("Please use Settings->")+QObject::tr("Set directory for vym macros");
        return QString();
    }

    QTextStream ts( &f );
    QString macro= ts.readAll();

   return macro;
}
