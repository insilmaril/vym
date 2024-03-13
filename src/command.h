#ifndef COMMAND_H
#define COMMAND_H

#include <QColor>
#include <QStringList>

class Command {
  public:
    enum SelectionType {
        Any,
        TreeItem,
        Branch,
        BranchLike,
        Image,
        BranchOrImage,
        XLink
    };
    enum ParameterType {
        Undefined,
        Bool,
        BranchItem,
        Color,
        Double,
        Int,
        String,
        Void};

    Command(const QString &n, SelectionType st, ParameterType retType = Void);
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
    void setComment(const QString&);
    QString comment();

  private:
    QString nameInt;
    QString commentInt;
    SelectionType selectionTypeInt;
    QList<ParameterType> parTypes;
    ParameterType returnType;
    QList<bool> parOpts;
    QStringList parComments;
};

#endif
