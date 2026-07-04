#include "debuginfo.h"
#include "git.h"

#include <QCoreApplication>
#include <QDir>
#include <QFont>
#include <QProcessEnvironment>
#include <QSslSocket>
#include <QString>
#include <QStyleFactory>
#include <QTranslator>

#include "settings.h"

extern bool usingDarkTheme;
extern bool systemSeemsDark;
extern QString vymVersion;

extern QString vymVersion;
extern QString vymPlatform;
extern QString vymCodeQuality;
extern QString vymCodeName;
extern QString vymBuildDate;

extern Settings settings;
extern QFont fixedFont;
extern QFont varFont;

extern QString localeName;

extern QDir vymBaseDir;
extern QDir tmpVymDir;          // All temp files go there, created in mainwindow
extern QDir vymTranslationsDir;
extern QTranslator vymTranslator;

extern bool useActionLog;
extern QString actionLogPath;

extern QStringList lastSessionFiles;
extern QString zipToolPath;

QString debugInfo()
{
    QString s;
    s =  QString("vym version: %1 - %2 - \"%3\" Quality: %4\n")
            .arg(vymVersion)
            .arg(vymBuildDate)
            .arg(vymCodeName)
            .arg(vymCodeQuality);
    s += QString("      git foo: %1 branch - commit %2\n").arg(GIT_FOO_BRANCH, GIT_FOO_COMMIT_HASH);
    s += QString("          git: %1 branch - commit %2\n").arg(GIT_BRANCH, GIT_COMMIT_HASH);
    s += QString("     Platform: %1\n").arg(vymPlatform);
    s += QString("    tmpVymDir: %1\n").arg(tmpVymDir.path());
    s += QString("  zipToolPath: %1\n").arg(zipToolPath);
    s += QString("   vymBaseDir: %1\n").arg(vymBaseDir.path());
    s += QString("  currentPath: %1\n").arg(QDir::currentPath());
    s += QString("   appDirPath: %1\n")
            .arg(QCoreApplication::applicationDirPath());
    s += QString("       config: %1\n").arg(settings.fileName());
    s += QString("use actionLog: %1\n").arg(useActionLog);
    s += QString("actionLogPath: %1\n").arg(actionLogPath);
    s += QString("     Settings: %1\n\n").arg(settings.fileName());
    s += QString("   Dark theme: %1   System seems dark: %2\n").arg(usingDarkTheme).arg(systemSeemsDark);
    s += QString("Avail. styles: %1\n\n").arg(QStyleFactory::keys().join(","));
    s += QString(" Fixed font: %1\n").arg(fixedFont.toString());
    s += QString("   Var font: %1\n").arg(varFont.toString());
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
    s += QString("               country: %1\n").arg(QLocale::territoryToString(QLocale::system().territory()));
    s += QString("           uiLanguages: %1\n").arg(QLocale::system().uiLanguages().join(","));
    s += QString("                  LANG: %1\n")
        .arg(QProcessEnvironment::systemEnvironment().value("LANG", "not set."));
    s += QString("       Translations in: %1\n").arg(vymTranslationsDir.path());
    s += QString("Available translations: %1\n").arg(translations.count());
    foreach (QString qm_file, translations)
        s += QString("                        %1\n").arg(qm_file);

    s += QString("      lastSessionFiles: %1\n").arg(lastSessionFiles.join(", "));

    return s;
}
