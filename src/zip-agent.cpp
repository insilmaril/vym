#include "zip-agent.h"

ZipAgent::ZipAgent(QDir zipDir, QString zipName)   // FIXME-4 Does not support deleting unused files
                                                   // Not supported in Windows tar.
                                                   // Probably supported with -FS option on Linux
{
    qDebug() << "Constr ZipAgent";
    zipNameInt = QDir::toNativeSeparators(zipName);
    zipDirInt = zipDir;
    isBackgroundProcessInt = true;
}

ZipAgent::~ZipAgent()
{
    qDebug() << "Destr ZipAgent";
}

void ZipAgent::setBackgroundProcess(bool b)
{
    isBackgroundProcessInt = b;
}

void ZipAgent::startZip()
{
    //qDebug() << "ZipAgent::start";
    connect(this, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(zipProcessFinished(int, QProcess::ExitStatus)));

    setProgram(zipToolPath);
    QStringList args;

#if defined(Q_OS_WINDOWS)
    setWorkingDirectory(QDir::toNativeSeparators(zipInputDir.path() + "\\"));
    args << "-a" << "-c" << "--format" << "zip" << "-f" << zipName << "*";
    args << "-r";
    args << zipNameInt;
    args << ".";
#else
    setWorkingDirectory(QDir::toNativeSeparators(zipDirInt.path()));
    args << "-r";
    args << zipNameInt;
    args << ".";
#endif
    setArguments(args);
    start();

    if (!isBackgroundProcessInt) {
        qDebug() << "ZA::startZip as foreground process"; // FIXME-00 check
        if (!waitForStarted()) {
            // zip could not be started
            QMessageBox::critical(
                0, QObject::tr("Critical Error"),
                QObject::tr("Couldn't start %1 tool to compress data!\n"
                            "The map could not be saved, please check if "
                            "backup file is available or export as XML file!")
                        .arg("zip") +
                    "\n\nziptoolpath: " + zipToolPath +
                    "\nargs: " + args.join(" "));
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
    qDebug() << "ZA::zipProcessFinished  exitCode=" << exitCode << " exitStatus=" << exitStatus;

#if defined(Q_OS_WINDOWS)
    // zip could be started
    if (exitStatus != QProcess::NormalExit) {
        QMessageBox::critical(0, QObject::tr("Critical Error"),
                              QObject::tr("zip didn't exit normally") +
                                  "\n" + getErrout());
        err = File::Aborted;
    }
    else {
        //QMessageBox::information( 0, QObject::tr( "Debug" ),
        //                   "Called:" + zipToolPath + "\n" +
        //                   "Args: "  + args.join(" ") + "\n" +
        //                   "Exit: "  + exitCode + "\n" +
        //                   "Err: " + getErrout()  + "\n" +
        //                   "Std: " + getStdout() );

        if (exitCode > 1) {
            QMessageBox::critical(
                0,
                QObject::tr("Error"),
                QString(
                    "Called: %1\n"
                    "Args: %2\n"
                    "Exit: %3\n"
                    "Err: %4\n"
                    "Std: %5").arg(zipToolPath).arg(args.join(" ")).arg(exitCode).arg(getErrout()).arg(getStdout())
                );
            err = File::Aborted;
        }
        else if (exitCode == 1) {
            // Non fatal according to internet, but for example
            // some file was locked and could not be compressed
            QMessageBox::warning(
                0, QObject::tr("Error"),
                QString(
                    "Called: %1\n"
                    "Args: %2\n"
                    "Err: %3\n"
                    "Std: %4\n"
                    "%5")
                        .arg(zipToolPath)
                        .arg(args.join(" "))
                        .arg(getErrout())
                        .arg(getStdout())
                        .arg(
                           "Please check the saved map, e.g. by opening in "
                           "another tab.\n"
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
