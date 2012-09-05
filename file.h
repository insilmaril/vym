#ifndef FILE_H
#define FILE_H

#include <QDir>

namespace File
{
    enum ErrorCode {Success,Aborted,NoZip};
}

enum LoadMode {NewMap,ImportAdd,ImportReplace};
enum SaveMode {PartOfMap,CompleteMap,UndoCommand};
enum FileType {VymMap, FreemindMap,UnknownMap};

/////////////////////////////////////////////////////////////////////////////
QString maskPath (QString );
QString convertToRel (const QString &,const QString &);
QString basename (const QString&);

QString browseDirectory (QWidget *parent=NULL, const QString &caption="");
bool reallyWriteDirectory(const QString &dir);

QString makeTmpDir (bool &ok, QString prefix);
bool isInTmpDir (QString fn);
QString makeUniqueDir (bool &ok, QString);
void removeDir(QDir);
void copyDir (QDir src,QDir dst);
void makeSubDirs (const QString &);
File::ErrorCode zipDir (const QDir &,const QString&);
File::ErrorCode unzipDir (const QDir &,const QString&);
bool loadStringFromDisk (const QString &fn, QString &s);
bool saveStringToDisk (const QString &fn, const QString &s);

FileType getMapType ( const QString &fn);

//////////////////////////////////////////////////////////
// Helper function to select image format while in SaveDialogs

class ImageIO
{
public:
    ImageIO ();
    QStringList getFilters();
    QString getType ( QString );
    QString guessType ( QString );

private:    
    QStringList imageFilters;
    QStringList imageTypes;
};

#endif
