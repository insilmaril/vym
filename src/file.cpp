#include <QDebug>
#include <QDir>
#include <QLabel>
#include <QMessageBox>
#include <QOperatingSystemVersion>
#include <QPixmap>
#include <QTextStream>

#include "file.h"
#include "vymprocess.h"

#if defined(Q_OS_WINDOWS)
#include "mkdtemp.h"
#include <windows.h>
#endif

#if defined(Q_OS_MACX)
#include "unistd.h"
#endif

extern QString zipToolPath;
extern QString unzipToolPath;
extern bool zipToolAvailable;
extern bool unzipToolAvailable;

QString convertToRel(const QString &src, const QString &dst)
{
    // Creates a relative path pointing from src to dst

    QString s = src;
    QString d = dst;
    int i;

    if (s == d) {
        // Special case, we just need the name of the file,
        // not the complete path
        i = d.lastIndexOf("/");
        d = d.right(d.length() - i - 1);
    }
    else {
        // remove identical left parts
        while (s.section("/", 0, 0) == d.section("/", 0, 0)) {
            i = s.indexOf("/");
            s = s.right(s.length() - i - 1);
            d = d.right(d.length() - i - 1);
        }

        // Now take care of paths where we have to go back first
        int srcsep = s.count("/");
        while (srcsep > 0) {
            d = "../" + d;
            srcsep--;
        }
    }
    return d;
}

QString convertToAbs(const QString &src, const QString &dst)
{
    // Creates a relative path pointing from src to dst
    QDir dd(src);
    return dd.absoluteFilePath(dst);
}

QString basename(const QString &path) { return path.section('/', -1); }

QString dirname(const QString &path) { return path.section('/', 0, -2); }

extern QString vymName;
bool confirmDirectoryOverwrite(const QDir &dir)
{
    if (!dir.exists()) {
        qWarning() << "Directory does not exist: " << dir.path();
        return false;
    }

    QStringList eList = dir.entryList();
    while (!eList.isEmpty() && (eList.first() == "." || eList.first() == ".."))
        eList.pop_front(); // remove "." and ".."

    if (!eList.isEmpty()) {
        QMessageBox mb(
               QMessageBox::Warning,
               vymName,
               QObject::tr("The directory %1 is not empty.\nDo you "
                           "risk to overwrite its contents?",
                           "write directory")
                   .arg(dir.path()));

        mb.addButton(QObject::tr("Overwrite"), QMessageBox::AcceptRole);
        mb.addButton(QObject::tr("Cancel"), QMessageBox::RejectRole);
        mb.exec();
        if (mb.result() != QMessageBox::AcceptRole)
            return false;
    }
    return true;
}

QString makeTmpDir(bool &ok, const QString &dirPath,
                   const QString &prefix)
{
    QString path = makeUniqueDir(ok, dirPath + "/" + prefix + "-XXXXXX");
    return path;
}

QString makeTmpDir(bool &ok, const QString &prefix)
{
    return makeTmpDir(ok, QDir::tempPath(), prefix);
}

bool isInTmpDir(QString fn)
{
    QString temp = QDir::tempPath();
    int l = temp.length();
    return fn.left(l) == temp;
}

QString makeUniqueDir(bool &ok, QString s) // FIXME-3 use QTemporaryDir
{
    ok = true;

    QString r;

#if defined(Q_OS_WINDOWS)
    r = mkdtemp(s);
#else
    // On Linux and friends use cstdlib

    // Convert QString to string
    ok = true;
    char *p;
    int bytes = s.length();
    p = (char *)malloc(bytes + 1);
    int i;
    for (i = 0; i < bytes; i++)
        p[i] = s.at(i).unicode();
    p[bytes] = 0;

    r = mkdtemp(p);
    free(p);
#endif

    if (r.isEmpty())
        ok = false;
    return r;
}

bool removeDir(QDir d)
{
    // This check should_ not be necessary, but proved to be useful ;-)
    if (!isInTmpDir(d.path())) {
        qWarning() << "file.cpp::removeDir should remove " + d.path() +
                          " - aborted.";
        return false;
    }

    return d.removeRecursively();
}

bool copyDir(QDir src, QDir dst, const bool &override)
{
    QStringList dirs =
        src.entryList(QDir::AllDirs | QDir::Hidden | QDir::NoDotAndDotDot);
    QStringList files = src.entryList(QDir::Files);

    // Check if dst is a subdir of src, which would cause endless recursion...
    if (dst.absolutePath().contains(src.absolutePath()))
        return false;

    // Traverse directories
    QList<QString>::iterator d, f;
    for (d = dirs.begin(); d != dirs.end(); ++d) {
        if (!QFileInfo(src.path() + "/" + (*d)).isDir())
            continue;

        QDir cdir(dst.path() + "/" + (*d));
        cdir.mkpath(cdir.path());

        if (!copyDir(QDir(src.path() + "/" + (*d)),
                     QDir(dst.path() + "/" + (*d)), override))
            return false;
    }

    // Traverse files
    for (f = files.begin(); f != files.end(); ++f) {
        QFile cfile(src.path() + "/" + (*f));
        QFile destFile(dst.path() + "/" +
                       src.relativeFilePath(cfile.fileName()));
        if (destFile.exists() && override)
            destFile.remove();

        if (!cfile.copy(dst.path() + "/" +
                        src.relativeFilePath(cfile.fileName())))
            return false;
    }
    return true;
}

bool subDirsExist()
{
    QStringList dirList;
    dirList << "images";
    dirList << "flags";
    dirList << "flags/user";
    dirList << "flags/standard";
    foreach (QString d, dirList)
        if (QDir(d).exists() ) return true;

    return false;
}

void makeSubDirs(const QString &s)
{
    QDir d(s);
    d.mkdir(s);
    d.mkdir("images");
    d.mkdir("flags");
    d.mkdir("flags/user");
    d.mkdir("flags/standard");
}

bool checkZipTool()
{
    zipToolAvailable = false;
#if defined(Q_OS_WINDOWS)
    if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows10)
        zipToolAvailable = true;
#else

    QFile tool(zipToolPath);

    zipToolAvailable = tool.exists();
#endif
    return zipToolAvailable;
}

bool checkUnzipTool()
{
    unzipToolAvailable = false;
#if defined(Q_OS_WINDOWS)
    if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows10)
        zipToolAvailable = true;
#else
    QFile tool(unzipToolPath);

    unzipToolAvailable = tool.exists();
#endif
    return unzipToolAvailable;
}

File::ErrorCode zipDir(QDir zipInputDir, QString zipName)   // FIXME-2 remove here and only use ZipAgent
    // FIXME-4 Does not support deleting unused files
    // Not supported in Windows tar.
    // Probably supported with -FS option on Linux
{
    zipName = QDir::toNativeSeparators(zipName);
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
            return File::Aborted;
        }
    }

    // zip the temporary directory
    VymProcess *zipProc = new VymProcess(); // FIXME-2 never deleted?
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
            /*
            QMessageBox::information( 0, QObject::tr( "Debug" ),
                               "Called:" + zipToolPath + "\n" +
                               "Args: "  + args.join(" ") + "\n" +
                               "Exit: "  + zipProc->exitCode() + "\n" +
                               "Err: " + zipProc->getErrout()  + "\n" +
                               "Std: " + zipProc->getStdout() );
            */
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
                return err;
            }

            if (!QFile(zipName).rename(symLinkTarget)) {
                QMessageBox::critical(
                    0, QObject::tr("Critical Error"),
                    QObject::tr("Couldn't rename output to target of old "
                                "symbolic link %1")
                        .arg(symLinkTarget));
                err = File::Aborted;
                return err;
            }
            if (!QFile(symLinkTarget).link(zipName)) {
                QMessageBox::critical(
                    0, QObject::tr("Critical Error"),
                    QObject::tr("Couldn't link from %1 to target of old "
                                "symbolic link %2")
                        .arg(zipName)
                        .arg(symLinkTarget));
                err = File::Aborted;
                return err;
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

    return err;
}

File::ErrorCode unzipDir(QDir zipOutputDir, QString zipName)    // FIXME-2 move to ZipAgent
{
    File::ErrorCode err = File::Success;

    VymProcess *zipProc = new VymProcess(); // FIXME-2 never deleted?
    QStringList args;

#if defined(Q_OS_WINDOWS)
    zipProc->setWorkingDirectory(
        QDir::toNativeSeparators(zipOutputDir.path() + "\\"));
    args << "-x" << "-f" << zipName.toUtf8() << "-C" << zipOutputDir.path();
    zipProc->start(zipToolPath, args);
#else
    zipProc->setWorkingDirectory(QDir::toNativeSeparators(zipOutputDir.path()));
    args << "-o"; // overwrite existing files!
    args << zipName;
    args << "-d";
    args << zipOutputDir.path();

    zipProc->start(unzipToolPath, args);
#endif
    if (!zipProc->waitForStarted()) {
        QMessageBox::critical(
            0, QObject::tr("Critical Error"),
            QObject::tr("Couldn't start %1 tool to decompress data!\n")
                    .arg("Windows zip") +
                "\n\nziptoolpath: " + zipToolPath +
                "\nargs: " + args.join(" "));
        err = File::Aborted;
    }
    else {
        zipProc->waitForFinished();
        if (zipProc->exitStatus() != QProcess::NormalExit) {
            QMessageBox::critical(
                0, QObject::tr("Critical Error"),
                QObject::tr("%1 didn't exit normally").arg(zipToolPath) +
                    zipProc->getErrout());
            err = File::Aborted;
        }
        else {
            /*
            QMessageBox::information( 0, QObject::tr( "Debug" ),
                               "Called:" + zipToolPath + "\n" +
                               "Args: "  + args.join(" ") + "\n" +
                               "Exit: "  + zipProc->exitCode() + "\n" +
                               "Err: " + zipProc->getErrout()  + "\n" +
                               "Std: " + zipProc->getStdout() );
            */
            if (zipProc->exitCode() > 1) {
                QMessageBox::critical(0, QObject::tr("Error"),
                     QString("Called: %1").arg(zipToolPath) +
                     QString("Args: %1").arg(args.join(" ")) +
                     QString("Exit: %1\n").arg(zipProc->exitCode()) +
                     QString("Err: %1\n").arg(zipProc->getErrout()) +
                     QString("Std: %1").arg(zipProc->getStdout())
                );
                err = File::Aborted;
            }
           else if (zipProc->exitCode() == 1) {    // FIXME-3 cleanup, duplicated code
                // Non fatal according to internet, but for example
                // some file was locked and could not be compressed
                QMessageBox::warning(0, QObject::tr("Warning"),
                     QString("Called: %1").arg(zipToolPath) +
                     QString("Args: %1").arg(args.join(" ")) +
                     QString("Exit: %1\n").arg(zipProc->exitCode()) +
                     QString("Err: %1\n").arg(zipProc->getErrout()) +
                     QString("Std: %1").arg(zipProc->getStdout())
                );
            }
        }
    }
    return err;
}

bool loadStringFromDisk(const QString &fname, QString &s)
{
    s = "";
    QFile file(fname);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        qWarning() << QString("loadStringFromDisk: Cannot open file %1\n%2")
                          .arg(fname)
                          .arg(file.errorString());
        return false;
    }

    QTextStream in(&file);
    s = in.readAll();
    return true;
}

bool saveStringToDisk(const QString &fname, const QString &s)
{
    QFile file(fname);
    // Write as binary (default), QFile::Text would convert linebreaks
    if (!file.open(QFile::WriteOnly)) {
        qWarning() << QString("saveStringToDisk: Cannot open file %1:\n%2.")
                          .arg(fname)
                          .arg(file.errorString());
        return false;
    }

    QTextStream out(&file);
    out << s;

    return true;
}

bool appendStringToFile(const QString &fname, const QString &s)
{
    QFile file(fname);
    // Write as binary (default), QFile::Text would convert linebreaks
    if (!file.open(QFile::WriteOnly | QIODevice::Append)) {
        qWarning() << QString("appendStringToFile: Cannot open file %1:\n%2.")
                          .arg(fname)
                          .arg(file.errorString());
        return false;
    }

    QTextStream out(&file);
    out << s;

    return true;
}

File::FileType getMapType(const QString &fn)
{
    int i = fn.lastIndexOf(".");
    if (i >= 0) {
        QString postfix = fn.mid(i + 1);
        if (postfix == "vym" || postfix == "vyp" || postfix == "xml" ||
            postfix == "vym~")
            return File::VymMap;
        if (postfix == "mm")
            return File::FreemindMap;
    }
    return File::UnknownMap;
}

ImageIO::ImageIO()
{
    // Create list with supported image types
    // foreach (QByteArray format, QImageWriter::supportedImageFormats())
    // imageTypes.append( tr("%1...").arg(QString(format).toUpper()));
    imageFilters.append(
        "Images (*.png *.jpg *.jpeg *.bmp *.bmp *.ppm *.xpm *.xbm)");
    imageTypes.append("PNG");
    imageFilters.append("Portable Network Graphics (*.png)");
    imageTypes.append("PNG");
    imageFilters.append("Joint Photographic Experts Group (*.jpg)");
    imageTypes.append("JPG");
    imageFilters.append("Joint Photographic Experts Group (*.jpeg)");
    imageTypes.append("JPG");
    imageFilters.append("Windows Bitmap (*.bmp)");
    imageTypes.append("BMP");
    imageFilters.append("Portable Pixmap (*.ppm)");
    imageTypes.append("PPM");
    imageFilters.append("X11 Bitmap (*.xpm)");
    imageTypes.append("XPM");
    imageFilters.append("X11 Bitmap (*.xbm)");
    imageTypes.append("XBM");
}

QStringList ImageIO::getFilters() { return imageFilters; }

QString ImageIO::getType(QString filter)
{
    for (int i = 0; i < imageFilters.count() + 1; i++)
        if (imageFilters.at(i) == filter)
            return imageTypes.at(i);
    return QString();
}

QString ImageIO::guessType(QString fn)
{
    int i = fn.lastIndexOf(".");
    if (i >= 0) {
        QString postfix = fn.mid(i + 1);
        for (int i = 1; i < imageFilters.count(); i++)
            if (imageFilters.at(i).contains(postfix))
                return imageTypes.at(i);
    }
    return QString();
}
