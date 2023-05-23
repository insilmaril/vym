#include "xml-base.h"

#include "vymmodel.h"

#define qdbg() qDebug().nospace().noquote()

BaseReader::BaseReader(VymModel *vm)
{
    //qDebug() << "Constr. BaseReader (VymModel*)";
    model = vm;
    lastBranch = nullptr;
}

BaseReader::~BaseReader()
{
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

void BaseReader::readHtml()
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("html"));

    bool finished = false;

    while (!finished) {
        // qdbg() << "readHtml: " << xml.name() << " " << xml.tokenString();
        switch(xml.tokenType())
        {
            case QXmlStreamReader::StartElement:
                htmldata += "<" + xml.name().toString();
                for (int i = 0; i < xml.attributes().count(); i++) {
                    htmldata += " " + xml.attributes().at(i).name();
                    htmldata += "=\"" + xml.attributes().at(i).value() + "\"";
                }
                htmldata += ">";
                break;
            case QXmlStreamReader::EndElement:
                htmldata += "</" + xml.name().toString() + ">";
                if (xml.name() == QLatin1String("html"))
                    return;
                break;
            case QXmlStreamReader::Characters:
                htmldata += xml.text().toString();
                break;
            default:
                // Ignore other token types
                break;
        }
        xml.readNext();
        if (xml.tokenType() == QXmlStreamReader::Invalid) {
            qdbg() << "Error in " << xml.lineNumber() << "  " << xml.errorString();
            return;
        }
    }
}

