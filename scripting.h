#ifndef SCRIPTING_H
#define SCRIPTING_H

#include <QColor>
#include <QObject>
#include <QScriptContext>
#include <QScriptable>
#include <QScriptValue>
#include <QVariant>

class BranchItem;
class VymModel;


void logError(QScriptContext *context, QScriptContext::Error error, const QString &text);

///////////////////////////////////////////////////////////////////////////
class VymWrapper : public QObject, protected QScriptable
{
    Q_OBJECT
public:
    VymWrapper ();

public slots:
    void clearConsole();
    QObject* currentMap();
    bool loadMap( const QString &filename);
    int mapCount();
    void selectMap (uint n);
    void toggleTreeEditor();
    QString version();
};


#endif
