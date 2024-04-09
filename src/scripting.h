#ifndef SCRIPTING_H
#define SCRIPTING_H

#include <QColor>
#include <QObject>
#include <QJSValue>
#include <QVariant>

class BranchItem;
class VymModelWrapper;

///////////////////////////////////////////////////////////////////////////
class VymScriptContext : public QObject {
    Q_OBJECT
  public:
    VymScriptContext();
    QString setResult(const QString &r);
    bool setResult(bool r);
    int setResult(int r);
    uint setResult(uint r);
    qreal setResult(qreal r);
};

///////////////////////////////////////////////////////////////////////////
class VymWrapper : public VymScriptContext {
    Q_OBJECT
  public:
    VymWrapper();

  public slots:
    void print(const QString &s);
    void statusMessage(const QString &s);
    void abortScript(const QString &s);
    void clearConsole();
    bool isConfluenceAgentAvailable();
    QObject *currentMap();
    void editHeading();
    bool directoryIsEmpty(const QString &dirName);
    bool directoryExists(const QString &dirName);
    bool mkdir(const QString &dirName);
    bool removeDirectory(const QString &dirName);
    bool fileExists(const QString &fileName);
    bool removeFile(const QString &fileName);
    bool loadMap(const QString &filename);
    int mapCount();
    void gotoMap(uint n);
    bool closeMapWithID(uint n);
    void selectQuickColor(int n);
    QString currentColor();
    uint currentMapID();
    void toggleTreeEditor();
    QString loadFile(const QString &filename);
    void saveFile(const QString &filename, const QString &s);
    QString version();
};

class Selection : public VymScriptContext {
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
