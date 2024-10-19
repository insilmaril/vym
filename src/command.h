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
        XLinkObject
    };
    enum ParameterType {
        AttributeItem,
        Bool,
        BranchItem,
        Color,
        Double,
        ImageItem,
        Int,
        String,
        Undefined,
        Void,
        XLinkItem
    };
    enum SelectionType {
        Any,
        TreeItem,
        Branch,
        BranchLike,
        Image,
        BranchOrImage,
        XLink
    };

    Command(const QString &n, SelectionType st = Command::Any, ParameterType retType = Void);
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

  private:
    QString nameInt;
    QString commentInt;
    SelectionType selectionTypeInt;
    QList<ParameterType> parTypes;
    ParameterType returnType;
    ObjectType objectTypeInt;
    QList<bool> parOpts;
    QStringList parComments;
};

#endif
