#include "zip-agent.h"

ZipAgent::ZipAgent(QDir zipDir, QString zipName)   // FIXME-4 Does not support deleting unused files
                                                   // Not supported in Windows tar.
                                                   // Probably supported with -FS option on Linux
{
    qDebug() << "Constr ZipAgent";
    zipNameInt = QDir::toNativeSeparators(zipName);
    zipDirInt = zipDir;
}

ZipAgent::~ZipAgent()
{
    qDebug() << "Dest ZipAgent";
}

void ZipAgent::startZip()
{
    qDebug() << "ZipAgent::start";

    //setProgram("/usr/bin/sleep");
    setProgram("/usr/bin/sleep");
    QStringList args;
    args << "8";
    setArguments(args);
    qDebug() << "a) status=" << state();
    start("/usr/bin/sleep", args);
    qDebug() << "b) status=" << state();

    /*
    File::ErrorCode err = File::Success;

    QString symLinkTarget;

    QString newName;
    // Move existing file away
    QFile file(zipName);
    if (file.exists()) {
        symLinkTarget = file.symLinkTarget();
        QString zipNameTmp = zipName + ".tmp";
        newName = zipNameTmp;
        int n = 0;
        while (!file.rename(newName) && n < 5) {
            newName =
                zipNameTmp + QString().setNum(n);
            n++;
        }
        if (n >= 5) {
            QMessageBox::critical(0, QObject::tr("Critical Error"),
                                  QObject::tr("Couldn't move existing file out "
                                              "of the way before saving."));
            // FIXME-00 return File::Aborted;
        }
    }

    // zip the temporary directory
    VymProcess *zipProc = new VymProcess();
    QStringList args;

#if defined(Q_OS_WINDOWS)
    zipProc->setWorkingDirectory(
        QDir::toNativeSeparators(zipInputDir.path() + "\\"));

    args << "-a" << "-c" << "--format" << "zip" << "-f" << zipName << "*";

    zipProc->start(zipToolPath, args);

    if (!zipProc->waitForStarted()) {
        // zip could not be started
        QMessageBox::critical(
            0, QObject::tr("Critical Error"),
            QObject::tr("Couldn't start %1 tool to compress data!\n"
                        "The map could not be saved, please check if "

                        "backup file is available or export as XML file!")
                    .arg("Windows zip") +
                "\n\nziptoolpath: " + zipToolPath +
                "\nargs: " + args.join(" "));
        err = File::Aborted;
    }
    else {
        // zip could be started
        zipProc->waitForFinished();
        if (zipProc->exitStatus() != QProcess::NormalExit) {
            QMessageBox::critical(0, QObject::tr("Critical Error"),
                                  QObject::tr("zip didn't exit normally") +
                                      "\n" + zipProc->getErrout());
            err = File::Aborted;
        }
        else {
            //QMessageBox::information( 0, QObject::tr( "Debug" ),
            //                   "Called:" + zipToolPath + "\n" +
            //                   "Args: "  + args.join(" ") + "\n" +
            //                   "Exit: "  + zipProc->exitCode() + "\n" +
            //                   "Err: " + zipProc->getErrout()  + "\n" +
            //                   "Std: " + zipProc->getStdout() );

            if (zipProc->exitCode() > 1) {
                QMessageBox::critical(
                    0,
                    QObject::tr("Error"),
                    QString(
                        "Called: %1\n"
                        "Args: %2\n"
                        "Exit: %3\n"
                        "Err: %4\n"
                        "Std: %5").arg(zipToolPath).arg(args.join(" ")).arg(zipProc->exitCode()).arg(zipProc->getErrout()).arg(zipProc->getStdout())
                    );
                err = File::Aborted;
            }
            else if (zipProc->exitCode() == 1) {
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
                            .arg(zipProc->getErrout())
                            .arg(zipProc->getStdout())
                            .arg(
                               "Please check the saved map, e.g. by opening in "
                               "another tab.\n"
                               "Workaround if save failed: Export as xml")
                );

            }
        }
    }
    // qDebug() <<"Output: " << zipProc->getStdout()<<flush;
#else
    zipProc->setWorkingDirectory(QDir::toNativeSeparators(zipInputDir.path()));
//    args << "--filesync"; // FIXME-3 Doesn't seem to change much, should delete vanished files from archive
    args << "-r";
    args << zipName;
    args << ".";

    zipProc->start(zipToolPath, args);
    if (!zipProc->waitForStarted()) {
        // zip could not be started
        QMessageBox::critical(
            0, QObject::tr("Critical Error"),
            QObject::tr("Couldn't start %1 tool to compress data!\n"
                        "The map could not be saved, please check if "
                        "backup file is available or export as XML file!")
                    .arg("zip") +
                "\n\nziptoolpath: " + zipToolPath +
                "\nargs: " + args.join(" "));
        err = File::Aborted;
    }
    else {
        // zip could be started
        zipProc->waitForFinished();
        if (zipProc->exitStatus() != QProcess::NormalExit) {
            QMessageBox::critical(0, QObject::tr("Critical Error"),
                                  QObject::tr("zip didn't exit normally") +
                                      "\n" + zipProc->getErrout());
            err = File::Aborted;
        }
        else {
            if (zipProc->exitCode() > 0) {
                QMessageBox::critical(
                    0, QObject::tr("Critical Error"),
                    QString("zip exit code:  %1").arg(zipProc->exitCode()) +
                        "\n" + zipProc->getErrout());
                err = File::Aborted;
            }
        }
    }
#endif
    // Try to restore previous file, if zipping failed
    if (err == File::Aborted && !newName.isEmpty() && !file.rename(zipName))
        QMessageBox::critical(0, QObject::tr("Critical Error"),
                              QObject::tr("Couldn't rename %1 back to %2")
                                  .arg(newName)
                                  .arg(zipName));
    else {
        // Take care of symbolic link
        if (!symLinkTarget.isEmpty()) {
            if (!QFile(symLinkTarget).remove()) {
                QMessageBox::critical(
                    0, QObject::tr("Critical Error"),
                    QObject::tr(
                        "Couldn't remove target of old symbolic link %1")
                        .arg(symLinkTarget));
                err = File::Aborted;
                // FIXME-00 return err;
            }

            if (!QFile(zipName).rename(symLinkTarget)) {
                QMessageBox::critical(
                    0, QObject::tr("Critical Error"),
                    QObject::tr("Couldn't rename output to target of old "
                                "symbolic link %1")
                        .arg(symLinkTarget));
                err = File::Aborted;
                // FIXME-00 return err;
            }
            if (!QFile(symLinkTarget).link(zipName)) {
                QMessageBox::critical(
                    0, QObject::tr("Critical Error"),
                    QObject::tr("Couldn't link from %1 to target of old "
                                "symbolic link %2")
                        .arg(zipName)
                        .arg(symLinkTarget));
                err = File::Aborted;
                // FIXME-00 return err;
            }
        }

        // Remove temporary file
        if (!newName.isEmpty() && !file.remove())
            QMessageBox::critical(
                0, QObject::tr("Critical Error"),
                QObject::tr("Saved %1, but couldn't remove %2")
                    .arg(zipName)
                    .arg(newName));
    }

    // FIXME-00 return err;
    */
}

