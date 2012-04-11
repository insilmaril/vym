#ifndef PARSER_H
#define PARSER_H

#include <QColor>
#include <QStringList>

enum ErrorLevel {NoError,Warning,Aborted};

class Command
{
public:
    enum SelectionType {Any, TreeItem, Branch, BranchLike, Image, BranchOrImage, XLinkItem}; 
    enum ParameterType {Undefined,String, Int, Double, Color, Bool};
    Command (const QString &n, SelectionType st);
    QString getName();
    void addPar (ParameterType t, bool opt, const QString &c=QString() );
    int parCount();
    ParameterType getParType (int n);
    bool isParOptional (int n);
    QString getParComment(int n);

private:
	QString name;
	SelectionType selectionType;
	QList <ParameterType> parTypes;
	QList <bool> parOpts;
	QStringList parComments;
};

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
    bool checkParameters();
    bool checkParCount (QList <int> plist);
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
    void addCommand (Command*);

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

    QList <Command*> commands;
};

#endif
