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

  public slots:
    void clearConsole();
    bool closeMapWithID(uint n);
    QString currentColor();
    QObject *currentMap();
    uint currentMapID();
    void editHeading();
    bool directoryIsEmpty(const QString &dirName);
    bool directoryExists(const QString &dirName);
    bool fileCopy(const QString &srcPath, QString dstPath);
    bool fileExists(const QString &fileName);
    bool fileRemove(const QString &fileName);
    void gotoMap(uint n);
    bool isConfluenceAgentAvailable();
    QString loadFile(const QString &filename);
    bool loadMap(const QString &filename);
    int mapCount();
    bool mkdir(const QString &dirName);
    void print(const QString &s);
    bool removeDirectory(const QString &dirName);
    bool removeFile(const QString &fileName);
    void selectQuickColor(int n);
    void statusMessage(const QString &s);
    void saveFile(const QString &filename, const QString &s);
    void toggleTreeEditor();
    bool usesDarkTheme();
    QString version();
};

class Selection : public QObject {
    Q_OBJECT
  public:
    Selection();

  public slots:
    void test();
    void setModel(VymModelWrapper *mw);

  private:
    VymModelWrapper *modelWrapper;
};

#endif
