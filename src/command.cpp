#include "command.h"

#include <QDebug>
Command::Command(const QString &n, SelectionType st, ParameterType retType)
{
    nameInt = n;
    selectionTypeInt = st;
    returnType = retType;
}

QString Command::name() { return nameInt; }

QString Command::description()
{
    QString s;
    s  = QString("        Command: \"%1\"\n").arg(nameInt);
    s += QString("         Object: %1\n").arg(objectTypeName());
    s += QString("        Comment: %1\n").arg(commentInt);
    if (objectTypeInt == MapObject)
        s += QString("  SelectionType: %1\n").arg(selectionTypeName());
    s += QString("    Return type: %1\n").arg(typeToString(returnType));
    s += QString("     Parameters: %1\n").arg(parameterCount());
    // s+=QString(" Parameters: %1\n").arg(parameterCount() );
    for (int i = 0; i < parameterCount(); i++) {
        s += QString("       Parameter #%1:\n").arg(i + 1);
        s += QString("             Comment: %1\n").arg(parameterComment(i));
        s += QString("                Type: %1\n").arg(typeToString(parameterType(i)));
        s += QString("            Optional: ");
        isParameterOptional(i) ? s += "yes\n" : s += "No\n";
    }
    return s;
}

QString Command::descriptionLaTeX()
{
    QString s;
    s = QString("\\item %1\\\\\n").arg(nameInt);
    s += "\\begin{tabular}{rl}\n";
    s += QString("        Comment: & %1\\\\\n").arg(commentInt);
    s += QString("  SelectionType: & %1\\\\\n").arg(selectionTypeName());
    s += QString("    Return Type: & %1\\\\\n").arg(typeToString(returnType));

    s += QString("     Parameters: & %1\\\\\n").arg(parameterCount());
    for (int i = 0; i < parameterCount(); i++) {
        s += QString("   Parameter: &  %1:\\\\\n").arg(i + 1);
        s += QString("        Comment: & %1\\\\\n").arg(parameterComment(i));
        s += QString("           Type: & %1\\\\\n").arg(typeToString(parameterType(i)));
        s += QString("       Optional: &  ");
        isParameterOptional(i) ? s += "yes\\\\\n" : s += "No\\\\\n";
    }
    s += "\\end{tabular}\n";
    return s;
}

void Command::addParameter(ParameterType t, bool opt, const QString &c)
{
    parTypes.append(t);
    parOpts.append(opt);
    parComments.append(c);
}

int Command::parameterCount() { return parTypes.count(); }

Command::ParameterType Command::parameterType(int n)
{
    if (n >= 0 && n < parTypes.count()) {
        return parTypes.at(n);
    }
    qDebug() << "Command::parameterType n out of range";
    return Undefined;
}

QString Command::typeToString(const ParameterType &type)
{
    switch (type) {
    case String:
        return "String";
    case Int:
        return "Int";
    case Double:
        return "Double";
    case Color:
        return "Color";
    case Bool:
        return "Bool";
    case Void:
        return "Void";
    case Undefined:
        return "Undefined";
    default:
        return "not defined in class Command.";
    }
}

Command::SelectionType Command::selectionType() { return selectionTypeInt; }

QString Command::selectionTypeName()
{
    switch (selectionType()) {
    case Any:
        return "Any";
    case TreeItem:
        return "TreeItem";
    case Branch:
        return "Branch";
    case BranchLike:
        return "BranchLike";
    case Image:
        return "Image";
    case BranchOrImage:
        return "BranchOrImage";
    case XLink:
        return "XLink";
    default:
        return "Undefined";
    }
}

bool Command::isParameterOptional(int n)
{
    if (n >= 0 && n < parTypes.count()) {
        return parOpts.at(n);
    }
    qDebug() << "Command::isParOpt n out of range";
    return false;
}

QString Command::parameterComment(int n)
{
    if (n >= 0 && n < parTypes.count()) {
        return parComments.at(n);
    }
    qDebug() << "Command::parameterComment n out of range";
    return QString();
}

void  Command::setObjectType(const ObjectType &ot) {
    objectTypeInt = ot;
}

QString  Command::objectTypeName() {
    switch (objectTypeInt) {
    case VymObject:
        return "Vym program";
    case MapObject:
        return "Vym map";
    case BranchObject:
        return "Branch in a map";
    default:
        return "Undefined";
    }
}

void  Command::setComment(const QString &s) {
    commentInt = s;
}

QString Command::comment()
{
    return commentInt;
}
