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

QString BaseReader::parseHREF(QString href)
{
    QString type = href.section(":", 0, 0);
    QString path = href.section(":", 1, 1);
    if (!tmpDir.endsWith("/"))
        return tmpDir + "/" + path;
    else
        return tmpDir + path;
}

