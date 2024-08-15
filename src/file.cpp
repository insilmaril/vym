#include <QDebug>
#include <QDir>
#include <QLabel>
#include <QMessageBox>
#include <QPixmap>
#include <QTextStream>

#include "file.h"

#if defined(Q_OS_WINDOWS)
#include "mkdtemp.h"
#include <windows.h>
#endif

#if defined(Q_OS_MACX)
#include "unistd.h"
#endif

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

/*
bool removeDirContent(QDir d) FIXME-2 not used atm
{
        // (might still contain no longer needed images from unzipping before)
        qDebug() << "removeDirContent" << d.path();

        d.setFilter( QDir::NoDotAndDotDot | QDir::Files );
        foreach( QString dirItem, d.entryList() )
            d.remove( dirItem );

        d.setFilter( QDir::NoDotAndDotDot | QDir::Dirs );
        foreach( QString dirItem, d.entryList() )
        {
            QDir subDir( d.absoluteFilePath( dirItem ) );
            subDir.removeRecursively();
        }
}
*/

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
    file.close();

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
