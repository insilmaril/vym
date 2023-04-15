#include "xml-freeplane.h"

#define qdbg() qDebug().nospace().noquote()

#include "vymmodel.h"

FreeplaneReader::FreeplaneReader(VymModel* m)
    : BaseReader(m)
{
    qDebug() << "Constr. FreeplaneReader";
}

QString FreeplaneReader::attrString()
{
    QStringList sl;
    for (int i = 0; i < xml.attributes().count(); i++) {
        sl << xml.attributes()[i].name().toString();
    }
    return sl.join(",");
}

void FreeplaneReader::readToEnd()
{
    QString endName = xml.name().toString();
    QStringList startElements;
    while (!xml.isEndElement() || xml.name() != endName) {
        xml.readNext();
        qdbg() << "FP::readToEnd  " << xml.name() << " " << xml.tokenString();
        if (xml.isStartElement() && !startElements.contains(xml.name()))
            startElements << xml.name().toString();
    }
    qdbg() << "FPR::readToEnd of '" << endName << "' found startElements: [" << startElements.join(", ") << "]";
}

bool FreeplaneReader::read(QIODevice *device)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("map"));

    xml.setDevice(device);

    if (xml.readNextStartElement()) {
        if (xml.name() == QLatin1String("map")) {
            readMap();
        } else {
            xml.raiseError("No vymmap or heading as next element.");
        }
    }

    // FIXME-1 implementation missing...
    return !xml.error();
}

void FreeplaneReader::readArrowLink()
{
    QString elementName = "arrowlink";

    Q_ASSERT(xml.isStartElement() && xml.name() == elementName);

    qdbg () << QString("FP <%1> attributes: %2").arg(elementName).arg(attrString());
    readToEnd();
}

void FreeplaneReader::readAttribute()
{
    QString elementName = "attribute";

    Q_ASSERT(xml.isStartElement() && xml.name() == elementName);

    qdbg () << QString("FP <%1> attributes: %2").arg(elementName).arg(attrString());
    readToEnd();
}

void FreeplaneReader::readCloud()
{
    QString elementName = "cloud";

    Q_ASSERT(xml.isStartElement() && xml.name() == elementName);

    qdbg () << QString("FP <%1> attributes: %2").arg(elementName).arg(attrString());
    readToEnd();
}

void FreeplaneReader::readEdge()
{
    QString elementName = "edge";

    Q_ASSERT(xml.isStartElement() && xml.name() == elementName);

    qdbg () << QString("FP <%1> attributes: %2").arg(elementName).arg(attrString());
    readToEnd();
}

void FreeplaneReader::readFont()
{
    QString elementName = "font";

    Q_ASSERT(xml.isStartElement() && xml.name() == elementName);

    qdbg () << QString("FP <%1> attributes: %2").arg(elementName).arg(attrString());
    qdbg() << "FP::readFont a xml=" << xml.name() << " " << xml.tokenString();
    readToEnd();
    qdbg() << "FP::readFont b xml=" << xml.name() << " " << xml.tokenString();
}

void FreeplaneReader::readIcon()
{
    QString elementName = "icon";

    Q_ASSERT(xml.isStartElement() && xml.name() == elementName);

    qdbg () << QString("FP <%1> attributes: %2").arg(elementName).arg(attrString());

    QString a = "BUILTIN";
    QString s = xml.attributes().value(a).toString();
    if (!s.isEmpty())
        qDebug() << "FR:: Found builtin icon=" << s;

    if (xml.tokenType() == QXmlStreamReader::StartElement)
    qdbg() << "name: " << xml.name() << "  " << xml.tokenString();
    xml.readNext();
    qdbg() << "name: " << xml.name() << "  " << xml.tokenString();
}

void FreeplaneReader::readHook()
{
    QString elementName = "hook";

    Q_ASSERT(xml.isStartElement() && xml.name() == elementName);

    qdbg () << QString("FR <%1> attributes: %2").arg(elementName).arg(attrString());
    readToEnd();
}

void FreeplaneReader::readMap()
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("map"));

    // Check version// FIXME-2 How to deal with Freeplane versions???
    QString a = "version";
    QString s = xml.attributes().value(a).toString();
    if (s.isEmpty()) {
        xml.raiseError("No version found for Freeplane map.");
        return;
    } else {
        qDebug() << "FR: found version " << s;
    }

    // Start with clean map 
    model->clear();
    lastBranch = model->getRootItem();

    while (xml.readNextStartElement()) {
        if (xml.name() == QLatin1String("node")){
            readNode();
        } else {
            raiseUnknownElementError();
            return;
        }
    }
}

void FreeplaneReader::readNode()
{
    QString elementName = "node";

    Q_ASSERT(xml.isStartElement() && xml.name() == elementName);

    qdbg() << QString("FP <%1> attributes: %2").arg(elementName).arg(attrString());

    lastBranch = model->createBranch(lastBranch);

    vymtext.clear();
    htmldata.clear();

    QString a = "TEXT";
    QString s = xml.attributes().value(a).toString();
    if (!s.isEmpty()) {
        qDebug() << "FP::readNode Found text=" << s;
        vymtext.setPlainText(s);
    }

    while (xml.readNextStartElement()) {
        qdbg() << "FP::readNode   startElement=" << xml.name();
        if (xml.name() == QLatin1String("icon"))
            readIcon();
        else if (xml.name() == QLatin1String("arrowlink"))
            readArrowLink();
        else if (xml.name() == QLatin1String("attribute"))
            readAttribute();
        else if (xml.name() == QLatin1String("cloud"))
            readCloud();
        else if (xml.name() == QLatin1String("edge"))
            readEdge();
        else if (xml.name() == QLatin1String("font"))
            readFont();
        else if (xml.name() == QLatin1String("hook"))
            readHook();
        else if (xml.name() == QLatin1String("richcontent"))
            readRichContent();
        else if (xml.name() == QLatin1String("node"))
            readNode();
        else {
            raiseUnknownElementError();
            return;
        }
    }

    if (!htmldata.isEmpty())
        vymtext.setRichText(htmldata);
    lastBranch->setHeading(vymtext);

    lastBranch = lastBranch->parentBranch();
    lastBranch->setLastSelectedBranch(0);
}

void FreeplaneReader::readRichContent()
{
    QString elementName = "richcontent";

    Q_ASSERT(xml.isStartElement() && xml.name() == elementName);

    qdbg() << QString("FP <%1> attributes: %2").arg(elementName).arg(attrString());

    while (xml.readNextStartElement()) {
        if (xml.name() == QLatin1String("html")){
            readHtml();
            qdbg() << "FP: richcontent finished. htmldata=" << htmldata;
        } else {
            raiseUnknownElementError();
            return;
        }
    }

    //readToEnd();
}
