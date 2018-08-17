#include "macros.h"

#include "settings.h"

#include <QDir>
#include <QTextStream>

extern Settings settings;
extern QDir vymBaseDir;
extern QString macroPath;

QString Macros::getPath ()
{
    return macroPath + "/macros.vys";
}

QString Macros::get()
{
    QString fn = getPath();

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
