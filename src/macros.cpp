#include "macros.h"

#include "settings.h"

#include <QDebug>
#include <QDir>
#include <QMessageBox>
#include <QTextStream>

extern Settings settings;
extern QDir vymBaseDir;

QString Macros::getPath()
{
    return macrosPath;
}

bool Macros::setPath(const QString &path)
{
    if (pathExists(path)) {
        macrosPath = path;
        return true;
    } else
        return false;
}

QString Macros::get()
{
    QFile f(macrosPath);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "Couldn't read macros in get()";
        return QString();
    }
        
    QTextStream ts(&f);
    QString macros = ts.readAll();

    return macros;
}

bool Macros::pathExists(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(
            0, QObject::tr("Warning"),
                QObject::tr("Couldn't find macros at  %1.\n","Macros::pathExists").arg(path) +
                    QObject::tr("Please use Settings->") +
                    QObject::tr("Set directory for vym macros"));
        return false;
    } else
        return true;
}

QString Macros::help()
{
    QRegExp re("^//!.*Macro.*F[0-9]{1,2}");
    return get().split("\n").filter(re).replaceInStrings("//! ", "").join("\n");
}
