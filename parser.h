#ifndef PARSER_H
#define PARSER_H

#include <QColor>
#include <QStringList>

enum ErrorLevel {NoError,Warning,Aborted};

class Command;
class TreeItem;

class Parser
{
public:
    Parser();
    void parseAtom (QString input);
    QString getAtom();
    QString getCommand();
    QStringList getParameters();
    int parCount();
    QString errorMessage();
    QString errorDescription();
    ErrorLevel errorLevel();
    void setError (ErrorLevel level,const QString &description);
    void resetError();
    bool checkParameters(TreeItem *selti);
    bool checkParCount (const int &index);
    bool checkParIsInt (const int &index);
    bool checkParIsDouble (const int &index);
    int parInt (bool &,const uint &index);
    QString parString(bool &ok,const int &index);
    bool parBool (bool &ok, const int &index);
    QColor parColor (bool &ok, const int &index);
    double parDouble (bool &ok, const int &index);

    void setScript (const QString &);
    QString getScript();
    void execute();
    bool next();

    QStringList getCommands(); 

private:
    void initParser();
    void initAtom();

    QString input;
    QString atom;
    QString com;
    QStringList paramList;
    int current;
    QString script;

    QString errMessage;
    QString errDescription;
    ErrorLevel errLevel;
};

#endif
