#ifndef FILE_H
#define FILE_H

#include <QDir>

enum LoadMode {NewMap,ImportAdd,ImportReplace};
enum SaveMode {PartOfMap,CompleteMap,UndoCommand};
enum FileType {VymMap, FreemindMap};
enum ErrorCode {success,aborted,nozip};


/////////////////////////////////////////////////////////////////////////////
QString maskPath (QString );
QString convertToRel (const QString &,const QString &);

QString browseDirectory (QWidget *parent=NULL, const QString &caption="");
bool reallyWriteDirectory(const QString &dir);

QString makeTmpDir (bool &ok, QString prefix);
bool isInTmpDir (QString fn);
QString makeUniqueDir (bool &ok, QString);
void removeDir(QDir);
void copyDir (QDir src,QDir dst);
void makeSubDirs (const QString &);
ErrorCode zipDir (const QDir &,const QString&);
ErrorCode unzipDir (const QDir &,const QString&);
bool loadStringFromDisk (const QString &fn, QString &s);
bool saveStringToDisk (const QString &fn, const QString &s);

/////////////////////////////////////////////////////////////////////////////
#include <QLabel>
#include <q3filedialog.h>
#include <QPixmap>

class ImagePreview : public QLabel, public Q3FilePreview
{
public:
    ImagePreview( QWidget * );
    void previewUrl( const Q3Url & );
protected:
    Q3FileDialog *fdia;
};


//////////////////////////////////////////////////////////
// Helper function to select image format while in SaveDialogs

class ImageIO
{
public:
    ImageIO ();
    QStringList getFilters();
    QString getType ( QString );

private:    
    QStringList imageFilters;
    QStringList imageTypes;
};

#endif
