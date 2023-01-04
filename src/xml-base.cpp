#include "xml-base.h"

#include "vymmodel.h"


BaseReader::BaseReader(VymModel *m)
{
    //qDebug() << "Constr. BaseReader";
    model = m;
}

QString BaseReader::errorString() const
{
    return QObject::tr("%1\nLine %2, column %3")
            .arg(xml.errorString())
            .arg(xml.lineNumber())
            .arg(xml.columnNumber());
}

void BaseReader::setLoadMode(const File::LoadMode &lm, int p)
{
    loadMode = lm;
    insertPos = p;
}
