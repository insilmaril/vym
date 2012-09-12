#include "macros.h"

#include "settings.h"

#include <QDir>
#include <QTextStream>

extern Settings settings;
extern QDir vymBaseDir;

QString Macros::getPath ()
{
    return settings.value ("macros/macroDir",vymBaseDir.path()+"/macros").toString();
}

QString Macros::getPath (const int &i)
{
    return getPath() + QString("/macro-%1.vys").arg(i);
}

QString Macros::getMacro (const int &i)    
{

    QString fn=getPath(i+1);
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
