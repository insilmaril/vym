#ifndef SCRIPTING_H
#define SCRIPTING_H

#include <QObject>
#include <QScriptContext>
#include <QScriptable>
#include <QScriptValue>

#include "branchitem.h"

class VymModel;

void logError(QScriptContext *context, QScriptContext::Error error, const QString &text);

class VymModelWrapper : public QObject, protected QScriptable
{
    Q_OBJECT
public:
    VymModelWrapper (VymModel* m);
    BranchItem* getSelectedBranch();

public slots:
    void addBranch();
    void copy();
    void cut();
    QString getFileName();
    QString getHeadingPlainText();
    void paste();
    void setHeadingPlainText(const QString &s);
private:
    VymModel *model;
};

///////////////////////////////////////////////////////////////////////////
class VymWrapper : public QObject, protected QScriptable
{
    Q_OBJECT
public:
    VymWrapper ();

public slots:
    void toggleTreeEditor();
    QObject* getCurrentMap();
    void selectMap (uint n);
};


#endif
