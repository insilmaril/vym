#ifndef SCRIPTING_H
#define SCRIPTING_H

#include <QObject>
#include <QScriptValue>
#include <QScriptable>

class VymModel;

class VymModelWrapper : public QObject
{
    Q_OBJECT
public:
    VymModelWrapper (VymModel* m);

public slots:
    void addBranch(int pos=-2);
    void setHeadingPlainText(const QString &s);
    QString getHeadingPlainText();
    QString getFileName();
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
