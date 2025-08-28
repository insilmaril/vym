#ifndef COMMAND_H
#define COMMAND_H

#include <QColor>
#include <QStringList>

class Command {
  public:
    enum ObjectType{
        VymObject,
        MapObject,
        BranchObject,
        ImageObject,
        ItemListObject,
        XLinkObject
    };
    enum ParameterType {
        ArrayPar,
        AttributePar,
        BoolPar,
        BranchPar,
        BranchListPar,
        ColorPar,
        DoublePar,
        ImagePar,
        ItemListPar,
        IntPar,
        StringPar,
        StringListPar,
        UndefinedPar,
        VoidPar,
        XLinkPar
    };
    enum SelectionType {
        AnySel,
        TreeItemSel,    // used 14x 
        BranchSel,
        BranchLikeSel,  // used 2x
        ImageSel,       // used 9x
        BranchOrImageSel,   // used 14x
        XLinkSel
    };

    Command(const QString &n, SelectionType st = Command::AnySel, ParameterType retType = VoidPar);
    QString name();
    QString description();
    QString descriptionLaTeX();
    void addParameter(ParameterType t, bool opt, const QString &c = QString());
    int parameterCount();
    ParameterType parameterType(int n);
    SelectionType selectionType();
    QString selectionTypeName();
    QString typeToString(const ParameterType &type);
    bool isParameterOptional(int n);
    QString parameterComment(int n);
    void setObjectType(const ObjectType &);
    QString objectTypeName();
    void setComment(const QString&);
    QString comment();
    void setReturnType(const ParameterType &type);
    ParameterType returnType();

  private:
    QString nameInt;
    QString commentInt;
    SelectionType selectionTypeInt;
    QList<ParameterType> parTypes;
    ParameterType returnTypeInt;
    ObjectType objectTypeInt;
    QList<bool> parOpts;
    QStringList parComments;
};

#endif
