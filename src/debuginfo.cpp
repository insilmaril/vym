#include "debuginfo.h"

#include <QCoreApplication>
#include <QDir>
#include <QProcessEnvironment>
#include <QSslSocket>
#include <QString>
#include <QStyleFactory>
#include <QTranslator>

#include "settings.h"

extern bool usingDarkTheme;
extern QString vymVersion;

extern QString vymVersion;
extern QString vymPlatform;
extern QString vymCodeQuality;
extern QString vymCodeName;
extern QString vymBuildDate;

extern Settings settings;

extern QString localeName;

extern QDir vymBaseDir;
extern QDir tmpVymDir;          // All temp files go there, created in mainwindow
extern QDir vymTranslationsDir;
extern QTranslator vymTranslator;

extern QString zipToolPath;

QString debugInfo()
{
    QString s;
    s =  QString("vym version: %1 - %2 - %3 %4\n")
            .arg(vymVersion)
            .arg(vymBuildDate)
            .arg(vymCodeQuality)
            .arg(vymCodeName);
    s += QString("     Platform: %1\n").arg(vymPlatform);
    s += QString("    tmpVymDir: %1\n").arg(tmpVymDir.path());
    s += QString("  zipToolPath: %1\n").arg(zipToolPath);
    s += QString("   vymBaseDir: %1\n").arg(vymBaseDir.path());
    s += QString("  currentPath: %1\n").arg(QDir::currentPath());
    s += QString("   appDirPath: %1\n")
            .arg(QCoreApplication::applicationDirPath());
    s += QString("     Settings: %1\n\n").arg(settings.fileName());
    s += QString("   Dark theme: %1\n").arg(usingDarkTheme);
    s += QString("Avail. styles: %1\n\n").arg(QStyleFactory::keys().join(","));
    s += " SSL status: ";
    QSslSocket::supportsSsl() ? s += "supported\n" : s += "not supported\n";
    s += "     SSL Qt: " + QSslSocket::sslLibraryBuildVersionString() + "\n";
    s += "    SSL lib: " + QSslSocket::sslLibraryVersionString() + "\n";

    // Info about translations
    QStringList translations;
    if(vymTranslationsDir.exists())
        translations = vymTranslationsDir.entryList();
    s += "\n";
    s += QString("       Translator path: %1\n").arg(vymTranslator.filePath());
    s += QString("       Translator lang: %1\n").arg(vymTranslator.language());
    s += QString("            localeName: %1\n").arg(localeName);
    s += QString("                system: %1\n").arg(QLocale::system().name());
    s += QString("              language: %1\n").arg(QLocale::languageToString(QLocale::system().language()));
    s += QString("               country: %1\n").arg(QLocale::countryToString(QLocale::system().country()));
    s += QString("           uiLanguages: %1\n").arg(QLocale::system().uiLanguages().join(","));
    s += QString("                  LANG: %1\n")
        .arg(QProcessEnvironment::systemEnvironment().value("LANG", "not set."));
    s += QString("       Translations in: %1\n").arg(vymTranslationsDir.path());
    s += QString("Available translations: %1\n").arg(translations.count());
    foreach (QString qm_file, translations)
        s += QString("                        %1\n").arg(qm_file);

    return s;
}
