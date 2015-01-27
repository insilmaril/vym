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
QString convertToRel (const QString &,const QString &);
QString convertToAbs (const QString &,const QString &);
QString basename (const QString&);
QString dirname  (const QString&);

QString browseDirectory (QWidget *parent=NULL, const QString &caption="");
bool reallyWriteDirectory(const QString &dir);

QString makeTmpDir (bool &ok, QString prefix);
bool isInTmpDir (QString fn);
QString makeUniqueDir (bool &ok, QString);
void removeDir(QDir);
bool copyDir (QDir src,QDir dst, const bool &override=false);
void makeSubDirs (const QString &);
File::ErrorCode zipDir (QDir , QString);
File::ErrorCode unzipDir (QDir , QString);
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
