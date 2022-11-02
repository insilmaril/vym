#ifndef SCRIPTING_H
#define SCRIPTING_H

#include <QColor>
#include <QObject>
#include <QScriptContext>
#include <QScriptValue>
#include <QScriptable>
#include <QVariant>

class BranchItem;
class VymModelWrapper;

void logError(QScriptContext *context, QScriptContext::Error error,
              const QString &text);

///////////////////////////////////////////////////////////////////////////
class VymScriptContext : public QObject, protected QScriptable {
    Q_OBJECT
  public:
    VymScriptContext();
    QString setResult(const QString &r);
    bool setResult(bool r);
    int setResult(int r);
    uint setResult(uint r);
};

///////////////////////////////////////////////////////////////////////////
class VymWrapper : public VymScriptContext {
    Q_OBJECT
  public:
    VymWrapper();

  public slots:
    void clearConsole();
    bool isConfluenceAgentAvailable();
    QObject *currentMap();
    void editHeading();
    bool loadMap(const QString &filename);
    int mapCount();
    void selectMap(uint n);
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
