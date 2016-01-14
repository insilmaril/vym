#ifndef SCRIPTING_H
#define SCRIPTING_H

#include <QObject>

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
private:
    VymModel *model;
};

class VymWrapper : public QObject
{
    Q_OBJECT
public:
    VymWrapper ();

public slots:
    void toggleTreeEditor();
};


#endif
