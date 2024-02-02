#ifndef SCRIPTING_H
#define SCRIPTING_H

#include <QColor>
#include <QObject>
#include <QJSValue>
#include <QVariant>

class BranchItem;
class VymModelWrapper;

/* FIXME-2 old logError, remove...
void logError(QScriptContext *context, QScriptContext::Error error,
              const QString &text);
*/

void logErrorNew(const QString &text);

///////////////////////////////////////////////////////////////////////////
class VymScriptContext : public QObject { // FIXME-0 , protected QScriptable {
    Q_OBJECT
  public:
    VymScriptContext();
    QString setResult(const QString &r);
    bool setResult(bool r);
    int setResult(int r);
    uint setResult(uint r);
    qreal setResult(qreal r);
    int argumentCount();
    QJSValue  argument(int index);
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
