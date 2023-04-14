#include "xml-base.h"

#include "vymmodel.h"


BaseReader::BaseReader(VymModel *vm)
{
    //qDebug() << "Constr. BaseReader (VymModel*)";
    model = vm;
    lastBranch = nullptr;
}

void BaseReader::setContentFilter(const int &c) { contentFilter = c; }

QString BaseReader::errorString() const
{
    return QObject::tr("%1\nLine %2, column %3")
            .arg(xml.errorString())
            .arg(xml.lineNumber())
            .arg(xml.columnNumber());
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

void BaseReader::setModel(VymModel *vm)
{
    model = vm;
}

void BaseReader::setTmpDir(QString tp)
{
    tmpDir = tp;
}

void BaseReader::setInputString(const QString &s) { inputString = s; }

void BaseReader::setLoadMode(const File::LoadMode &lm, int p)
{
    loadMode = lm;
    insertPos = p;
}

void  BaseReader::raiseUnknownElementError()
{
    xml.raiseError("Found unknown element: " + xml.name().toString());
}

QString BaseReader::attributeToString(const QString &a)
{
    return xml.attributes().value(a).toString();
}

