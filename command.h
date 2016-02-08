#ifndef COMMAND_H
#define COMMAND_H

#include <QColor>
#include <QStringList>

class Command
{
public:
    enum SelectionType {Any, TreeItem, Branch, BranchLike, Image, BranchOrImage, XLink}; 
    enum ParameterType {Undefined,String, Int, Double, Color, Bool};
    Command (const QString &n, SelectionType st);
    QString getName();
    QString getDescription();
    QString getDescriptionLaTeX();
    void addPar (ParameterType t, bool opt, const QString &c=QString() );
    int parCount();
    ParameterType getParType (int n);
    QString getParTypeName (int n);
    SelectionType getSelectionType ();
    QString getSelectionTypeName ();
    bool isParOptional (int n);
    QString getParComment(int n);

private:
    QString name;
    SelectionType selectionType;
    QList <ParameterType> parTypes;
    QList <bool> parOpts;
    QStringList parComments;
};

#endif
