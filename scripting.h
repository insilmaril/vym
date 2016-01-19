#ifndef SCRIPTING_H
#define SCRIPTING_H

#include <QObject>
#include <QScriptContext>
#include <QScriptable>
#include <QScriptValue>

class VymModel;

class VymScriptable : protected QScriptable
{
public:
    VymScriptable();
    void throwError(QScriptContext::Error error, const QString &text);
private:
    QScriptContext *ctxt;
};

class VymModelWrapper : public QObject, protected VymScriptable
{
    Q_OBJECT
public:
    VymModelWrapper (VymModel* m);

public slots:
    void addBranch();
    void setHeadingPlainText(const QString &s);
    QString getHeadingPlainText();
    QString getFileName();
private:
    VymModel *model;
};

///////////////////////////////////////////////////////////////////////////
class VymWrapper : public QObject, protected VymScriptable
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
