#ifndef SCRIPTING_H
#define SCRIPTING_H

#include <QObject>

class BranchItem;
class VymModelWrapper;

///////////////////////////////////////////////////////////////////////////
class VymWrapper : public QObject {
    Q_OBJECT

  public:
    VymWrapper();
    ~VymWrapper();

  public slots:
    void clearConsole();
    bool closeMapWithId(uint n);
    QString currentColor();
    Q_INVOKABLE QObject *currentMap();
    Q_INVOKABLE QObject *mapWithId(uint n);
    uint currentMapID();
    void editHeading();
    bool directoryIsEmpty(const QString &dirName);
    bool directoryExists(const QString &dirName);
    void exit();
    bool fileCopy(const QString &srcPath, QString dstPath);
    bool fileExists(const QString &fileName);
    bool fileRemove(const QString &fileName);
    void gotoMap(uint n);
    bool isConfluenceAgentAvailable();
    QString loadFile(const QString &filename);
    bool loadMap(QString filename);
    int mapCount();
    bool mkdir(const QString &dirName);
    void print(const QString &s);
    void printCol(const QString &color, const QString &s);
    bool removeDirectory(const QString &dirName);
    bool removeFile(const QString &fileName);
    void selectQuickColor(int n);
    void statusMessage(const QString &s);
    void saveFile(const QString &filename, const QString &s);
    bool usesDarkTheme();
    QString version();
    QString vymBaseDir();
};

#endif
