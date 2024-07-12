#include <QOperatingSystemVersion>

#include "zip-agent.h"

extern QString zipToolPath;
extern QString unzipToolPath;

ZipAgent::ZipAgent(QDir zipDir, QString zipName)   // FIXME-4 Does not support deleting unused files
                                                   // Not supported in Windows tar.
                                                   // Probably supported with -FS option on Linux
{
    // qDebug() << "Constr ZipAgent  zipNameInt=" << zipName;
    zipNameInt = QDir::toNativeSeparators(zipName);
    zipDirInt = zipDir;
    isBackgroundProcessInt = true;

}

ZipAgent::~ZipAgent()
{
    //qDebug() << "Destr ZipAgent";
}

bool ZipAgent::checkZipTool()
{
    bool zipToolAvailable = false;
#if defined(Q_OS_WINDOWS)
    if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows10)
        zipToolAvailable = true;
#else
    QFile tool(zipToolPath);
    zipToolAvailable = tool.exists();
#endif
    return zipToolAvailable;
}

bool ZipAgent::checkUnzipTool()
{
    bool unzipToolAvailable = false;
#if defined(Q_OS_WINDOWS)
    if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows10)
        unzipToolAvailable = true;
#else
    QFile tool(unzipToolPath);
    unzipToolAvailable = tool.exists();
#endif
    return unzipToolAvailable;
}

void ZipAgent::setBackgroundProcess(bool b)
{
    isBackgroundProcessInt = b;
}

void ZipAgent::startZip()
{
    //qDebug() << "ZipAgent::startZip";
    connect(this, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(zipProcessFinished(int, QProcess::ExitStatus)));

    setProgram(zipToolPath);

#if defined(Q_OS_WINDOWS) // FIXME-1 tar same result as with zip -FS (delete unused files) on Win 11. Retest on Win 10
    // Uses tar
    setWorkingDirectory(QDir::toNativeSeparators(zipDirInt.path() + "\\"));
    args << "-a" << "-c" << "--format" << "zip" << "-f" << zipNameInt << "*";
#else
    // Uses zip
    setWorkingDirectory(QDir::toNativeSeparators(zipDirInt.path()));
    args << "-FS";  // Also available on Mac.
    args << "-r";
    args << zipNameInt;
    args << ".";
#endif
    setArguments(args);
    start();

    if (!isBackgroundProcessInt) {
        if (!waitForStarted()) {
            // zip could not be started
            QMessageBox::critical(
                0, QObject::tr("Critical Error"),
                QObject::tr("Couldn't start to compress data!\n"
                            "The map could not be saved, please check if "
                            "backup file is available or export as XML file!\n\n")
                        + zipToolPath + args.join(" "));
        }
        else {
            // zip could be started
            waitForFinished();
            if (exitStatus() != QProcess::NormalExit) {
                QMessageBox::critical(0, QObject::tr("Critical Error"),
                                      QObject::tr("zip didn't exit normally"));
            }
            else {
                if (exitCode() > 0) {
                    QMessageBox::critical(
                        0, QObject::tr("Critical Error"),
                        QString("zip exit code:  %1").arg(exitCode()));
                }
            }
        }
    }
}

void ZipAgent::zipProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    //qDebug() << "ZA::zipProcessFinished  exitCode=" << exitCode << " exitStatus=" << exitStatus;

#if defined(Q_OS_WINDOWS)
    // zip could be started
    if (exitStatus != QProcess::NormalExit) {
        QMessageBox::critical(0, QObject::tr("Critical Error"),
                              QObject::tr("zip didn't exit normally"));
    }
    else {
        //QMessageBox::information( 0, QObject::tr( "Debug" ),
        //                   "Called:" + zipToolPath + "\n" +
        //                   "Args: "  + args.join(" ") + "\n" +
        //                   "Exit: "  + exitCode + "\n" +
        //                   "Err: " + getErrout()  + "\n" +
        //                   "Std: " + getStdout() );

        if (exitCode > 1) {
            QString output = readAllStandardError() +"\n" + readAllStandardOutput();
            QMessageBox::critical(
                0,
                QObject::tr("Error"),
                QString(
                    "Called: %1 with %2\n"
                    "Exit: %3\n"
                    "%4").arg(zipToolPath).arg(args.join(" ")).arg(exitCode).arg(output)
                );
        }
        else if (exitCode == 1) {
            QString output = readAllStandardError() +"\n" + readAllStandardOutput();
            // Non fatal according to internet, but for example
            // some file was locked and could not be compressed
            QMessageBox::warning(
                0, QObject::tr("Error"),
                QString(
                    "Called: %1 with %2\n"
                    "Exit: %3\n"
                    "%4").arg(zipToolPath)
                    .arg(args.join(" ")).arg(exitCode).arg(output)
                    .arg("Please check the saved map, e.g. by opening inanother tab.\n"
                         "Workaround if save failed: Export as xml")
            );
        }
    }
#else
    // zip could be started
    if (exitStatus != QProcess::NormalExit) {
        QMessageBox::critical(0, QObject::tr("Critical Error"),
                              QObject::tr("zip didn't exit normally"));
    }
    else {
        if (exitCode > 0) {
            QMessageBox::critical(
                0, QObject::tr("Critical Error"),
                QString("zip exit code:  %1").arg(exitCode));
        }
    }
#endif
    emit(zipFinished());
}

void ZipAgent::startUnzip()
{
    // qDebug() << "ZipAgent::startUnzip "  << zipNameInt << zipDirInt.path();

    // For now only as blocking foreground process
    isBackgroundProcessInt = false;
    //connect(this, SIGNAL(finished(int, QProcess::ExitStatus)),
    //        this, SLOT(unzipProcessFinished(int, QProcess::ExitStatus)));

    setProgram(unzipToolPath);

#if defined(Q_OS_WINDOWS)
    setWorkingDirectory(QDir::toNativeSeparators(zipDirInt.path() + "\\"));
    args << "-x" << "-f" << zipNameInt.toUtf8() << "-C" << zipDirInt.path();
#else
    setWorkingDirectory(QDir::toNativeSeparators(zipDirInt.path()));
    args << "-o"; // overwrite existing files!
    args << zipNameInt;
    args << "-d";
    args << zipDirInt.path();
#endif
    setArguments(args);
    start();

    // qDebug() << "ZA::unzip started " << unzipToolPath << args.join(" ") << "status:" << state();
    if (!isBackgroundProcessInt) {
        if (!waitForStarted()) {
            // zip could not be started
            QMessageBox::critical(
                0, QObject::tr("Critical Error"),
                QObject::tr("Couldn't start tool to decompress data!\n\n")
                        + unzipToolPath + args.join(" "));
        }
        else {
            // zip could be started
            // qDebug() << "ZA wait for unzip to finish";
            waitForFinished();
            if (exitStatus() != QProcess::NormalExit) {
                QMessageBox::critical(0, QObject::tr("Critical Error"),
                                      QObject::tr("zip didn't exit normally"));
            } else {
                if (exitCode() > 0) {
                    QMessageBox::critical(
                        0, QObject::tr("Critical Error"),
                        QString("zip exit code:  %1").arg(exitCode()));
                }
            }
        }
    }
}

