#include "command.h"

#include <QDebug>
Command::Command(const QString &n, SelectionType st)
{
    name = n;
    selectionType = st;
}

QString Command::getName() { return name; }

QString Command::getDescription()
{
    QString s;
    s = QString("%1\n").arg(name);
    s += QString("  Selection: %1\n").arg(getSelectionTypeName());
    // s+=QString(" Parameters: %1\n").arg(parCount() );
    for (int i = 0; i < parCount(); i++) {
        s += QString("   Parameter %1:\n").arg(i);
        s += QString("        Comment: %1\n").arg(getParComment(i));
        s += QString("           Type: %1\n").arg(getParTypeName(i));
        s += QString("       Optional: ");
        isParOptional(i) ? s += "yes\n" : s += "No\n";
    }
    return s;
}

QString Command::getDescriptionLaTeX()
{
    QString s;
    s = QString("\\item %1\\\\\n").arg(name);
    s += "\\begin{tabular}{rl}\n";
    s += QString("  Selection: & %1\\\\\n").arg(getSelectionTypeName());
    for (int i = 0; i < parCount(); i++) {
        s += QString("   Parameter: &  %1:\\\\\n").arg(i);
        s += QString("        Comment: & %1\\\\\n").arg(getParComment(i));
        s += QString("           Type: & %1\\\\\n").arg(getParTypeName(i));
        s += QString("       Optional: &  ");
        isParOptional(i) ? s += "yes\\\\\n" : s += "No\\\\\n";
    }
    s += "\\end{tabular}\n";
    return s;
}

void Command::addPar(ParameterType t, bool opt, const QString &c)
{
    parTypes.append(t);
    parOpts.append(opt);
    parComments.append(c);
}

int Command::parCount() { return parTypes.count(); }

Command::ParameterType Command::getParType(int n)
{
    if (n >= 0 && n < parTypes.count()) {
        return parTypes.at(n);
    }
    qDebug() << "Command::getParType n out of range";
    return Undefined;
}

QString Command::getParTypeName(int n)
{
    switch (getParType(n)) {
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
    default:
        return "Undefined";
    }
}

Command::SelectionType Command::getSelectionType() { return selectionType; }

QString Command::getSelectionTypeName()
{
    switch (getSelectionType()) {
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

bool Command::isParOptional(int n)
{
    if (n >= 0 && n < parTypes.count()) {
        return parOpts.at(n);
    }
    qDebug() << "Command::isParOpt n out of range";
    return false;
}

QString Command::getParComment(int n)
{
    if (n >= 0 && n < parTypes.count()) {
        return parComments.at(n);
    }
    qDebug() << "Command::getParComment n out of range";
    return QString();
}
