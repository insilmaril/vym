#include "macros.h"

#include "settings.h"

#include <QDir>
#include <QTextStream>

extern Settings settings;
extern QDir vymBaseDir;
extern QString macroPath;

QString Macros::getPath ()
{
    return QFile (":/macros.vys").fileName();
}

QString Macros::get()
{
    QString fn = getPath();

    QFile f (fn);
    if ( !f.open( QIODevice::ReadOnly ) )
    {
        QObject::tr("Warning"),
                QObject::tr("Couldn't find a macros at  %1.\n").arg(fn)+
                QObject::tr("Please use Settings->")+QObject::tr("Set directory for vym macros");
        return QString();
    }

    QTextStream ts( &f );
    QString macros = ts.readAll();

    return macros;
}

QString Macros::help()
{
    QRegExp re("^//.*Macro.*F[0-9]{1,2}");
    return get().split("\n").filter(re).replaceInStrings("// ", "").join("\n");
}
